# 冰岔迷宫固定关卡实施计划

> **For agentic workers:** Use superpowers:executing-plans to implement this plan in the current session, with RED/GREEN checks before implementation.

**Goal:** 将用户已确认的v2必经陷阱地图接入固件，验证普通与冰面两条绕坑路线。

**Architecture:** 保持现有独立关卡数据、model惯性/连续扫掠、app状态与唯一LVGL主线程。仅新增固定maze_level_frost_fork，启动切换到该实例，旧关卡/对比场保留。

**Tech Stack:** 现有C99、SiFli SDK v2.5.1、RT-Thread、LVGL v9、WSL GCC/Python。不安装工具。

**Spec:** [用户已确认的v2布局](../specs/2026-10-31-frost-fork-layout.md)，精确几何与绕坑参考点在docs/design/maze-layout-2026-10-31/layout.json。

## Global Constraints

- 参数保持0.5.1的180 px/s、600 px/s²、普通/冰阻尼3.0/0.6，不改变判定或积分。
- 350×300、半径7、起点25,25、目标325,275/r14；29墙、2冰区、3坑83,75/183,75/317,175/r8。
- 不生成运行时随机地图，不加选关/存储/音效/中文，不恢复B/C，不提交推送；既存课程清单回退保留。
- 当前codex/maze-ui专用目录继续，ASCII快照构建，UI线程单一所有权。

## Task 1：真实关卡与两条路径

- [x] 新增tests/maze/map_harness.c，先用现开放场复现不满足29墙的RED，再绑定新实例。
- [x] 从确认的JSON转写只读frost_walls/frost_traps/frost_surfaces及maze_level_frost_fork，声明放maze_level.h；生成只发生于开发时，固件不读JSON。
- [x] 用实际C几何重建42单元的通道图，验证42边、7分岔/5死路、终点终端、每个坑所在单元8/10/27的割点性。
- [x] 对两条参考绕坑折线验证完整球体余量；真实model使用加减速输入逐点通过，不能传送或清速度。普通路线无冰、捷径确实经冰，二者均应WON；每个坑可实际触发LOST并冻结。
- [x] scripts/test_maze.py编译运行新harness，与原后台/输入/惯性/几何/状态回归一起PASS。

## Task 2：UI与完整构建

- [x] 真实LVGL离屏测试先验证新地图标题、对象数量和墙/冰区几何，观察旧标题断言RED；实现FROST FORK及新说明，不扩大切页面板。
- [x] 主入口使用新关卡，版本0.6.0-frost-fork，保留原校准/日志/操作及材质标签。
- [x] 真实LVGL保留40000像素活动切换、10000像素普通移动阈值，暂停校准失败提示、材质标题回归PASS。
- [x] 必要相关检查、只读复核、SDK完整构建退出0、source_unchanged和3产物哈希一致；归档失败与通过证据。
- [x] 给用户地图短路径指导与当前固件，不把软件验证当真机通过，不要求重做A或B/C。
