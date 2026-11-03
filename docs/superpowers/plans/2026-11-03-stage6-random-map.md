# 第6阶段板端随机地图实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans 在本会话原生执行。计划待审阅，未开始产品实现；用户已有整阶段执行偏好，不重新询问执行方式。

**Goal:** 板上生成可取消的随机迷宫与认证机关模板，保留当前种子/两模式成绩及固定五关档案。
**Architecture:** 双稳定地址地图/定义缓冲与独立生成任务；生成与校验可续作，外层主循环显式发布；随机成绩独立块以982B v5同事务保存。
**Tech Stack:** 现有C99、SiFli SDK2.5.1/RT-Thread/LVGL9/FlashDB、Python及已有WSL编译器，不安装依赖。
**Spec:** [已确认设计](../specs/2026-11-03-stage6-random-map-design.md)

## Global Constraints

保留固定五关稳定ID、布局/记录、180px/s限速及600加速度、100ms输入/时基保护、0.12.1远墙包围盒筛选、128B诊断和显示完成等待。沿用600×500随机世界/350×300视口及当前14×12容量上限，不新增整图像缓冲。UI线程操作LVGL；生成/校验/写档不能在flush服务回调内执行。每bank32768B/sector8192B不改，不清档、不自动烧录/安装/提交推送，旧B/C不恢复；阶段7/8不实现。

## Review Focus

1. 活动地图自指针及字符串被候选覆写：任务1/3测试固定地址、反复发布和取消、旧页面引用。
2. 单个BFS节点遍历大量对象拖长UI：任务1测试邻边/墙/坑/机关中途让出、1024检查/128节点/10ms截止与tick回绕。
3. 生成取消丢失原暂停挑战或停掉同代次IMU：任务3/5测试生成中健康采样、取消继续、累计时间/失误不重复。
4. 新种子与旧榜混合、未知生成器块被固定关保存归零：任务4测试取消/失败/接受事务、混合版本与实际SDK库中断。
5. 固定五关加随机入口/保存按钮超出command数组或屏幕：任务5测试9个命令的独立性/触摸、真实LVGL异步刷新及字形。

## 文件与接口约定

新增src/maze_random.h/.c（生成状态机/确定性PRNG与拥有定义存储）、src/maze_random_templates.h/.c（受支持模板与旋转/包络契约），测试分别放tests/maze/random_*_harness.c；新增scripts/test_maze_random.py并接入完整test_maze.py。

maze_generated_t拥有maze_map_definition_t definition、21行每行26字节的char lines、21个const char*行指针和有界对象/机关表；固定12×10底图，id为random_trial。其地址/内部数组不能移动。maze_generator_t保存PRNG、候选、DFS栈/主路径、阶段/游标/计数与接受种子；两者须静态生命周期，不放线程局部栈。

新纯C接口maze_generator_begin(maze_generator_t*,maze_generated_t*,uint32_t seed,bool exact)、int maze_generator_step(maze_generator_t*,unsigned operations)、void maze_generator_cancel(maze_generator_t*)；状态MAZE_RANDOM_PENDING=1/成功0，失败负值，错误区间-2201起，分别表示候选耗尽/不支持生成版本/已存候选重建失败。exact=true只重建该种子一次，false最多8候选；候选推进PRNG参数固定1664525/1013904223，版本1固定，零种子正常。

地图校验共用现有maze_map.c和队列/位图工作区，不重复分配36256B队列。新增maze_map_begin(const maze_map_definition_t*,maze_map_t*)、int maze_map_step(unsigned node_budget,unsigned check_budget,uint32_t deadline_tick,uint32_t(*clock_now)(void))、void maze_map_cancel(void)，新增MAZE_MAP_PENDING=1。一个静态校验任务/非重入，clock_now=NULL表示主机同步调用无墙钟限制；clock非空按有符号tick差比较截止，处理回绕。原maze_map_load是begin+无限预算step同步包装，固定五关原结果/几何/节点统计保持。

导航将单个map改为maze_map_t maps[2]及maze_map_t *active_map，定义存储也有两个对应的maze_generated_t槽，发布只切指针。补随机域标志、生成请求id、来源页面/状态、seed/exact和待发布状态；固定index仍只索引五关，随机不是目录第六项。新增maze_navigation_random_open(nav,seed)、maze_navigation_random_replace(nav)、maze_navigation_random_step(nav,now,clock_fn)、maze_navigation_random_cancel(nav)、maze_navigation_random_accept(nav,job_id)。step返回待发布时，由外层main先maze_ui_clear再accept；maze_navigation_update/flush回调不驱动生成。

档案新增maze_random_record_t {bool valid;uint16_t generator_version;uint32_t seed;maze_mode_record_t modes[2];}与archive.random；显式小端编码，无结构体直接落盘。MAZE_ARCHIVE_V4_BYTES=845、MAZE_ARCHIVE_BYTES=982、MAZE_ARCHIVE_LEVELS仍5。progress增加bool random分域；所有固定level下标访问前按域分支，不能把index_of=-1强转后索引数组。

## Task 1：可续作地图校验与双缓冲基础

**Files:** src/maze_map.c/.h、maze_navigation.c/.h、main.c；tests/maze/map_load_harness.c、navigation_harness.c；新增tests/maze/random_map_step_harness.c。
**Interfaces:** 上述map begin/step/cancel及active_map，旧maze_map_load外部语义不变。

- [x] 写RED：同一实际固定图同步/逐检查预算1得到相同几何/节点与结果；非法数据错误与原调用一致；邻边/对象对扫描中途可续作，预算计数准确。
- [x] 将格式、装载、几何、基础/动态对象、单调机关状态路线等阶段拆为游标；每片最多128节点、1024基本几何检查，任一墙钟截止立即让出，不在一次节点内隐藏无界扫描。
- [x] 用可控clock测试10ms截止、tick回绕和总超时上层可取消；复用一个全局队列，cancel不发布半成品。
- [x] 导航以maps[2]/active_map改固定装载，更新main节点日志和一个直接断言n.map.level的现有测试为活动指针；所有旧固定关/错误页/切关/最新成本回归GREEN。

## Task 2：确定性骨架、模板与动力学认证

**Files:** 新增maze_random*.c/.h与random_generator_harness.c、random_templates_harness.c、random_route_harness.c；scripts/test_maze_random.py及test_maze.py。
**Interfaces:** generator begin/step/cancel、maze_generated_t，与Task1异步校验消费的definition；所有生成表归拥有槽存储。

- [x] 写RED：固定种子0/1/0xffffffff与预选32个种子可再现，分片预算1/1024输出完全一致；实际结构有差异，非随机颜色；exact不跳种子，最多8候选无死循环。
- [x] 实现6×5宏格随机DFS树、边界起点/最远终点、主路径>=10格且>=3分岔/死路、12×10字符串导出。失败不删约束伪装成功。
- [x] 按设计实现有限模板表：terrain/可绕坑/checkpoint、最多两组钥匙或开关门、magnet35/12/<=240、laser/gate/patrol沿用周期/35px/s；至少两类复杂模板、最多4动态/16基础；本版不随机破墙。
- [x] 对模板入口出口、影响包络、门户/等待区、允许旋转/镜像建立实际几何与模型测试；危险前检查点安全、陷阱必经但可绕、钥匙在门前可达、模板间不相互影响。
- [x] 真实模型验证每种支持变换的制动/等候/通过及巡逻绕行，覆盖相位边界/周期抽样；至少4个固定种子整图控制输入WON及失败/复活，不把有限集合称全部种子证明。未认证模板不能启用，认证失败必须修模板或如实停在门槛，不偷偷缩减已确认范围。
- [x] 在32种子/重建/非法模板用例GREEN后导出两张真实生成图预览，保存种子/版本/几何摘要和实际模型日志。

## Task 3：生成生命周期、取消与原子发布

**Files:** src/maze_navigation.c/.h、maze_random.c/.h、main.c及random_navigation_harness.c；实际main I/O替身。
**Interfaces:** 随机open/replace/step/cancel/accept；MAZE_NAV_GENERATING与随机错误页追加到原枚举，固定五关index不扩展。

- [x] 写RED：首次进随机生成、已存seed exact重建、换图候选、取消/失败恢复原准备/暂停/结果；种子/机关/时间/失误/旧榜直到接受前完全不变。
- [x] 页面转换增加page_version，生成使用独立job_id；迟到完成、快速换图/取消、旧触摸和旧IMU样本不能发布新地图；accept仅匹配当前已成功job。
- [x] 生成期间原暂停会话保持采样健康/参考、物理与相位冻结；旧app代次不随候选取消而改。实际worker handled_generation生命周期回归覆盖取消后继续可采样，真实故障不吞掉。
- [x] 每轮主循环仅调用一次step，先处理取消命令再做工作；从拓扑到校验累计3秒截止，超过则失败保留旧图；flush服务仅原暂停/输入健康，不生成或写档。
- [x] 接受前main解除旧LVGL引用，再progress_leave并新app代次绑定候选，切active_map指针；成功才替换随机档案/清榜并排队安全保存。取消不复用半成品、无已接受图时错误可返回。实际main与旧重入/代次回归GREEN。

## Task 4：982B v5档案与随机成绩隔离

**Files:** src/maze_archive.c/.h、maze_store.c/.h、maze_progress.c/.h；archive/store-host夹具及脚本；新增tests/maze/fixtures/archive-v4.bin、random_archive_harness.c和random_progress_harness.c。
**Interfaces:** 982B v5布局=21头+820固定五关+137随机块+4CRC，头count=5/rules1；当前seed两模式各前5及losses，非固定第六榜。

- [x] 改codec前用真实v4编码器生成合成golden并独立检查CRC/五关字段；仅该.bin路径忽略例外，不提交固件或真实档案。
- [x] 写RED：v1/v2/v3/v4读后随机valid0且全零；v5固定五关字节保持；seed0、模式隔离、排行同分/第6名、valid0非零垃圾拒绝、非法count/CRC/版本/长度不发布输出。
- [x] 更新显式codec/store版本与legacy判断；未知generator保留其已知结构内容，不执行该生成器，不影响固定关原样写回该块；未来档案版本/rules仍保护。最高revision不回绕。
- [x] progress按固定/随机分域共享现有计时与插榜规则；随机失败/通关一次结算，无固定解锁副作用；重玩不清榜，换图接受才以一次修订替换seed+两模式清空记录；取消/失败不清，exact重建不加修订。
- [x] 实际SDK FlashDB/NOR覆盖四种旧格式到v5迁移、同修订混合/冲突、最大修订、保存/读回失败同修订重试、GC及初建写擦边界受控中断。核对982B容量和源bank不改，不重复宣称物理断电通过。

## Task 5：板端随机入口、生成页与成绩UI

**Files:** src/maze_ui.c/.h、main.c、maze_config.h、字形生成结果；新增tests/maze/random_ui_harness.c，修改lvgl-host/CMakeLists.txt、必要真实异步用例及test_maze_main.py。
**Interfaces:** 追加MAZE_UI_RANDOM、MAZE_UI_RANDOM_REPLACE、MAZE_UI_RANDOM_CANCEL、MAZE_UI_RANDOM_SCORES；新增MAZE_UI_MAX_BUTTONS=10替换当前button_commands[8]，各command带现有页面/代次校验。

- [x] 写RED：五个固定关＋随机入口＋成绩/方向/保存同时出现时9个按钮均独立可触发，390×450可读、不覆盖；采用滚动入口列表或经离屏验证的排版，不缩小到难点触的尺寸。
- [x] 生成页显示阶段与进度、取消；随机准备显示8位十六进制seed/版本/模式、开始/“换图并重置本图成绩”/返回；暂停/结果可换图，游玩中无直接换图入口。
- [x] 随机成绩页显示当前seed及两模式成绩，固定五关成绩页循环保持五项，不出现假第六关；未知generator/重建失败/超时/存档错误都有明确出口，不能回退旧图冒充生成成功。
- [x] 主循环安全菜单保存，生成/写档不在flush回调；新诊断满足实际128B最宽日志预算，记录候选/阶段/每片与总耗时而不逐节点刷串口。
- [x] 更新字形并验证源像素/可再现；真实LVGL进度/取消/种子标题/榜单/错误/滚动与局部球刷新，异步源消费无覆写/黑洞，保留原五关与试验回归GREEN。

## Task 6：整合、独立复核、正式构建及短测交接

**Files:** docs/guides/stage6-use.md、需求/验收/报告/计划/风险/路线/开发日志与evidence实际输出。

- [x] 完整模型/新生成器与模板、actual-main、真实SDK存储、六个原真实LVGL目标及新随机UI、字形、分区和快照测试通过；保存RED/失败与回归，不补造板端结果。
- [x] 一次fresh-context整体复核Review Focus五项；修重要问题先RED再GREEN、跑受影响回归，不做无关重构。
- [x] python scripts/check_project.py和git diff --check；在现有激活SDK终端python scripts/build_firmware.py完整退出0/source_unchanged。逐项核对当前源码与快照、三产物SHA、目标正式图/生成器符号、原分区和无清档镜像。
- [x] 从实际ELF复核双地图/定义缓冲、共用队列、store双镜像及主线程嵌套栈，不拿静态尾区当free。若982B扩展使嵌套栈不足，按非重入所有权移出大局部工作区并回归，不无依据增栈或改分区。
- [x] 指南交接一次新增板端短测：生成/取消/重玩同图/换图与成绩清除、固定榜保留、重启同种子、机关游玩和第五关健康；记录每片<=10ms/总<=3s目标、free/ps与真实服务间隔。没有数据只记总体反馈，不把TC-075全部判通过。

## 自查与执行记录

设计各节对应Task1至6；未知generator块保护/取消生命周期/模板动力学/分片细粒度均有明确任务。后续接口名称沿用本节声明，不以新代码猜现有字段。本计划待用户审阅，在本会话原生连续执行，不另开执行方式选择；用户未要求本次提交推送。


## 执行结果

Task1至6软件、复核、完整normal构建与交接完成，证据见../../../../evidence/stage6-2026-11-03/README.md。最后一项是板端短测指南已交接，不代表TC-075已执行。目标10ms/3s与free/ps待用户实测，未提交推送。
