# 第2阶段多关卡与方向设置实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 在板上提供两关正式迷宫、选关/下一关/返回与方向反转，移除产品实验地图。

**Architecture:** 纯C导航层持有单一静态装载结果、关卡索引和方向设置，原app继续管理关内状态。主线程在切关前解除LVGL场景引用，再调用导航层装载；后台IMU仍由原唯一所有者提供带代次快照。

**Tech Stack:** SiFli SDK v2.5.1、RT-Thread、LVGL v9、C99；现有WSL编译器及字体生成工具，不安装新依赖。

**Spec:** [已批准设计](../specs/2026-11-01-level-selection-design.md)。执行方式沿用用户指定的本会话逐步实现；本计划已获用户确认。

## Global Constraints

- 只在主线程调用LVGL；不增加IMU线程，不改既有速度、阻尼、校准及100ms阈值。
- 7×6格、50px格距、8px墙厚，地图结果和9204B检查工作区静态存储，切关前解除旧引用。
- 第一关冰岔布局不变；第二关布局单独给出预览，确认后才写产品定义。
- 方向反转只在选关页修改，两轴同时反转，校准原始数据不变；重玩/下一关保留，重启默认正常。
- 不做解锁/成绩/存档/机关；不恢复B/C；不提交推送，不动既存课程清单改动。

## Review Focus

- 重选同关后旧采样代次不能再次有效（Task 1导航测试）。
- 同名页面返回重入后的旧点击失效（Task 3真实LVGL测试）。
- 载入失败不得调用旧app采样/物理或访问覆写地图（Task 1及Task 3受控失败测试）。
- 暂停重校准错误缓存不得带入新关（Task 3页面重建回归）。
- 实验图只允许在测试夹具/历史材料中保留，产品与最终ELF应无对应符号（Task 2及Task 4）。

## Task 1 导航、切关与反转

**Files:** 新建`src/maze_navigation.h/.c`、`src/maze_catalog.h/.c`、`tests/maze/navigation_harness.c`；修改`src/maze_app.h/.c`、`scripts/test_maze.py`及需求/验收文档。

**Interfaces:** 新建`maze_catalog_entry_t`保存`title`及`const maze_map_definition_t *definition`；以`maze_catalog`和`maze_catalog_count`提供有序目录。新建`maze_navigation_t`保存SELECT/GAME/MAP_ERROR页面、页面版本、关卡索引、reversed、error、app首次初始化标记、`maze_map_t map`及`maze_app_t app`。提供`void maze_navigation_init(maze_navigation_t *, uint32_t tick_hz)`、`int maze_navigation_select(maze_navigation_t *, unsigned index)`、`void maze_navigation_back(maze_navigation_t *)`、`void maze_navigation_toggle_direction(maze_navigation_t *)`、`int maze_navigation_next(maze_navigation_t *)`。select/next要求主线程先解除旧UI地图引用，索引来自目录按钮；next只允许WON且存在下一关。

`maze_app_t`新增`bool reversed`；`void maze_app_bind_level(maze_app_t *, const maze_level_t *, bool reversed)`要求已取消旧会话，保留递增generation并重置本轮状态。首次成功选择才调用原`maze_app_init`；后续切关不重新memset app。反转在死区处理后的屏幕输入处完成。

- [x] 同步REQ-029至032、TC-039至043到产品需求/验收计划，保留未执行状态。
- [x] 写导航与反转RED：首次处于SELECT/正常；两关切换及同关重选代次递增；返回关闭请求；旧采样不能推进新校准；位置/速度/时间/参考重置；相同输入两模式符号相反、校准参考相同。
- [x] 用测试专用两条定义（可共享同一合法几何、不同ID）及非法定义驱动目录，验证最后一关无next、载入错误可back、选关/失败时禁止游戏采样更新。运行`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`观察缺新接口RED，保留日志。
- [x] 实现最小导航、app重绑定及反转，测试目录不冒充第二关产品布局。运行同命令确认全部新测试及原后台/校准/物理/状态/地图GREEN。

## Task 2 第二关与实验图清理

**Files:** 新建`docs/design/maze-layout-2026-11-01/`布局数据/预览、第二关布局说明、`src/maze_map_second.c`、`tests/maze/legacy_levels.h/.c`；修改`src/maze_catalog.c`、`src/maze_level.h/.c`、惯性及模型harness、`scripts/test_maze.py`。第二关文件中的正式ID/显示名由布局审阅确定。

**Interfaces:** 第二关导出`const maze_map_definition_t maze_map_second`供目录使用；测试专用文件承接现有`maze_level_one`与`maze_level_surface_lab`常量及声明。产品`maze_level.c`仅保留通用地面查询，删除两个实验图ID及声明；测试只链接测试专用定义。

- [x] 提出第二关具体字符串、对象表、中文名及预览；计算分岔/死路、必经陷阱通道及完整球体静态安全折线，明确这是设计证据。
- [x] 给用户审阅第二关布局，等待确认；此项不阻止Task 1纯逻辑工作，但未确认前不定稿产品第二关或发布双关固件。
- [x] 新地图harness先断言几何/完整安全路线/真实model通关与各坑失败并观察RED，再实现第二关常量及目录两入口。
- [x] 迁移实验图至测试夹具并修改编译链接；检查`rg -n 'maze_level_one|maze_level_surface_lab|MAZE_LEVEL_ID_ONE|MAZE_LEVEL_ID_SURFACE_LAB' src`无残留（UI清理在Task 3完成）。运行完整`test_maze.py`验证旧几何/惯性回归与新地图测试GREEN。

## Task 3 真实UI与主循环集成

**Files:** 修改`src/maze_ui.h/.c`、`src/main.c`、`tests/maze/ui_refresh_harness.c`、`tests/maze/lvgl-host/CMakeLists.txt`、字体字集及生成字体。

**Interfaces:** UI入口改为`void maze_ui_render(const maze_navigation_t *)`，导航按钮使用新建`maze_ui_command_t`（kind、index、game_event）表达SELECT/TOGGLE/BACK/NEXT/GAME；`bool maze_ui_take_command(const maze_navigation_t *, maze_ui_command_t *)`验证页面版本、关内状态及generation。`void maze_ui_clear(void)`删除场景并清除全部对象/显示缓存及待处理事件，在所有实际切关装载前调用。不保留失效API包装。

- [x] 真实LVGL先补选关/两关标题、模式标签、准备返回、胜利下一关/末关完成、失败/错误返回按钮及旧事件拒绝断言RED。
- [x] 实现目录驱动的中文页面；移除旧实验图标题、说明和普通/冰面分割装饰分支。原地图活动态局部更新方式保留，完整场景只在导航切换时重建。
- [x] 主循环只在GAME页处理app采样/更新/渲染；所有页面持续UI服务。SELECT/MAP_ERROR页停止IMU请求。GAME退出由导航处理，校准取消仍回本关准备，next进下一关准备不自动校准。记录关卡ID、模式、代次和加载检查tick。
- [x] 为受控载入失败→选关→成功、暂停校准错误→切关、同页面重入旧点击补真实LVGL回归；保持原活动转换40000像素/单步10000像素阈值，不把选关整页刷新冒充活动态刷新。
- [x] 根据实际新增文案生成字体，运行生成`--check`、实际LVGL字形解码源像素检查及离屏布局测试；打开实际导出的选关/两关/结果/错误图确认无缺字、截断和越界。

## Task 4 固件、记录与板端短测

**Files:** 更新`docs/guides/maze-use.md`、新阶段短测指南、测试报告/计划/风险/开发日志及`evidence/`；修改`src/main.c`版本为`0.8.0-level-selection`。

- [x] 执行主机model/probe、真实LVGL及字体检查、快照与`python scripts/check_project.py`；独立复核本轮代码并修复重要问题，重跑相关失败项，不扩大无关测试。
- [x] 冻结源码，在已激活SDK终端运行`python scripts/build_firmware.py`，核对完整退出状态、source_unchanged、源码与三产物SHA；以目标ELF确认实验图符号不再链接、静态工作区/地图结果大小。
- [ ] 更新玩家指南与准确烧录目录；用户实际确认端口并关闭串口占用后自行烧录。记录两关选择/返回、胜利next、末关结果和正常/反转四向、切关重校准及加载日志；不要求重做旧A/B/C。
- [ ] 归档实测反馈，区分用户确认、原始日志、主机/离屏和板端量化证据；未测项保持待证。仅用户明确授权时审查相关差异、钩子提交并正常推送，核对远端SHA。

## 计划自查

设计全部行为分别落在Task 1导航/反转、Task 2地图与清理、Task 3交互/生命周期、Task 4构建与板端。五项Review Focus均有对应测试；第二关布局及计划审阅保持显式等待，执行方式沿用本会话逐步实现，不要求用户另选。测试迁移只改变存放位置，原始证据不删除。
