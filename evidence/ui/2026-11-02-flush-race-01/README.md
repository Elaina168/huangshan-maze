# 小球闪烁与持久黑块：异步源缓冲竞态

2026-11-02，关联REQ-037/038、TC-054/058。[用户原始反馈](user-feedback.md)为第三关球闪烁伴随持久黑块，再经过重绘才恢复。

## 源码证据

固定SDK v2.5.1，源文件SHA见sdk-sources.json；未改SDK。

- middleware/lvgl/lv_drivers_v9/lv_lcd.c:257-268：tx完成回调先rt_sem_release(lcd_sema)，后p_disp->flushing=0。该文件233-242的wait_flush_done以take/release等待同一信号量；300-343的lcd_flush先take许可再异步提交。
- rtos/rtthread/bsp/sifli/drivers/drv_lcd.c:2193-2206：lcd_task调用draw_core完成后才调用aysnc_send_cmplt_cbk；1051-1062再调用tx_complete。这是线程上下文；此前真实ps已见main优先级10、lcd_task12。
- rtos/rtthread/src/ipc.c：rt_sem_release唤醒等待者后会调度。因而旧LCD线程可在release后被高优先级main抢占，main等待结束后重绘/提交下一笔，旧回调再恢复清掉新flushing。
- external/lvgl_v9/src/core/lv_refr.c:1017-1021：单缓冲仅据flushing决定是否调用等待；1402-1423的FLUSH_FINISH只是flush提交函数返回，并非物理传输完成。标志被旧回调晚清后，绘图源可能在LCD还使用时被覆写。

## 项目侧修复

src/maze_ui.c在FLUSH_FINISH中每次显式调用已安装的驱动flush_wait_cb，再flush_ready，最后才运行原纯逻辑协作服务。等待不依赖易受旧回调影响的flushing值；同一SDK等待函数take后release，不吞许可，逐笔只等待物理完成。同步主机显示无wait_cb时保持原同步回调路径。没有修改SDK、绘图大小/物理/输入100ms限制、地图、存档格式或分区。

固定SDK的公开API无等待回调getter，因此仅UI适配读取lv_display_private.h已确认字段，升级SDK需重新核对此边界。

## RED / GREEN

真实LVGL+ui_async_harness异步替身保存源指针、延后消费，模拟旧完成回调晚清新标志，按笔核对提交与消费CRC。

- red.log：354提交，源覆写302次，最终球0，断言失败。此前同步离屏flush不可能暴露这类交错。
- green.log：354提交/354真实替身等待，源覆写0；每笔消费后最小球128=完整球128，零色黑洞累计0。包含不滚镜头快速移动和横纵滚动/折返。
- refresh/scroll/third/map-error.log：原界面状态、字体与刷新面积、共用试验路径、三关及地图错误页回归。
- main.log：实际main的I/O替身，启动迁移/重试/结算边界回归。存储实现本轮未改，不重复无关395次中断。

这个替身证明上述交错下缓冲生命周期修复，并非复现真实DMA、电气噪声或穷尽全部硬件原因。是否消除用户看到的闪球/黑块须用新固件复测同一路径。物理等待加入服务周期，保留原100ms约束，必须观察physics_gap_tick。没有扩大旧B/C或自动烧录/提交推送。
