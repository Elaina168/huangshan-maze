# 第3阶段成绩、解锁与存档实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 在板上提供分模式挑战成绩、前5排行、累计失误、关卡解锁及断电存档。

**Architecture:** 先用真实FlashDB与受控NOR适配验证两份完整档案的中断恢复，再接纯C挑战统计与导航/UI。档案编码独立于结构体布局；FAL适配仅在启动、结果和菜单等不采样的状态读写。已有导航代次/页面版本和唯一IMU所有者保留。

**Tech Stack:** SiFli SDK v2.5.1、RT-Thread、LVGL v9、SDK已有FlashDB/FAL、C99及已有WSL/Python测试环境；不安装新依赖或改SDK。

**Spec:** [已批准设计](../specs/2026-11-01-progress-save-design.md)。执行方式沿用本会话逐步实现，本计划已获用户确认。

## Global Constraints

- 只在主线程操作LVGL和游戏存档；校准/游玩/暂停不执行Flash擦写，提交前停用IMU请求。
- 用时累计各轮有效游玩时间，再统一转换百分之一秒；设备错误/校准失败不算失误。
- 每关每模式前5、用时优先同用时比失误、同分稳定；解锁共享，初始仅冰岔开放。
- A/B各32KiB，偏移0x00DB0000/0x00DB8000，不移动旧分区；不把存档列为烧录镜像，不擦整片。
- 非空未知/不支持版本不自动覆盖；首次建库中断无有效档案可停错误，不承诺重试必修复。
- 存储门槛未通过，不接后续产品持久化/UI；不恢复B/C，不提前做机关/看门狗，不自动提交推送。

## Review Focus

- FlashDB遇无效扇区头时在GC恢复前失败，另一份档案仍须可恢复（Task 1）。
- 写入返回失败但目标已部分/完整落盘，重试不重复计数或插榜（Task 1、3）。
- 设备恢复增加IMU代次但不是玩家重开，不能多加时间或失误（Task 3）。
- 新版读未知版本、同序号不同内容或两份均损坏，不得默认为新玩家（Task 1）。
- Flash阻塞不能发生在校准/继续时，成绩页不能运行旧app或接受旧点击（Task 4）。

## 文件与接口分工

- `src/maze_archive.h/.c`：固定容量档案、显式编码/校验；不依赖RTOS/FAL/UI。
- `src/maze_store.h/.c`：两个独立FlashDB实例的探测、选择、提交及读回验证；实际FAL由SDK提供，主机用测试适配。
- `src/maze_progress.h/.c`：挑战统计、一次结算、排行、解锁与档案revision变更，不调用存储或LVGL。
- `src/maze_navigation.h/.c`、`src/main.c`：事件前后统计接点、存储安全时机、锁关和成绩页导航。
- `src/maze_ui.h/.c`及字集生成：成绩/未保存/锁关/错误入口与挑战显示，保留原活动刷新回归。
- `project/ptab.json`、`project/custom_mem_map.h`、`project/proj.conf`：项目专用分区和已有库启用，不改SDK板表。
- `tests/maze/store-host/`：真实库使用的主机fdb_cfg/fal_cfg及RT函数适配、持久化NOR故障模型；不是给玩家的电脑工具。

## Task 1 档案格式及真实库中断门槛

**Files:** 新建`src/maze_archive.h/.c`、`src/maze_store.h/.c`、`tests/maze/archive_harness.c`、`tests/maze/store-host/{fdb_cfg.h,fal_cfg.h,rtthread.h,nor_harness.c}`、`scripts/test_maze_store.py`，证据存`evidence/research/2026-11-01-store-gate-01/`。

**Interfaces:** `maze_archive_t`保存`revision`、两个关卡的稳定ID/解锁/分模式完成标记/累计失误/前5成绩。`maze_score_t`为`uint64_t time_cs`与`uint32_t mistakes`，每模式5条，关卡ID固定32字节。`void maze_archive_default(maze_archive_t *)`、`int maze_archive_encode(const maze_archive_t *, uint8_t out[353])`、`int maze_archive_decode(const uint8_t *, size_t, maze_archive_t *)`；成功0，格式/版本/校验明确分开返回错误，外部数据验证数量、ID终止、标志及排序。

固定小端353字节：magic四字节`HMZ1`、格式u16=1、长度u16=353、revision u64、规则版本u32=1、关卡数u8=2；每关ID32/解锁u8/完成标记u8，两个模式各累计失误u32/榜单数u8/5条(time_cs u64、mistakes u32)；末尾CRC32覆盖此前字节。CRC32采用IEEE标准，固定测试向量`123456789`为0xCBF43926。使用稳定ID映射，不因目录重排改变归属；规则版本不支持时明确错误，本版不造迁移框架。

`maze_store_t`静态持有两份数据库、当前有效副本与错误/待修复状态；对外`int maze_store_open(maze_store_t *, maze_archive_t *)`、`int maze_store_save(maze_store_t *, const maze_archive_t *)`。每库键`archive`，选择最高revision；重试同一档案保持revision，游戏档案实际变更时由progress增revision。序号相同但字节不同、不支持版本都报错并禁止覆盖。每次提交只修改非当前有效副本，成功读回后切换；已有有效本项目档案才允许重建损坏目标副本。

- [x] 写编解码RED：完整往返、固定字节布局、CRC向量、截断/长度/非法ID/数量/版本/CRC/排序拒绝；实现最小编码并验证GREEN。
- [x] 编译SDK原样`external/FlashDB/src/fdb.c`、`fdb_kvdb.c`、`fdb_utils.c`，测试适配提供真实FAL头对应的NOR接口及本版实际使用的rt函数。不拷贝改写库算法；主机配置与目标的KV/FAL/缓存选项一致，SDK路径通过参数传入。
- [x] 写A/B恢复RED：空白首次创建、未知非空拒绝且Flash字节不变、单份损坏恢复另一份、两份无效/未知版本拒绝、同序号冲突、提交失败及重试；观察缺实现/断言失败后实现store最小逻辑，运行`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze_store.py --sdk /mnt/d/sifli/SiFli-SDK/v2.5.1`。
- [x] NOR模型限制写入只能1→0、擦除8192字节对齐；Flash镜像保留到下一独立进程。以强制中断结束进程而不是只返回写失败，重新启动真实库恢复。对已初始化档案的更新/GC轨迹枚举每次写/擦的前后及内部首字节/中部/尾部等代表断点，逐次验证恢复为完整旧/新档且未擦当前有效副本；记录覆盖矩阵，不能称为穷尽真实电气故障。
- [x] 连续提交直到初始化之后实际再次发生擦除，证明进入回收路径；首次建库中断另测，允许无有效档时明确错误，重试不误擦未知数据。读失败、部分写及读回失败单独标明注入，不冒充断电。
- [x] 门槛GREEN后归档库文件SHA、配置/编译命令、断点总数及恢复对照。若目标不满足，停在此任务修订存储设计，不进入Task 2及后续产品集成。

## Task 2 目标分区与存储后端

**Files:** 新建`project/ptab.json`、`project/custom_mem_map.h`、`scripts/test_maze_partitions.py`；修改`project/proj.conf`与第三方清单。

**Interfaces:** FAL名称`maze_a`/`maze_b`，生成标签`MAZE_SAVE_A_REGION`/`MAZE_SAVE_B_REGION`，游戏FAL使用maze_flash逐副本写保护代理，相对偏移0/32768；代理按生成的A区物理起点映射rt_flash_*驱动。保留dfu/ble原NOR_FLASH2_DEV_NAME及原偏移。启用已存在`CONFIG_PKG_USING_FLASHDB=y`、`CONFIG_PKG_FDB_USING_FAL_MODE=y`、`CONFIG_PKG_FLASHDB_ERASE_GRAN=8192`并核对实际Kconfig结果，禁止只手改生成rtconfig.h。

- [x] 分区测试先RED：项目表缺失或缺游戏区域，核对旧区域逐项不变、新区域地址/对齐/容量且无镜像字段。
- [x] 从实际板表保留原内容并新增A/B，项目本地custom_mem_map.h保留原板级语义，FAL新增两项通过maze_flash代理映射生成标签；新增src/maze_store_flash.c并测试读写闸门及错误传播，不改SDK。
- [x] 运行`python scripts/test_maze_partitions.py`，并使用真实SDK分区解析/生成路径验证优先选择项目表；后续完整构建再次核对生成ptab.h/FAL及默认三文件下载列表。仅配置/构建，不读写设备。

## Task 3 挑战统计、排行与解锁

**Files:** 新建`src/maze_progress.h/.c`、`tests/maze/progress_harness.c`；修改`src/maze_navigation.h/.c`、`scripts/test_maze.py`，同步REQ-033至036及TC-044至049。

**Interfaces:** `maze_progress_t`持有档案、当前挑战关卡/固定模式、累计完整毫秒、失误及本轮已归并/结算标记。提供`void maze_progress_init(maze_progress_t *, const maze_archive_t *)`、`void maze_progress_before_event(maze_progress_t *, const maze_app_t *, maze_event_t)`、`void maze_progress_observe(maze_progress_t *, const maze_app_t *)`、`void maze_progress_leave(maze_progress_t *, const maze_app_t *)`、`uint64_t maze_progress_time_cs(const maze_progress_t *, const maze_app_t *)`、`bool maze_progress_unlocked(const maze_progress_t *, const char *id)`。挑战开始上下文在成功绑定关卡后以app.level->id及app.reversed确定，禁止通过IMU代次推断玩家尝试身份。

- [x] 写RED：两次落坑后通关累加时间/失误；LOST/WON重复观察不重复结算；暂停重玩保留旧用时；通关重玩/退出重选新挑战；结果等候不计时；每轮不足10ms累计后再取整。
- [x] 写RED：设备错误RETRY/暂停重校准不加失误或双计时间；榜单第5/6名边界、稳定同分及模式/ID隔离；任一模式通关共享解锁、初始拒选第二关、最后一关边界。
- [x] 实现纯逻辑与导航锁关，结果观察只修改RAM档案；实际变化递增revision，重复保存不触发业务函数。运行完整`test_maze.py`及Task 1存储测试GREEN，旧导航测试以显式已解锁测试档案覆盖两关切换，不删原测试。

## Task 4 板上界面、保存时机与错误重试

**Files:** 修改`src/main.c`、`src/maze_navigation.h/.c`、`src/maze_ui.h/.c`、真实LVGL harness/CMake、字体生成物和字集。

**Interfaces:** 导航新增成绩页、启动存档错误页及对应页面版本；新增查看成绩/选择关卡成绩/切换成绩模式/返回/重试保存命令，命令仍验证页面版本与关内状态。main先初始化UI再打开存储，成功后使用读取档案初始化progress；仅安全状态停用IMU后调用store，存储错误与传感器错误分开保存。

- [x] 真实LVGL RED：锁定入口可见不可进入、分关分模式前5/无记录/完成状态/累计失误、挑战用时及本次失误、前5结果提示、错误码/未保存/重试/副本修复提示。
- [x] 主循环事件前归并、采样/update后observe结果，结果只结算一次；优先处理已存在的结果再接重玩/返回，避免快速点击漏结算。结果/选关/存储页保存最新dirty档案；校准/游玩/暂停不擦写，继续前不做补写。保存成功才清dirty/错误，失败保留RAM并明确提示；重试不重新记分。
- [x] 实现导航/界面并更新实际字集，界面所有新文字纳入字体生成和实际像素检查；必要时调整布局但保留原活动转换40000/单步10000像素上限。成绩页用分页保持390×450范围，不挤进关内活动显示。
- [x] 真实LVGL验证旧点击/缓存清理与新页面返回，主机模拟调用轨迹证明活跃状态无Flash调用；运行原UI/校准/暂停/运动回归及存储失败→RAM继续→重试成功链路。

## Task 5 完整构建与新增板端短测

**Files:** 更新使用/短测指南、需求追踪、测试报告/风险/项目计划/开发日志/THIRD_PARTY；`src/main.c`版本`0.9.0-progress-save`。

- [ ] 运行相关model/probe、真实库中断矩阵、真实LVGL/字形、分区、快照和`python scripts/check_project.py`；独立复核整个第3阶段增量，修复重要问题后只复测相关项。
- [ ] 冻结源码，在激活SDK终端运行`python scripts/build_firmware.py`，核对完整结束、实际启用配置、生成ptab.h/FAL、新旧分区及默认下载不含档案；独立核对全部源码/三产物SHA，记录库与静态/动态资源边界。
- [ ] 给出准确烧录目录，用户确认实际端口后自行烧录。新增短测：首次锁关、失败累计、通关/next解锁、两模式排行榜、重启保留、待保存提示；记录实际读写耗时及新版准备/结果态资源，不重做旧A/B/C。
- [ ] 板端保存完成后正常重启验证持久化；保存中真实断电验证与主机注入分别记录，不用前者未知填通过，也不要求用户反复盲拔。存档不随默认三文件烧录擦除的结论须有地址及实际证据，整片擦除会丢档应在指南说明。
- [ ] 归档失败/修复/回归及AI协作，未实测保持待证。用户明确收工再审查、钩子提交、正常推送并核对远端SHA，本计划不自动提交。

## 计划自查

设计范围由Task 1编码/恢复门槛、Task 2分区、Task 3统计规则、Task 4交互/时序及Task 5构建/板端覆盖。五个Review Focus均有测试归属；新命名为项目选择，现有SDK标识符来自只读核查。首次建库中断不保证自动修复，与正常更新完整旧/新档保证分开验证。执行方式沿用本会话，不要求用户另选；先审阅计划再实施。
