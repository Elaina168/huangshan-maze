# 大地图与跟随镜头试验实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 在独立试验固件中运行700×600地图和350×300跟随视口，验证板端滚动手感和性能。

**Architecture:** 物理、碰撞与IMU使用原世界坐标和时基；纯C镜头输出显示偏移。试验入口复用app和UI基础设施，绕开正式导航结算及Flash档案；正式构建默认不启用试验。

**Tech Stack:** 现有SiFli SDK v2.5.1、RT-Thread、LVGL v9、C99、Python与WSL主机测试；不安装依赖。

**Spec:** [已批准设计](../specs/2026-11-01-scrolling-map-trial-design.md)。用户已选择本会话逐步实现，沿用原执行方式。

## Global Constraints

- 地图14×12格，每格50像素；视口350×300；中央区域140×120；不缩放球、墙或通道。
- 原物理参数及100ms时序/采样限制不变，不以放宽阈值消除性能错误。
- 试验不打开或写入游戏档案，不新增正式目录、解锁或排行榜记录。
- HUD与提示固定在视口；仅UI主线程操作LVGL；不分配整张地图像素缓冲。
- 用户真实板端操作才算板端证据；不恢复旧B/C测试，无自动烧录、提交或推送。
- 保留既存docs/course-requirements.md差异；文本UTF-8、中文代码注释。

## Review Focus

1. 地图边缘及折返时镜头夹紧且球可见；平滑不能导致高速移动的球被甩出视口（Task 1/3）。
2. 暂停、校准、错误、结果冻结镜头；重玩直接复位，继续不补算停顿（Task 1/3）。
3. 滚动内容不能盖住HUD、固定提示或触摸按钮；离屏物体进入视口时正确出现（Task 3）。
4. 试验从启动到退出均不接触Flash档案，正式构建不能误带试验入口或地图（Task 2/4）。
5. 地图扩大后的索引、静态内存、装载和绘制开销必须测量，不能沿用旧局部刷新通过结论（Task 2/4）。

## 文件职责与接口

- 新建src/maze_camera.h/.c：无LVGL/RTOS依赖的镜头状态与更新。
  - `maze_camera_t`持有偏移、世界/视口尺寸与上次tick；不修改model。
  - `void maze_camera_reset(maze_camera_t *camera, float world_width, float world_height, float ball_x, float ball_y, uint32_t now_tick);`
  - `void maze_camera_update(maze_camera_t *camera, float ball_x, float ball_y, uint32_t now_tick, uint32_t tick_hz, bool playing);`
- 新建src/maze_map_scroll_trial.c：试验字符串地图，导出`const maze_map_definition_t maze_map_scroll_trial`，仅试验构建编译。
- 新建src/maze_scroll_trial.h/.c：`int maze_scroll_trial_run(void)`；独立app主循环，已有UI/IMU初始化完成后调用，不调用maze_store或maze_progress业务结算。
- 修改src/maze_ui.h/.c：共享视口场景绘制，新增试验入口`void maze_ui_render_trial(const maze_app_t *app)`及`bool maze_ui_take_trial_event(const maze_app_t *app, maze_event_t *event)`；正式导航入口保持兼容。
- 修改src/main.c：试验初始化后进入独立runner，正式存档路径只保留在正式构建；日志明确试验版本。
- 修改src/maze_config.h、src/SConscript、scripts/build_firmware.py：显式试验构建开关，默认正式；试验容量14×12、正式7×6，按构建选择源文件和宏。
- 新建tests/maze/camera_harness.c、tests/maze/scroll_trial_harness.c、tests/maze/ui_scroll_harness.c；扩展scripts/test_maze.py和tests/maze/lvgl-host/CMakeLists.txt运行相关验证。
- 修改scripts/test_build_snapshot.py：构建变体及快照可追溯测试。
- 更新需求/验收/计划/风险/日志，新增docs/guides/scrolling-map-trial-use.md及本轮evidence目录。

### Task 1：纯C镜头与行为验证

- [x] 在需求与验收登记REQ-037及TC-050至052，注明试验范围、通过条件与证据层次。
- [x] camera_harness先断言：350×300视口、140×120中央区内不追随；越界产生正确方向偏移；四边限制在[0,350]×[0,300]；小地图偏移为零。
- [x] 断言暂停等非playing调用更新时间基准但冻结偏移，恢复不补时间；reset在新位置立即定位；20/40Hz等总时长更新在持续追随段结果一致或在明确浮点容差内。
- [x] 运行新增测试保留RED，再实现上述两个接口。平滑建议起点为指数跟随系数12/s；目标为小球回到中央区域边缘所需偏移，最终夹紧世界边界；为球保留可见余量，不能用镜头影响运动。
- [x] 用固定世界轨迹验证最大速度180px/s、连续折返、四角与短边地图；同一输入下有无镜头的model轨迹/胜败完全相同。通过后保留GREEN证据。

### Task 2：大地图与独立试验构建

- [x] 构建脚本新增`--variant`，仅接受`normal`（默认）和`scrolling-trial`；显式向子进程传递所选变体，清除调用终端遗留试验环境影响。manifest记录变体/宏，源码快照校验规则保持。
- [x] src/SConscript仅试验时定义`MAZE_SCROLL_TRIAL`并纳入试验runner/地图；正式构建不包含这两个源文件。依据实际DefineGroup接口传宏，不编辑SDK。
- [x] 给test_build_snapshot新增正式默认、显式试验、遗留环境不影响正式、增删改仍失效的断言，先RED再实现；检查子进程实际环境与manifest一致。
- [x] 以宏选择地图容量。扩大后检查节点数17061、uint16_t索引上界、数组大小与静态工作区；正式构建仍保持原容量。
- [x] 编写14×12试验地图：包含跨两个轴的折返与分支、普通/冰面及可绕行必经通道陷阱。沿用地图格式、50像素网格、原球半径，不做正式目录新增。
- [x] scroll_trial_harness用真实装载与model验证尺寸、格式/容量拒绝、静态安全路线、实际惯性通关和陷阱失败；保留世界轨迹供镜头回放，不通过传送球或中途清速制造成功。
- [x] runner复用`maze_app_init/sample/update/event/request`及唯一IMU生产者，提供触摸开始、暂停、继续、校准、重玩和退出到试验准备页；停止/退出立即更新生产请求。复用现有初始化失败上报，不添加隐式恢复。
- [x] 用会失败的存储调用桩及源码/目标符号调用检查验证试验不调用store；不借用正式关卡索引来结算试验成绩。

### Task 3：视口渲染与真实LVGL验证

- [ ] 新增ui_scroll_harness，先用实际LVGL离屏断言：350×300裁剪、世界到屏幕位置、HUD与按钮不随地图移动、四边和折返、暂停冻结/重玩定位，保存RED。
- [x] 将现有地图区域分为固定视口与世界内容；球与几何减镜头偏移，提示在视口上层。现有小地图偏移为零，正式页面/事件代次检查保持。
- [x] 初始策略使用持久场景对象和LVGL视口裁剪；离屏对象不绘制，活动帧不重建整页。不为整图分配位图；只有测试证明对象开销或裁剪不足时才改用可见对象池，避免同时实现两套渲染。
- [x] 镜头以实际tick更新，仅游玩态追随；使用渲染时的当前model位置。检查负屏幕坐标的舍入，避免沿用仅适于非负值的`value+0.5`产生偏差。
- [x] 运行新的离屏用例，导出中心/边缘/折返截图、每帧刷新像素、对象/堆变化。滚动可刷新完整视口，但不能无故刷新整个390×450屏幕。
- [x] 回归既有ui-refresh正常/地图错误页；正式小地图保留原局部刷新判据。检查新增中文字符，必要时按现有源字体/授权生成并验证字形，不增加字体依赖。

### Task 4：集成、双变体完整构建与板端短测交接

- [x] 执行`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`及新增试验/真实LVGL目标；运行`python scripts/test_build_snapshot.py`、`python scripts/test_maze_partitions.py`、`python scripts/check_project.py`与`git diff --check`。
- [x] 原存档字节格式和分区不变；检查相关回归，不扩大到与试验无关的故障注入重跑。
- [x] 完成一次整轮代码复核，重点为镜头可见性、独立启动/存储隔离、原UI回归、数组容量及实际刷新开销；发现问题后仅重跑受影响检查。
- [x] 在已激活SDK终端分别运行`python scripts/build_firmware.py --variant normal`和`python scripts/build_firmware.py --variant scrolling-trial`，每次等待完整退出，立即归档各自log/manifest/三固件SHA，避免last_build覆盖证据。
- [x] 独立核对两种变体源码哈希、下载三文件范围、ELF入口/符号与静态内存；试验版记录工作区及固件增量，正式版不携带试验地图/runner，不改变存档地址。
- [x] 使用指南给出真实构建目录、COM6仅在确认后使用的烧录命令，提醒先关闭串口监视；不自动烧录。试验启动日志明确不保存成绩。
- [x] 交付一次板端短测清单：横纵滚动、折返/四边、暂停继续/重玩、冰面/陷阱/终点。用户评价流畅与跟手；若出现-1002/-1004或明显卡顿，记录原始时序与现象再定位，不提高100ms阈值。
- [x] 日志如实区分自动测试、离屏、构建和待用户实测。用户未反馈前不宣布板端通过；仅在用户明确授权后提交推送。

## 自检与交接

设计中的局部视野、平滑/死区/边界、状态冻结、HUD固定、大图校验、板端性能和存档隔离均已有所属任务。具体试验布局和渲染内部组织属于实现选择，须用真实装载/model/LVGL证据验证，不改变正式地图。用户已确认计划，按本会话原生方式执行。

执行记录：Task 1/2及Task 3实现和最终验证完成。UI初始RED因缓存/并发构建未形成有效缺接口证据，此项保留未勾；实际字体断言失败及全应用源码重编通过另见证据。Task 4双构建/交接已完成；板端验收由用户执行，尚未通过。
