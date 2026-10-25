# 体感迷宫实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task if the user selects native execution; use superpowers:subagent-driven-development only if the user explicitly selects subagent execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在真实黄山派上完成单关卡体感迷宫闭环，保留自动测试、构建和板端证据。

**Architecture:** UI主线程拥有状态、模型和LVGL对象，一个后台线程读取IMU并发布受保护快照。关卡只读数据独立于规则；代次隔离迟到结果；物理与显示时基分离。

**Tech Stack:** SiFli SDK v2.5.1、RT-Thread、LVGL v9、C99；复用现有Python与WSL C编译工具，不安装依赖。

**Spec:** [已认可的架构设计](../specs/2026-10-17-maze-architecture-design.md)。用户已于2026-10-17选择本会话逐步实现；Task 1初步显示/触摸与资源准入已有反馈，Task 2进行中，后续任务未执行。

## Global Constraints

- 板型 `sf32lb52-lchspi-ulp`，屏幕朝上、USB在右侧（2026-10-17用户确认）；正式REQ/TC依照现有PRD和验收计划。
- 只有UI线程操作LVGL；禁止持共享锁执行I2C、等待、打印、LVGL。
- 只实现一个固定关卡；不做选关、存档、网络、音效、编辑器或额外硬件功能。
- IMU候选：104Hz、约50ms等待、20点窗口/至少950ms、三轴极差≤300 raw、3000ms逻辑预算、300 raw死区、100ms最大年龄及间隔。
- 新运动候选：固定步10ms、合成速度上限120px/s、死区后8192 raw达到上限、超长调度间隔阈值100ms、UI模型刷新间隔50ms；候选经显示负载复测后确认。
- SDK API和配置从实际版本读取；中文路径通过 `scripts/build_firmware.py` 创建ASCII快照构建。
- 新文本UTF-8，代码注释中文；不修改SDK，不覆盖其他工程，不把主机注入或构建结果当硬件验收。
- 每任务只完成必要验证并本地保存；用户明确收工后统一审查、暂存、提交、推送，遵守提交钩子与正文格式。

## Review Focus

1. 退出后旧I2C读取才返回：必须丢弃旧代次，不更新旧页面或下一轮；任务2/5覆盖。
2. 球恰在圆角接触点且沿切线运动：不能卡住或漏进墙；任务4覆盖。
3. 一步穿越陷阱后终点：必须结算首次相交区域，不能只检测最终球心；任务4覆盖。
4. 暂停重校准失败后继续：保留旧参考与用时，继续还须有有效采样；任务5覆盖。
5. 时钟回绕、重复序号、UI停顿：不得生成巨大位移、重复校准点或补算暂停时间；任务3/5覆盖。

## 文件与依赖

产品文件按设计使用 `src/maze_ui.c/.h`、`src/maze_imu.c/.h`、`src/maze_app.c/.h`、`src/maze_model.c/.h`、`src/maze_level.c/.h`。共享平台无关数据类型放各模块所属头文件，不新增通用工具层。

新增 `tests/maze/harness.c`、`tests/maze/imu_harness.c` 及采样测试所需最小RT-Thread/I2C桩；`scripts/test_maze.py` 编译运行它们，沿用现有 `scripts/test_imu_probe.py` 的临时目录、严格编译告警与失败即退出方式。所有下列接口均为本计划的新提议，不冒称已有SDK接口。

主机测试约定：`wsl --exec python3 '/mnt/d/codex/黄山派/scripts/test_maze.py'`，输出各场景PASS且退出码0。材料检查使用既有Python运行 `scripts/check_project.py --history`，差异检查 `git diff --check`。板级改动额外激活现有SDK环境并运行 `scripts/build_firmware.py`，完整成功结束才记构建通过。

## Task 1：最小显示、触摸与资源原型

**Files:** 修改 `src/main.c`、`project/proj.conf`；新建 `src/maze_ui.c/.h`、`docs/guides/display-probe.md`；更新 `docs/research/display-resource-audit.md`、当日开发日志。

**Interfaces:** 产生 `int maze_ui_init(void)`、`void maze_ui_process(void)`；初始化返回0表示成功，错误保留SDK实际返回值；process仅UI线程调用，服务LVGL与本阶段触摸显示。后续任务6把它扩展为完整视图，不新增第二套UI。

- [x] 核对SDK已登记的 `littlevgl2rtt_init("lcd")`、`lv_timer_handler()` 及实际LVGL v9对象/事件API，核对缓冲和线程栈配置来源；记录所选值与理由，不复制全部外设工程配置。
- [x] 实现四角不同标记、中心触摸按钮、坐标/点击计数反馈，主线程持续服务LVGL；用中文指导说明可见方向、四角及中心测试。替换RGB阻塞循环时关灯并记录变化，保留历史个人修改证据。
- [x] 材料检查与ASCII快照固件构建通过，记录源码及固件哈希、ROM/RAM、真实缓冲模式和内存位置；不为静态页面写无意义主机测试。首次缺memheap失败及补配置后的成功均保留在[构建记录](../../../evidence/builds/2026-10-17-display-probe-01/README.md)。
- [x] 用户烧录后逐点验证物理方向、四角/中心触摸、按钮反馈，查询 `free`、`ps`；[聊天证据及复测](../../../evidence/display/2026-10-17-ui-01/README.md)已归档，17次点击、两组相同堆字段及关灯反馈；没有独立计时或设备哈希证据。
- [x] **准入门槛：** 用户确认USB右时正向显示，四角/中心触摸对应，有真实堆栈记录，可以进入下一步；不据此宣称长时稳定或正式产品TC通过。

## Task 2：IMU后台与异步生命周期

**Files:** 新建 `src/maze_imu.c/.h`、`tests/maze/imu_harness.c`、最小平台桩和 `scripts/test_maze.py`；修改 `src/imu_probe.c`、`src/main.c`。

**Interfaces:** `maze_imu_sample_t` 含 `uint32_t generation, sequence, read_start_tick; int16_t raw[3]; int error;`，时间字段保留RT tick单位。产生 `int maze_imu_init(void)`、`void maze_imu_request(uint32_t generation, bool enabled)`、`bool maze_imu_latest(maze_imu_sample_t *out)`。latest复制完整快照，无已发布快照时返回false；UI自己核对代次/序号。

- [x] 写桩测试并运行确认缺实现时失败：读取期间切代次、停止后不追加采样、快照字段完整/副本隔离已在真实线程入口验证；用例名对应late_read、stop_during_read、normal，并补停止状态读取/失败/恢复及积压唤醒，共11场景。
- [x] 实现单个常驻线程、锁保护请求与快照、锁外I2C。沿用已核对寄存器和错误返回；记录初始化、读失败、停用与恢复错误，不吞错。失败快照与成功快照均有代次和序号。
- [x] 产品启动后禁用现有 `imu_probe`、`imu_control_probe` 对硬件的直接访问，返回忙及product sampler owns IMU；保留诊断逻辑供原主机测试使用。测试断言被拒绝调用不发生I2C读写。停用不反复创建/销毁线程，驱动阻塞返回后才处理最新请求。
- [ ] 主机测试通过；执行原 `scripts/test_imu_probe.py` 回归；构建通过。在任务1界面运行采样开/关并保留UI仍可触摸和后台错误日志；无实际故障时不写硬件故障通过。

上述最后一步的自动验证部分已通过，见[构建/回归记录](../../../evidence/builds/2026-10-17-imu-background-01/README.md)；板端按[新指导](../../guides/imu-background-probe.md)等待用户操作，不标整个Task 2完成。

Task 2接入细化：补充 `bool maze_imu_owned(void)` 供旧诊断命令拒绝并发访问。后台保存/配置/恢复寄存器，UI请求不读I2C；后台静态2048 B栈、优先级11为试验值。中央按钮投递切换事件，主循环经新增 `bool maze_ui_take_sample_toggle(void)` 取走后更新代次并启停，通过 `void maze_ui_show_sampling(bool enabled)` 显示状态；每500ms最多打印一条当前代次raw快照。本阶段不校准、不输出控制方向；读错停止会话，界面显示OFF，下一次开启重试；恢复写错单独报告并保留原代次。一个代次只开始一次会话，重复请求或积压唤醒不重复尝试。

## Task 3：校准、输入有效性与映射

**Files:** 新建 `src/maze_app.c/.h`；扩展 `tests/maze/harness.c`、`scripts/test_maze.py`。

**Interfaces:** 在app头定义 `maze_input_t { float x, y; }`（死区后raw输入）；定义 `maze_app_t` 保存参考、窗口与当前代次。产生 `void maze_app_init(maze_app_t *app, const maze_level_t *level, uint32_t tick_hz)`、`void maze_app_sample(maze_app_t *app, const maze_imu_sample_t *sample, uint32_t now_tick)`、`void maze_app_update(maze_app_t *app, uint32_t now_tick)`；本任务update处理校准超时，任务5扩展为完整状态更新。关卡类型由任务4定义，任务3先前置声明，不解引用。app实现保持平台无关，不直接调用I2C/LVGL。

- [x] 写断言并观察缺模块时失败：19点不能成功；20点间隔50ms、跨度950ms且各轴极差300成功，极差301不成功；序号重复不增加窗口；年龄/间隔100ms允许、101ms无效；无样本到3000ms校准失败。
- [x] 补充输入断言：参考 `(0,0,0)` 时rawY=-301得到x=-1、rawX=-301得到y=1；绝对差300得到0。用靠近 `UINT32_MAX` 的tick开始同一轨迹，结果与非回绕轨迹一致。
- [x] 实现有限20点窗口、均值参考、映射与时效；不同代次不混入窗口，缺失序列/超限间隔重启窗口，UI超时独立于I2C返回。只在成功时提交新参考。
- [x] 主机测试通过。候选数值保留在app头的集中常量中，后续手感调整只改该处并更新参数记录；不自动平滑或漂移参考。

自动验证为后台11场景+输入8组、原探针回归、快照4/4及完整构建。阶段性UI已接入，[固件与证据](../../../evidence/builds/2026-10-17-imu-input-01/README.md)可追溯；[一轮板端反馈](../../../evidence/imu/2026-10-17-input-01/README.md)支持本次校准、xy变化及取消基本检查。没有物理方向标注或独立计时，不据此冻结参数或标正式产品TC通过。

Task 3阶段接入决定：新增maze_app_begin(app, now_tick)、maze_app_cancel(app)作为当前校准预研入口，后续Task 5事件接口复用；状态为准备/校准/输入就绪/校准失败/输入错误。中央按钮在准备或失败时启动新代次校准，在校准/就绪时取消。UI显示窗口进度、校准结果和经死区的x/y，每500ms限频输出；无迷宫或运动。时间按tick_hz换算，重复序号不重复计点；校准时序号跳跃或读取间隔>100ms清窗口后重新收集，过期样本丢弃并清窗口，3秒预算不延长；读取错误立即校准失败。就绪后过期或间隔超限转输入错误并清输入。稳定窗口三轴极差≤300、20点跨度≥950ms且在3000ms截止前才提交参考；超时不改旧参考。用户已确认USB右映射为rawY-referenceY及referenceX-rawX。本轮候选仍未做正式性能验收。

## Task 4：关卡模型、运动与连续碰撞

**Files:** 新建 `src/maze_level.c/.h`、`src/maze_model.c/.h`；扩展 `tests/maze/harness.c`。

**Interfaces:** `maze_level_t` 包含宽高、球半径、起点、墙数组/数量、陷阱圆数组/数量、终点圆；只读实例 `extern const maze_level_t maze_level_one`。产生 `void maze_model_reset(maze_model_t *model, const maze_level_t *level)`、`void maze_model_step(maze_model_t *model, float input_x, float input_y, uint32_t dt_ms)`；输入为app输出的死区后raw值，model不反向依赖app头文件。model含球心、速度、结果与本次到达事件所用毫秒数，供app准确冻结用时。结果枚举为运行/胜利/失败；不在model内调用系统时钟。

- [x] 地图尺寸使用任务1实测确认的区域；写地图常量及主机测试：起点可容纳球，圆区互斥，全部几何在边界内；给出带球半径余量且避开陷阱的路径作为通路见证并逐段断言可达。实际坐标在任务1结束后记录进设计附录，本计划不猜物理方向。
- [x] 写并运行失败测试：120px/s下空旷区运动1000ms位移120px；对角合成速度不超过120；8192 raw饱和；固定输入运行10秒，仅渲染20/40Hz最终每轴差≤1px。
- [x] 写几何断言：墙x=50、球半径5，从x=10向右跨墙，球心最大45；接触后切向移动可继续；圆角切线、两墙夹角、薄墙大位移、外边界均不穿透。陷阱圆心(30,50)/半径3、终点(60,50)/半径3，球心从(10,50)向(80,50)扫掠必须先失败；边界包含，圆外不结算。
- [x] 实现比例速度、合成限幅及按设计的圆角扫掠/最早事件；最多4次接触后停在安全点；禁止普通方形膨胀代替圆角。胜败后再次step不改坐标或结果。主机测试全部通过，严格编译告警为零。

2026-10-25完成Task 4模型增量，地图坐标见设计附录A，[主机验证](../../../evidence/model/2026-10-25-model-01/README.md)及[完整构建](../../../evidence/builds/2026-10-25-model-01/README.md)已保存。20/40Hz为调用间隔对照，固定步/实际显示对照属后续app/UI任务，正式TC-022未执行。模型尚未接入app，当前板端仍为输入预研页。

## Task 5：状态、计时与取消恢复

**Files:** 扩展 `src/maze_app.c/.h`、`tests/maze/harness.c`。

**Interfaces:** 定义 `maze_event_t` 的开始、暂停、继续、重校准、重玩、重试、退出/取消事件；产生 `void maze_app_event(maze_app_t *app, maze_event_t event, uint32_t now_tick)`、`void maze_app_update(maze_app_t *app, uint32_t now_tick)`、`void maze_app_request(const maze_app_t *app, uint32_t *generation, bool *enabled)`。UI以只读app状态渲染；模型步长累计余量仅app管理。

- [x] 写状态测试并确认失败：开始校准成功才计时；暂停10秒位置/用时不变；继续不补算；重玩清空并增加代次；不适用事件不改变状态；胜败只结算一次。
- [x] 覆盖三种校准来源：暂停校准失败保留参考与位置/用时，成功仍暂停；设备恢复成功仍暂停，失败回错误；继续时采样过期转错误。退出后注入旧代次成功/失败样本，准备态及下一轮均不被改变。
- [x] 覆盖时基边界：100ms调度间隔拆为10个10ms步，101ms记录时序错误并冻结；不足10ms保留余量；tick回绕仍正确；重校准、暂停、退出清除运动累计余量，暂停前5ms不得串到新轮。
- [x] 实现状态表、单调tick差、有效用时累计与新请求生成。计时按实际游玩时间，区域中途结束使用model返回的事件时间；错误停顿不补算，日志包含状态、代次、原因。主机全部场景通过。

2026-10-25完成Task 5：新增state_harness的7组联合断言，原回归通过，[状态证据](../../../evidence/state/2026-10-25-state-01/README.md)与[完整构建](../../../evidence/builds/2026-10-25-state-01/README.md)已保存。当前0.3.0-state-core仅适配预研页，尚未接完整地图/按钮，不要求中间版本烧录，正式TC仍未执行。

## Task 6：接入迷宫UI与板端闭环

**Files:** 扩展 `src/maze_ui.c/.h`、`src/main.c`；更新 `docs/guides/maze-use.md`、`THIRD_PARTY.md`、开发日志；新建相应构建/板端证据。

**Interfaces:** 保留task1入口；UI持有一个app实例，通过task2采样副本与task5请求接口交互。按钮回调记录单个待处理事件，本次循环处理后清除；状态切换后丢弃旧页事件，重复不适用事件由app忽略。不创建另一个游戏线程。

- [x] 先执行所有主机测试；确认失败场景已经覆盖再接UI。按准备、校准、暂停、结果、错误展示相应入口；球/墙/陷阱/终点读取同一关卡几何，圆形样式无可见危险区偏差。
- [x] 主循环依次处理事件、复制快照、更新app、发布请求、刷新界面；持续调用LVGL服务。状态切换才创建/销毁页面对象，游玩只更新球和用时；绘图路径不反复分配地图数据。
- [ ] SDK构建、快照测试、材料检查通过；用户按当前实际串口烧录，USB在右侧逐向验证屏幕运动、墙边滑动、终点胜利、陷阱失败、暂停、重玩和退出。分场景保存视频/日志与固件哈希，任何未测项保持未执行。
- [ ] 显示负载下复测校准/时效、堆和两个应用线程栈峰值；与任务1及显示前基线分开报告。出现故障只做相关回归，不借机新增功能。

2026-10-25代码集成及自动部分通过：0.4.0-maze-ui两轮构建（第二轮修正错误提示）、后台11/校准8/模型7/状态7、旧探针7+14/占用守卫、UI事件3及快照4/4；独立代码审查未发现Critical/Important项。[构建证据](../../../evidence/builds/2026-10-25-maze-ui-01/README.md)、[自动验证](../../../evidence/ui/2026-10-25-maze-ui-01/README.md)及[使用指导](../../guides/maze-use.md)已保存。板端四向/胜败/操作/资源尚未执行，因此保留后两项未勾选，Task 6进行中，不进入参数冻结或正式TC通过声明。

## Task 7：参数确认与正式验收

Task 6当前补充（2026-10-25）：0.4.2活动视图已构建/回归；用户日志记录Retry校准成功、胜利、Replay校准后陷阱失败、Exit回准备，本段无-1002/-1004。照片与官方标注澄清常亮绿灯实际为供电指示灯，RGB照片中未发光；不再以该绿灯作为RGB关闭失败证据。当前仍需暂停/继续、暂停重校准、四向/墙边可见验证及本版free/ps，未勾选Task 6整体通过。现用固件即可，下一组步骤及证据边界见[测试记录](../../test-report.md)。

**Files:** 更新 `docs/product-requirements.md`、`docs/acceptance-test-plan.md`、`docs/risks.md`、`docs/project-plan.md`；新增 `docs/test-report.md`、`evidence/acceptance/`实际记录与参数说明。

- [ ] 按相同固件/姿态记录静置5秒、四向可见响应、触摸反馈、显示时序及速度手感；有动作起点和屏幕证据才测物理响应。向用户展示候选参数表现，确认后更新参数基线；未达到目标不能事后无说明放宽。
- [ ] 按现有TC-001至TC-022逐条执行并填实际结果，失败保留原证据；异常注入与真实I/O分列。计时对照60秒、显示20/40Hz对照，记录分辨率、误差和样本量。
- [ ] 完成50轮及至少30分钟测试，每10轮同准备态取资源；记录对象、线程、堆栈变化，不从单次free判断无泄漏。
- [ ] 运行全部相关自动检查并完成针对最终差异的代码审查；必要修复后仅回归受影响项。准出以正式必达需求证据齐全为准，未执行项不得填通过。
- [ ] 本地汇总已知问题、复现方法、AI协作与人工操作。用户明确收工时才审查全部待提交文件和敏感信息，按钩子提交并推送；最终tag、交付固件及ZIP另在真正封版时执行，不在当前开发阶段伪造交付完成。

## 执行交接

建议本会话原生执行：由当前助手按顺序实施，每阶段验证后说明结果，遇到烧录/触摸等真实板端操作交给用户。任务1硬件门槛未通过前，不继续假设布局或资源已足够。

另一选项是用户明确选择后使用子代理逐任务实现与复核。无论采用哪种方式，本计划及执行方式需先经用户确认；此时不写产品代码，不创建分支或工作树，不提交推送。
