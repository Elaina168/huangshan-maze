# 第5阶段复杂机关实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans 原生执行本计划。步骤用复选框跟踪；用户已有本会话整阶段执行偏好，不重新选择执行方式。用户已确认计划，在本会话原生执行。

**Goal:** 新增正式第五关动力机关，完整接入磁力、定时危险/闸门、巡逻块和五关成绩存档。
**Architecture:** 只读动态对象表，model按统一有效游玩时钟推进力场与扫掠碰撞；app控制暂停/校准，UI消费同一快照，独立v4存档保留旧四关。
**Tech Stack:** 现有C99、SiFli SDK2.5.1、RT-Thread、LVGL9、FlashDB、Python与已有WSL主机编译器；不安装新依赖。
**Spec:** [已确认设计](../specs/2026-11-03-stage5-dynamic-mechanisms-design.md)

## Global Constraints

保留旧四关稳定ID、布局和记录；速度180、输入加速度600、正常/冰阻尼3/0.6及100ms有效期/时基限制不改。只有UI线程操作LVGL；保留显示完成等待、固定视口与单缓冲。每bank32768B、两bank共65536B、sector8192B不变；无格式化旧档、自动烧录、安装、SDK修改、提交推送。用户明确收工或要求提交时另执行。旧B/C不恢复，阶段6至8不实现。

## Review Focus

1. 相变/折返恰在步内和同刻危险：任务2、3验证最早事件和失败优先，不只测终点。
2. 静球被巡逻块推向坑/激光/墙：任务3验证推球位移扫掠和挤压一次结算。
3. 闸门待关状态跨暂停及复活：任务2、3验证时钟+待关掩码快照。
4. v3与v4同修订混合/最大revision/迁移中断：任务4比较规范快照，不覆盖未知或源bank。
5. 五关菜单拥挤和相位变化与滚动同时刷新：任务5用真实LVGL异步消费验证球完整、无源缓冲覆写，任务6核对实机资源边界。

## 文件与新接口约定

保留maze_level.h原基础机关类型；追加maze_dynamic_kind_t（MAZE_DYNAMIC_MAGNET/LASER/GATE/PATROL），maze_dynamic_t只读记录：kind、area、end（巡逻终点左上角）、force、outer_radius、inner_radius、speed、off_ms、warning_ms、on_ms、dwell_ms、phase_ms。无关字段为0。磁心为area中心，激光为非零宽高的细矩形，其他为实体矩形。

maze_level_t与maze_map_definition_t末尾追加const maze_dynamic_t *dynamics、unsigned dynamic_count；旧静态初始化补0，旧图仍走无动态路径。MAZE_MAP_MAX_DYNAMICS=8集中maze_config.h。新字段为本轮命名决策，不冒充现有接口。

maze_model_t追加uint64_t dynamic_us、checkpoint_dynamic_us及uint32_t gate_pending、checkpoint_gate_pending；10ms常规步，步内边界按微秒划分。追加maze_loss_reason_t（TRAP/LASER/CRUSH）及loss_reason，旧陷阱显示不变。巡逻位置由时钟与只读定义求值，无每帧累加漂移；gate_pending记录当前关闭窗口占位延迟状态。

model对UI提供maze_dynamic_view_t {maze_rect_t area; unsigned phase; bool blocked;}与maze_model_dynamic_view(const maze_model_t*,unsigned,maze_dynamic_view_t*)；phase含关闭/预警/开启，含义按kind解释并在头文件注释。读取函数不推进时钟。新增src/maze_dynamic.c/.h仅承载共享的只读相位/位置/磁力计算；碰撞留在maze_model.c，避免两个模块重复求解接触顺序。必须在所有实际编译入口加入该源码，不能仅改测试。

## Task 1：定义、磁力与动态地图输入边界

**Files:** src/maze_level.h、maze_model.h、maze_config.h、maze_map.h、maze_map.c；新增src/maze_dynamic.c/.h、tests/maze/dynamic_harness.c；scripts/test_maze.py及实际构建源选择。

**Interfaces:** 消费既有maze_model_step；提供上述只读类型/快照与磁力求值。最大8对象；磁力半径70/12、峰值240，两段线性公式与设计一致。

- [x] 写动态对象输入/磁力行为用例：中心0、r=12峰值、r=70为0、边界连续、区外不施力、静球起动、反向满幅可脱离、合力限幅、原四关同输入轨迹不变。
- [x] 先运行主机新用例并保留预期RED；随后以最小实现接入输入加速度/阻尼前施力，停止阈值不得吞磁力。
- [x] 实现map外部数据检查：有限数值、类型、数量、周期和相位；巡逻端点/扫掠体在界内，不穿墙/门/其他巡逻路径及起点/检查点安全区。激光可跨通道但端点和线宽须有效。
- [x] 运行新用例及原model/app/map回归GREEN，记录日志。磁区不是墙，静态BFS对动态物明确采用开放几何而非可解证明。

## Task 2：定时激光与安全闸门

**Files:** src/maze_dynamic.c/.h、maze_model.c/.h；tests/maze/dynamic_harness.c。
**Interfaces:** 模型时钟和只读动态视图；激光3600ms周期（off1800/warn600/on1200），门4800ms（open2400/warn600/closed1800）。激光矩形与球半径共同参与碰撞，显示线宽必须对应定义。

- [x] 先写边界前后1微秒、初相位、跨周期、高速过线、开启时球重叠及步内相变危险测试，观察RED。
- [x] 按下一个相位边界拆分模型时间，所有球路径使用最早碰撞/危险；激光有效时含圆形边缘接触失败，和陷阱同刻优先于物品/终点。
- [x] 写门占位关门、球静止等待、离开后关闭、关闭窗口结束清待关状态测试RED；实现延迟关闭及普通墙阻挡语义。
- [x] 验证步内结束/失败时实际消耗时间，不以整个10ms覆盖早结束；任务1与原模型回归GREEN。

## Task 3：巡逻相对碰撞与生命周期

**Files:** src/maze_model.c/.h、maze_app.c（仅必要接口调用）、tests/maze/dynamic_harness.c及state/app相关用例。
**Interfaces:** 巡逻35像素/秒、端点停400ms；model_loss_reason与checkpoint时间/门状态快照。

- [x] 写相对扫掠RED：静球被推、双方相向/同向、端点折返、切向滑动、推球跨陷阱/激光、推到墙角挤压一次失败。
- [x] 实现移动矩形相对运动碰撞，保持切向速度，推球位移仍进入同一危险/触发求解路径；块位置由时钟算出，避免绘图与碰撞两套坐标。
- [x] 写生命周期RED：暂停10秒机关不动、校准/错误/胜败冻结、继续不补时；重玩初相位，检查点恢复时钟/待关mask/基础mask，失误累计一次且不重复累计用时。
- [x] 以相同输入20/40Hz调用对比机关时钟和轨迹；执行旧代次/恢复/主循环相关测试GREEN。保留无动态对象的旧路径结果。

## Task 4：五关存档与真实库门槛

**Files:** src/maze_archive.c/.h、maze_store.c/.h、maze_progress.c（仅硬编码数量处）、tests/maze/archive*_harness.c、tests/maze/store-host/nor_harness.c、scripts/test_maze_store.py；新增tests/maze/fixtures/archive-v3.bin及说明。
**Interfaces:** MAZE_ARCHIVE_V3_BYTES=681、MAZE_ARCHIVE_BYTES=845、MAZE_ARCHIVE_LEVELS=5；v4唯一写，v1/v2/v3可读；新增ID dynamic_trial。

- [x] 在改旧编码器前生成合成v3 golden，独立核对原四关字段/CRC，夹具仅合成数据；如.bin被忽略，仅给该精确路径例外。
- [x] 写v1/v2/v3迁移到五关的RED：原字段保持、第五解锁由第四completed决定、同修订混合一致/冲突、未知保护、最大revision与失败重试不重复记分。
- [x] 更新codec与store的版本/长度/legacy判断，保留A/B和安全菜单迁移一次修订，执行逻辑及实际main替身回归GREEN。
- [x] 用实际SDK FlashDB/NOR执行旧版本迁移、更新、GC、初建各写擦边界进程中断；完整旧/新恢复且源bank不变，记录实际次数。不把受控进程退出当物理掉电。

## Task 5：第五关、UI与可玩见证

**Files:** 新增src/maze_map_dynamic_trial.c、tests/maze/stage5_route_harness.c、tests/maze/stage5_ui_harness.c；修改src/maze_catalog.c、maze_map.h、maze_ui.c、main.c及字体来源/生成结果；脚本和目标源选择按现有方式接入。
**Interfaces:** dynamic_trial第五稳定ID、标题动力机关；第四关任一模式完成解锁第五，原末关逻辑改由目录数量决定。

- [x] 生成设计JSON/全图预览，先自查必经机关、安全等候区/检查点、分岔/绕坑与球半径余量；依据用户已确认的整阶段方向设计具体布局，不伪造用户逐格审批。
- [x] 写地图/导航/五关成绩末关边界用例RED；接入新图与动态表。真实模型输入轨迹必须等候并通过所有必经机关、复活后可继续通关；证明静态BFS开放路线不足以替代动态见证。
- [x] UI读取同一model状态创建/更新动态对象，合并旧新失效区，展示预警/开启/吸附及激光/挤压失败原因；五入口在390×450屏幕可读且可触摸，不挤出按钮。
- [x] 真实LVGL验证五关菜单、两模式、对象相位、相机四边/折返、失败复活与文字边界；异步源消费验证球完整、无零色黑洞或源覆写；新增中文字形可再现。
- [x] 更新版本标识及使用指南、需求REQ-045至049/验收TC-065至070和相关计划风险；原四关/排行榜/字体与同步异步回归GREEN。

## Task 6：整合、复核与正式构建交接

**Files:** docs/test-report.md、project-plan.md、risks.md、roadmap.md、devlog/2026-11-03.md；新增docs/guides/stage5-use.md与evidence下本轮真实输出。

- [x] 运行完整逻辑、actual-main、实际SDK存储、真实LVGL同步/异步、字体、分区与6项快照测试，按实际现有脚本参数执行，失败修复只回归受影响范围。
- [x] 一次fresh-context整体复核，聚焦前述5类风险；修复重要问题并保留失败与回归，不追求无关重构。
- [x] python scripts/check_project.py及git diff --check通过；在已激活SDK终端执行python scripts/build_firmware.py并核对完整退出、source_unchanged及源码/三产物SHA。
- [x] 核对目标ELF已含正式第五图、无试验runner、原分区及下载范围不变；重新计算主线程嵌套栈和静态预算，不拿地址尾区或主机堆当实际free。
- [x] 交接一次新增板端阶段测试：机关/暂停/复活、五关导航、旧档升级/重启、实屏滚动、free/ps及服务间隔；未收到数据保持待证，不自动烧录或推送。

## 执行与验证记录

本计划覆盖设计全部行为。功能实现前先新测试RED再最小实现GREEN；具体主机命令复用现有WSL入口，不安装编译器。计划确认后已原生连续执行，任务间报告结果，未逐任务请求许可；最终状态以本节后续记录与实际证据为准。


2026-11-03：以上软件/复核/正式构建/板端指南交接步骤已执行；TC-070真实板端待用户，并非勾选即硬件通过。实现裁量：只读动态求值保留model内部，无独立maze_dynamic.c/.h，外部view接口不变；旧聚合尾字段按C保证归零、主机只抑制该特定警告。档案门槛、原图/动态回归、真实LVGL与目标构建证据均见本轮报告；未提交推送。
