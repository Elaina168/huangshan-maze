# 三关正式接入实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将已确认冰环迷宫接入正式三关版本，复用滚动显示，安全升级既有两关存档。

**Architecture:** 先完成支持v1读取/v2输出的纯C档案，再验证真实FlashDB上的A/B迁移；随后接入地图、业务与统一视口。显示提交回调只推进采样/导航/物理，持久化始终留在外层安全状态。

**Tech Stack:** 现有SiFli SDK v2.5.1、RT-Thread、LVGL v9、FlashDB/FAL、C99/Python/WSL GCC，不安装依赖。

**Spec:** [已认可设计](../specs/2026-11-02-third-level-design.md)，布局以[已确认JSON](../../design/maze-layout-2026-11-02-ice-loops/layout.json)为准。沿用用户已选择的本会话逐步原生实施；本计划待审阅后执行。

## Global Constraints

- 三个稳定ID为frost_fork、ice_bends、ice_loops；前两关几何/物理/成绩规则不变。
- v1为353B/两关/format1，v2为517B/三关/format2，magic保持HMZ1、rules保持1；只写v2。
- 两份32KiB数据库、8KiB擦除、物理区0x12DB0000..0x12DC0000及代理只读/逐bank写闸门保持；不改分区或默认三文件烧录地址。
- 第三关严格使用700×600、141墙、6冰格、5必经通道坑的确认布局；不重新搜索布局、不增加机关/地面/随机地图。
- 350×300视口、234000B单缓冲/64B对齐、原运动与100ms限制保持；只有UI线程调用LVGL。
- 旧成绩保留；同revision语义冲突/未知版本不覆盖；升级后旧正式固件无法读v2，必须告知用户。
- UTF-8与中文注释；复用当前codex/maze-ui工作目录，保留course-requirements.md既存差异；不自动烧录、提交或推送，不恢复旧B/C。

## Review Focus

1. v1/v2字节长度不同但同revision时不能用原始缓冲memcmp误判，也不能忽略第三关冲突（Task 1/2）。
2. 成功写入后读回失败、重启/手动重试不能重复提高迁移revision或重结算（Task 2/4）。
3. 显示期间进入胜败/错误，旧点击必须失效，结果只观察一次，回调不能触发Flash（Task 3/4）。
4. 单缓冲/141墙/正式存档并存的RAM、栈和单块耗时不能沿用试验数字（Task 4/5）。
5. 旧档仅解锁第二关不应解锁第三关，任一模式已通第二关才解锁；旧版本降级不能静默清档（Task 1/2/3/5）。

## 文件与接口约定

- src/maze_archive.h/.c：原`maze_archive_encode(const maze_archive_t *,uint8_t out[MAZE_ARCHIVE_BYTES])`、`maze_archive_decode(const uint8_t *,size_t,maze_archive_t *)`签名保持；MAZE_ARCHIVE_BYTES改517、MAZE_ARCHIVE_LEVELS改3，新常量MAZE_ARCHIVE_V1_BYTES=353。decode接受两种外部格式，encode只输出v2。
- src/maze_store.h/.c：原open/save接口保持；新增`bool upgrade_pending`表示选中v1尚待提交。`current`保存选中档案的规范v2字节、revision保存实际已提交值；私有read_archive携带源格式信息。
- src/maze_map_ice_loops.c：新导出`const maze_map_definition_t maze_map_ice_loops`；src/maze_map.h声明，src/maze_catalog.c追加第三项。
- src/maze_config.h：正式上限统一14×12；将MAZE_TRIAL_DRAW_COLUMNS/ROWS移为共用MAZE_DRAW_COLUMNS/ROWS，值仍390/300。
- src/maze_ui.h/.c：`maze_ui_set_trial_service`转为共用`void maze_ui_set_service(void (*service)(void *),void *context)`；新增`void maze_ui_loading(void)`用于载入反馈。正式/独立试验都复用相机、场景与合并球失效区；试验render/event适配仍由MAZE_SCROLL_TRIAL隔离。
- src/maze_navigation.c、src/maze_progress.c、src/main.c：现有接口保持，三关业务/共享解锁/回调服务/启动迁移适配；不新增一次性运行时框架。
- tests/maze、scripts/test_maze.py、scripts/test_maze_store.py、tests/maze/lvgl-host/CMakeLists.txt：新增迁移/第三关/正式显示回归；旧用例随新边界更新，不能简单删除失败断言。

### Task 1：双版本档案与第三关RAM扩展（REQ-039 / TC-055）

**Files:** 修改src/maze_archive.h/.c、tests/maze/archive_harness.c、scripts/test_maze.py；新增tests/maze/archive_migration_harness.c、tests/maze/fixtures/archive-v1.bin及其说明。

**Interfaces:** 消费原CRC/编解码接口；输出三关maze_archive_t，decode(v1)保留源revision，填入空第三关并由第二关completed决定unlocked；encode只写517B v2。

- [x] 先建立独立的旧353B golden fixture，记录来源为原v1字节格式与合成测试分数，不使用新编码器伪造旧档；新测试断言v2头version2/length517/count3、CRC，先运行见RED。
- [x] 验证v1第一/第二关两模式全字段、同分稳定顺序、失误和解锁保持；未通第二时第三锁定，completed=1/2/3均开放第三，空榜/失误0。覆盖旧ID顺序交换及v2三ID排序/重复/缺失/未知ID边界。
- [x] 实现两个确定长度分支及共同字段读取，发布输出只在完全成功后；保持外部格式/CRC/未知规则明确错误，不补造缺失老记录。
- [x] 验证乱长度、截断、超长、CRC错、未来format3/未来rules2、越界计数/排序；更新原archive_harness的v2预期，保留v1专门覆盖。
- [x] 在scripts/test_maze.py显式编译并运行原档案与新增迁移harness（目前原档案harness未由该脚本自动调用）；局部GREEN后进入Task 2，不能据此声称整个固件完成。

### Task 2：A/B选择、迁移事务与真实库中断门槛（REQ-039 / TC-055至057）

**Files:** 修改src/maze_store.h/.c、tests/maze/store-host/nor_harness.c、scripts/test_maze_store.py；复用原代理与8KiB NOR模型，不编辑SDK。

**Interfaces:** 消费Task 1编解码；open/save签名不变。选中v1时open在只读探测完成后返回RAM revision=n+1，store.revision/current仍代表已提交n，upgrade_pending=true；成功save后清flag。此为设计“一次打开提升一次”的工程落实，无Flash读时隐式写入。

- [x] 先增加真实库生成的v1 FlashDB镜像模式、v1/v2混合及future-version模式，运行见RED；不能只把v2头改成1冒充有效v1。
- [x] read_archive按实际blob.saved.len处理353/517，读取上限517并先识别未知版本/规则；归位后规范encode用于候选比对。按revision排序；同revision规范一致时优先v2以免不必要迁移，不一致返回MAZE_STORE_CONFLICT。
- [x] open选中v1时先保留旧revision及规范current，再在返回RAM档案中提升一次；UINT64_MAX无法提升时返回MAZE_STORE_STALE且底层无写擦。选中v2不提升。全部空白新建默认v2，非空未知不清除。
- [x] save仍仅改非active bank，读回字节一致才切active/清upgrade_pending；写入或读回失败保留RAM、原已提交修订和源bank。重试同一RAM对象不额外递增。
- [x] 测试普通v2保存、v1/v2同修订一致/冲突、较高修订优先、未来版本在任一bank时全只读、实际blob长度与未初始化尾字节；更新原未来版本测试为3，不能将已支持的2继续当未来格式。
- [x] NOR harness除STATE外输出实际选中blob RAW、COMMITTED修订和UPGRADE标志，防止RAM规范化掩盖磁盘仍为旧/新格式的差异。源bank原字节逐次比较。
- [x] 对迁移/普通更新/实际GC各写擦操作，按0、1、中间、末前、完整前缀强制_exit(77)，再启动验证完整旧v1或新v2及源bank不变；首次建档边界保持。操作次数从当前实际调用采集，不能引用旧215次当本版结果。
- [x] 验证写完但读回失败后的同修订重试、迁移失败重启再开、同进程反复保存；两个逻辑状态都不能重复记成绩/失误。GREEN门槛：`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze_store.py --sdk /mnt/d/sifli/SiFli-SDK/v2.5.1`完整成功。失败则先修复，不用后续UI掩盖存储问题。

### Task 3：固定第三图与三关业务（REQ-038 / TC-053/054/057）

**Files:** 新建src/maze_map_ice_loops.c、tests/maze/third_map_harness.c、tests/maze/third_fixture.h；修改src/maze_map.h、maze_config.h、maze_catalog.c、maze_progress.c、相关navigation/progress/map_load harness及scripts/test_maze.py。

**Interfaces:** 生产导出maze_map_ice_loops并追加目录；消费三关档案结构和原导航接口，前两关源数据不变。

- [x] 先写数据/导航失败测试：目录第3项ice_loops、锁关拒绝、第二关任一模式完成解锁、第三关末端无next，旧点击/样本与切关重置保持；RED后接入数据。
- [x] 从确认layout.json固化字符串/对象，不重新生成地图；正式容量14×12、工作区36256B、17061节点索引有效。测试上限拒绝改为15列/13行边界，保留坏格式/几何/断路等原测试。
- [x] 使用独立JSON见证fixture逐项核对141墙/6冰区/5坑/起终点，沿两条安全参考轨迹调用真实model，WON及五坑滚入LOST冻结保持；不在途中传送或清速度。
- [x] 进度按三稳定ID操作，第二关成功为两模式共享解锁第三；各关模式榜/累计失误隔离，旧两关计时/排序和失败累计语义回归。只更新原“第二关是末关”断言对应的规则，不删原末关边界覆盖。
- [x] `wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`跑完当前全部纯逻辑回归，检查无产品试验图目录入口。

### Task 4：统一正式视口、三关UI与安全启动迁移（REQ-037至039 / TC-053/054/057）

**Files:** 修改src/maze_ui.h/.c、src/main.c、src/maze_scroll_trial.c、src/maze_camera.c（仅必要）、tests/maze/ui_refresh_harness.c、ui_scroll_harness.c、scroll_runner_harness.c、tests/maze/lvgl-host/CMakeLists.txt；新增tests/maze/third_ui_harness.c。必要时重生成现有字体与元数据。

**Interfaces:** 使用共用maze_ui_set_service；maze_ui_loading无交互按钮；主入口回调调用已有navigation_sample/update/request，外层保持唯一store写入口。

- [x] 先在真实LVGL写三入口/成绩循环三关、loading反馈、第三关滚动/第一二关零镜头偏移、逐flush球像素完整和终帧单球断言，运行见RED。选择/下一关仅在允许状态触发装载，loading时不接受旧点击。
- [x] 把相机、固定view/world、234000B缓冲和旧新球失效union移到正式共用路径；normal/trial均调用同一私有定位逻辑。清场清指针/缓存/待处理事件，正式切关不重置app代次。
- [x] 按设计坐标排三入口、成绩/方向和底部保存反馈；成绩index从两关翻转改为目录长度循环。前两关控制/文案/几何保持，第三关准备提示不显示答案路线。
- [x] 正式main注册稳定nav上下文的服务回调：让出1tick、读最新采样、导航更新/幂等observe、更新IMU请求；回调不做UI或Flash。真实LVGL回放期间触发胜败/错误，验证状态转换后旧点击被拒绝，业务只结算一次；静态复核与目标调用路径核对实际main接线。
- [x] 启动/存档重试打开成功后，以store.revision初始化attempted_revision，而不是RAM loaded.revision。若upgrade_pending，先阻止这一轮选关命令消费、走现有安全菜单保存分支尝试一次，再恢复命令处理；失败显示未保存/重试但不永久锁住保存按钮或RAM游玩。
- [x] 旧档升级、保存重试及晚到结果回放覆盖：回调终态→外层再observe→保存、写完读回失败→重试、第二到第三next。断言archive逻辑变化/迁移revision各仅一次，不能由渲染次数增长。
- [x] 选择/next前显示并实际提交loading反馈，停止旧请求与清场后再同步校验；完成/错误后绑定或进错误页。记录check_ticks，不宣称长CPU检查期间可取消；若板端发现交互超出已确认预算，作为准出失败定位/分批，不能放宽输入超时。
- [x] 统一CMake应用源列表，正式UI也链接camera/第三图，trial只额外链接试验map；每个源只出现一次。档案/导航结构改变后用正常依赖构建或全应用源码编译，不能沿用/fast旧ABI对象。
- [x] 从实际ui/catalog字符串生成并检查字体，沿用现有Noto来源/许可，不安装。真实LVGL小图原局部刷新指标、新图完整视口、对象/主机堆及固定HUD均记录，主机数据不冒充目标资源。

### Task 5：整合复核、正式构建与一次板端交接（TC-058）

**Files:** 更新docs/guides/three-level-use.md（新）、现有使用指南/需求/验收/报告/路线/风险/当日日志；归档本轮evidence。src/SConscript保持正式不包含独立试验runner/map，main标识正式0.10.0-three-levels。

**Interfaces:** 消费前三任务数据/迁移以及Task 4正式UI；默认normal为唯一正式三关固件，独立试验不结算档案。

- [x] 收敛相关验证：纯逻辑全套、实际库迁移/GC中断、真实LVGL正常/错误/三关/滚动、字体源像素再现、分区和构建快照测试。`python scripts/check_project.py`及`git diff --check`成功；不复测无关工程。
- [x] 依据requesting-code-review做一次整轮独立代码复核，重点RAW/规范快照、迁移启动命令时序、回调/写档隔离、旧数据保持与RAM/栈；重要问题修复后只回归受影响内容。
- [x] 冻结产品源码，在已激活SDK终端运行`python scripts/build_firmware.py --variant normal`，等待完整退出0且source_unchanged=true，独立重算所有源码与三产物SHA。此阶段不自动烧录。
- [x] 核对实际ELF/FAL/下载：正式包含第三图/服务与store，排除试验map/runner；分区保持原值，无档案镜像；核对缓冲234000B/64B对齐、BFS静态36256B、新nav/store体积和SRAM尾区。查ARM编译栈信息或反汇编调用链预算，不能把单函数帧当8192B主线程峰值。
- [x] 指南提供真实成功构建目录和三文件命令，COM口须当前确认；说明自动v1升级、未保存RAM回退、旧正式固件不能读v2。无需用户擦档或电脑迁移工具。
- [x] 给用户一次新增短测：旧成绩/解锁保留与第三关规则、三关切换/两模式/成绩、第三关两种移动及暂停重玩、重启保持、一次free/ps与map/physics_gap日志。没旧档设备只记全新初始化，不冒充真机旧档升级。
- [x] 如实分开软件/受控中断/离屏/构建/板端反馈，保留既往失败；用户反馈前TC-058待执行。所有修改留本地，未获明确新授权不提交推送。

## 自检与交接

五项Review Focus均有对应任务和断言。v1 decode保留revision与store返回待提交n+1分层明确；规范current仍保留已提交n；初次迁移不会被attempted_revision误抑制。显示回调只处理纯逻辑，迁移/保存由外层安全菜单完成。新图与档案接口及尺寸一致，无新增外设/服务/安装。实施方法已由用户选定，不重复询问；计划交用户审阅后执行。

2026-11-02：五步的软件/构建/交接工作完成，TC-058板端短测仍待执行；勾选交接不代表已完成真实上板验收。
