# 显示、触摸与资源接口静态核对

日期：2026-10-17。对应课程C-11/14/17/25，支持REQ-001/013/015/016及TC-017/018/020/021的后续设计。性质是读取SDK、参考工程及已有构建，不是新增界面或硬件验收。固件未改、未重编译，RGB保留；新增文档本地保存，收工再提交推送。

后续实施说明：以上与第1–4节记录显示启用前的核对。用户已认可书面设计及本会话执行计划，Task 1开始接入显示/触摸预研页，关闭RGB闪灯；该后续固件的构建与硬件结果单独登记，不改写之前的证据。

## 1. 来源与当前工程

SDK为`D:/sifli/SiFli-SDK/v2.5.1`，本次读取HEAD仍是`421126d9f476ed8e2a6f0b0ca28a9f241c182e65`；参考工程为`D:/sifli/peripheral_lab`。以下SDK/参考路径均相对对应根目录，不把这些源码当作本项目已实现功能。

当前构建依据[IMU控制诊断构建](../../evidence/builds/2026-10-10-imu-control-01/README.md)及其ASCII快照的rtconfig.h、main.map。project/proj.conf只有RGB及I2C3配置，没有启用LVGL；生成配置已有LCD及触摸驱动，不等于已有应用界面。

| 核对项 | 实际材料 | 结论与边界 |
|---|---|---|
| 面板配置 | 板型`customer/boards/sf32lb52-lchspi-ulp/hcpu/board.conf`启用`LCD_USING_TFT_CO5300`与`BSP_USING_TOUCHD`；生成rtconfig.h有`LCD_HOR_RES_MAX=390`、`LCD_VER_RES_MAX=450`、`TSC_USING_FT6146` | 当前构建配置为390×450，不能替代实测物理方向、可见边界或触摸对应关系 |
| 显示驱动 | `customer/peripherals/display/co5300/co5300.c`通过LCD_HOR_RES_MAX/LCD_VER_RES_MAX定义尺寸 | 来源是驱动配置，不猜分辨率或旋转角度 |
| LVGL版本选择 | 参考proj.conf为`CONFIG_PKG_USING_LITTLEVGL2RTT=y`、`CONFIG_LVGL_V9=y`；SDK `middleware/lvgl/SConscript`在LVGL_V9条件下选择lv_drivers_v9 | 后续按依赖逐步启用，不复制参考工程全部蓝牙、音频、网络配置 |
| 应用入口 | 参考`src/main.c`调用`littlevgl2rtt_init("lcd")`，在主线程循环调用`lv_timer_handler()`，等待限于2–20ms | 可作为应用接入依据，2–20ms不是本项目触摸/可见运动延迟实测 |
| SDK初始化 | `middleware/lvgl/littlevgl2rtt.c`的gui_lib_init调用lv_init并通过INIT_COMPONENT_EXPORT注册；littlevgl2rtt_init调用lv_hal_init并记录host_thread | 后续明确主线程拥有UI；后台IMU线程不操作LVGL。初始化返回RT_EOK不替代逐项显示/触摸验收 |
| LVGL显示与触摸 | `middleware/lvgl/lv_drivers_v9/lvgl_drv.c`配置tick回调，初始化lcd并创建`"touch"`输入；lv_touch.c读取touch_message并传递x/y | 后续在LVGL事件中接收触摸，不另起竞争读取同一触摸设备的任务 |
| 帧缓冲 | lv_drivers_v9/lv_lcd.c按LV_COLOR_DEPTH、LV_FB_LINE_NUM及单双缓冲配置分配；FRAME_BUFFER_IN_PSRAM决定相应内存区段 | 必须结合实际链接映射及运行数据预算，不从RGB静态RAM占用推定可用显示内存 |

屏幕朝上、USB在左是此前IMU实验的操作朝向；尚无本工程可见方向标记及四角触摸测试，不能宣布该朝向已和LVGL坐标对齐。显示接入后需画方向/边界标记，核对左上原点、向右/向下及触摸点，再与IMU横向负Y差、纵向X差对接。

## 2. 当前固件可直接使用的资源命令

已有main.map包含`__fsym___cmd_free`、`__fsym___cmd_ps`、`__fsym___cmd_list_mem`及`__fsym___cmd_list_thread`。随后取得[板端free/ps记录](../../evidence/resources/2026-10-17-before-ui-01/README.md)，与命令行为一致；设备运行哈希仍未独立核验。

| 命令/API | 实际SDK路径与行为 | 解释 |
|---|---|---|
| `free` | `rtos/rtthread/components/finsh/msh_cmd.c`的cmd_free；当前未启用RT_USING_MEMHEAP_AS_HEAP，调用list_mem | 查看系统堆，不是固件文件大小或全部物理RAM |
| list_mem / rt_memory_info | 当前生成配置为RT_USING_SMALL_MEM，`rtos/rtthread/src/mem.c`报告total、used、maximum allocated memory；rt_memory_info返回total/used/max_used | maximum是本次运行累计分配峰值；单次记录不能判断泄漏，不能在多堆配置下假定覆盖全部内存 |
| `ps` | msh_cmd.c的cmd_ps调用list_thread；`components/finsh/cmd.c`扫描栈填充值，输出stack size、max used等 | max used是栈填充扫描得到的历史使用比例，不是CPU占用率；应结合栈大小和重复场景分析 |

当前生成配置为`RT_MAIN_THREAD_STACK_SIZE=2048`、`FINSH_THREAD_STACK_SIZE=4096`。参考工程主线程设8192；本项目尚无UI栈实测，不能据此直接认定2048足够或必须取8192。后续测量启动、对象创建、触摸操作及退出重入时的峰值，再定余量。

## 3. 最小资源取证流程

沿用当前串口，无需改固件。先等诊断命令结束、回到msh提示符，再逐条发送并保留完整输出：

```text
free
ps
```

第一轮记录当前RGB+IMU诊断固件的堆与线程状态，标明它是显示启用前基线，不是迷宫资源验收。之后可在一次imu_control_probe正常返回后再取同两条记录作对照；累计峰值不下降本身不等于泄漏。出现未知命令或格式不同就保留原输出，先核对实际运行固件。

当前构建报告ROM used 328098 B、RAM used 57128 B与main.bin 300980 B可追溯。随后取得诊断固件运行基线：系统堆total466100 B、used8980 B、maximum74252 B，当前空闲457120 B；tshell栈4096 B/max used22%，main栈2048 B/max used33%，tidle512 B/17%、timer1024 B/08%，四行error均000。这是一次诊断状态记录，不能判定泄漏或未来UI栈需求，具体见上述证据。

显示缓冲预算尚未定。若采用390×450、RGB565，单个未计对齐及额外区段的全屏像素数组算术大小为351000 B；SDK另有对齐、局部绘制缓冲、LCD缓冲及内存区段配置，不能把该算术值当作真实总分配或直接推定容量不足。

## 4. 下一阶段准入

本步完成接口静态核对及显示启用前的一次资源基线。后续形成架构、数据流和状态机设计，明确UI线程、IMU后台、快照所有权、取消/重试、运动时基和资源预算；显示启用后再实际验证分辨率/方向、触摸和内存变化。速度上限、响应和UI资源余量仍未定，候选300 raw死区、20点窗口、100ms时效不自动成为产品基线。所有正式产品TC保持未执行。

## 5. Task 1显示原型的配置决定

核对SDK HEAD仍为421126d9f476ed8e2a6f0b0ca28a9f241c182e65；external/lvgl_v9/lv_version.h为9.4.0，Kconfig首行v9.2.0只是不同的旧注释，不用于版本声明。

- 启用PKG_USING_LITTLEVGL2RTT、LVGL_V9，选RGB565、默认Montserrat 16字体。
- 首次构建暴露SDK app_mem.c依赖RT_USING_MEMHEAP；项目补启用该对象分配库。它与系统堆算法选择分开，未启用RT_USING_MEMHEAP_AS_HEAP，仍需核对生成配置确认系统堆保持small-memory。
- 选LCD_FB_USING_NONE与LV_FB_TWO_NOT_SCREEN_SIZE、LV_FB_LINE_NUM=40，使用CO5300自身显示存储并从局部缓冲直接刷新，不另设SDK LCD全屏缓冲。生成配置未启用DRV_EPIC_NEW_API，实际走lv_drivers_v9/lv_lcd.c，其模式为LV_DISPLAY_RENDER_MODE_PARTIAL，不能误用sifli_renderlist路径解释当前分配。
- 主线程栈首次设8192 B，为显示初始化/绘图留出试验空间；不是测得需求，待ps峰值决定后续余量。未复制参考工程优先级、蓝牙/网络等配置。
- 理论两块40行×390列×2 B=62400 B；最终静态大小/位置须核对map，运行堆与栈仍需free/ps。LVGL使用RT-Thread分配器，free可观察该系统堆的动态分配；静态绘图缓冲不计入used。
- 只在主线程创建页面和处理事件；输入经LVGL现有touch驱动读取，不新建触摸任务。仅按下/点击打印，空闲不刷RGB日志。

实际操作见[显示预研指导](../guides/display-probe.md)。此时没有真机显示/触摸结果，不把Task 1或正式TC标为通过。

修复依赖后完整构建通过，[构建及链接记录](../../evidence/builds/2026-10-17-display-probe-01/README.md)确认两块绘图缓冲各31200 B、共62400 B，位于SRAM的l1_non_ret_bss_frambuf；生成配置保持small-memory系统堆。main.bin559856 B，ELF text555644/data4208/bss91184 B；这些不代替运行堆栈测量。Task 1继续等待板端D-01至D-05证据。

## 6. 首次UI真机反馈与朝向冲突

用户提供[首次触摸/资源日志](../../evidence/display/2026-10-17-ui-01/README.md)：TL/TR/BL/BR与CENTER共7次点击，计数1–7连续；四角坐标大小关系符合LVGL坐标。堆total402912/used29700/maximum36648 B，当前空闲373212 B；main栈8192 B/报告峰值42%。仅一组free/ps，不能判定无泄漏，D-04与第二组资源仍缺。

用户明确报告正常显示朝向为USB在右，与旧IMU预研USB在左不同。原设计及页面USB LEFT标记是预设，不能当作硬件方向事实。用户后续同意采用USB右、同面朝上平面旋转180°将旧映射两轴取反，复用原始数据，不重采中立/四向实验日志；每轮仍按需求校准。后续小球接入时检查实际方向，不能将几何推导写成产品运动已实测。旧数据和原朝向保持原样。

追加资源复测已保存同目录repeat.log：CENTER再点10次至17，堆三字段与首组一致、线程仍6个、main峰值42%，用户确认灯已熄灭。满足本次显示/触摸原型进入后台采样阶段的初步准入，不当作正式TC-017/018或无泄漏证明。
