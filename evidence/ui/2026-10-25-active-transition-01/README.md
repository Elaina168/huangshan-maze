# 0.4.1间歇时序故障与0.4.2活动视图

2026-10-25。raw.log原样复制用户附件0287ac5f-726a-45c0-a0ca-9bc033bbef41，字节与SHA见analysis.json；不是聊天重建。附件没有启动版本或设备端哈希，按本次0.4.1复测上下文关联，不冒称读取设备固件校验。

## 实际结果

gen11/15陷阱失败用时7246/7760ms，gen13胜利用时12451ms；用户反馈“现在能玩”。同时gen12/14/16/17出现-1004，诊断service104–113ms、render3ms、loop108–117ms；producer_gap50ms/count0、age7–35ms，说明本组生产采样健康、显示服务仍超限。gen18还有一次恢复校准-1003，不能误当同一时序错误。RGB黑帧请求result0/tick151911，但用户确认绿灯仍亮。

## RED→GREEN

基于真实服务耗时，离屏回归把进入游玩的刷新上限从100000收紧到40000（技术回归约束，不是产品帧率或毫秒验收阈值）。保留旧源码before-active-view.c.txt。旧版刷新70492、断言start_pixels<40000实际失败；改为校准/游玩/暂停都不遮地图、进度与暂停提示放标题后，校准→游玩/暂停→游玩均33292、移动9518，断言通过，见host-ui-refresh.log。真实LVGL软件渲染不是假绘图桩；实际目标EPIC/双缓冲与服务时间仍需上板复测。

host-maze.log：后台13/输入8/模型7/状态9；host-probe.log：原探针7+14和占用守卫；host-ui-events.log：3组队列交接，均退出0。Windows快照4/4通过；最终项目材料检查另执行。新代码只改UI与RGB诊断，不放宽100ms、不自动继续、不改采样优先级。

## RGB与审查

核对实际SDK：RGB PA32专用脚、GPTIM2_CH1，PWM更新DMA内存/外设均半字，SF32LB52 GPTIM2固定24MHz；没有据错误注释或猜测改变波形。HAL_PIN_Get可读实际功能，PWM_CMD_GET读定时器配置。maze_rgb_off记录真实映射/期望值，只有不符才恢复该专用脚，再发黑帧并读回周期/脉宽。返回0和寄存器值都不是光学反馈；绿灯原因与实际熄灭仍未确认，共享电源和SDK不改。

独立Codex只读审阅本次maze_ui/main与刷新断言，未发现Important/Critical；审查不修改文件、不运行测试、不验证硬件。开发者执行回归及源码/固件hash核验，不将审查当作真机通过。

最终[构建](../../builds/2026-10-25-active-view-01/README.md)版本0.4.2-active-view；用户只需复测Start→暂停→Resume，异常贴maze_timing；RGB仍绿则运行maze_rgb_off并提供完整maze_rgb行。胜败闭环已有局部真机记录，但稳定性、资源及正式TC仍未全部执行。未提交推送。
