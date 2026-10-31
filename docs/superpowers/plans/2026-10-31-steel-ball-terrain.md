# 钢珠与地面对比原型实施计划

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task in the current session.

**Goal:** 做可验证的钢珠惯性和普通/冰面对比原型，供用户短路径比较手感。

**Architecture:** 沿用主线程拥有app/model/LVGL、单IMU后台。地面数据归关卡，model/UI共用球心查询，固定步物理与渲染频率分离。

**Tech Stack:** 现有SiFli SDK v2.5.1、RT-Thread、LVGL v9、C99、WSL GCC与既有Python，不安装依赖。

**Spec:** [本轮设计](../specs/2026-10-31-steel-ball-terrain.md)。用户已认可方向并要求继续，数值为待手感确认的实施候选。

## Global Constraints

- 保留课程清单既存未提交回退；在现有codex/maze-ui执行，不自动提交推送。
- 600 px/s²加速度、120 px/s限速、4.0/0.8 s⁻¹阻尼、1.5 px/s静止阈值均为候选；产品步10ms。
- USB右；只有UI线程操作LVGL；不恢复B/C，不新增中文/选关/正式迷宫或共享供电改动。
- 中文目录通过现有ASCII快照脚本构建，保持UTF-8与现有换行，区分主机/构建/实物证据。

## Task 1：需求、地面与运动逻辑

- [x] 更新REQ-021/022/023与TC-023至027，写真实模型新断言并确认旧行为失败。
- [x] 在maze_level.h/.c定义maze_surface_t、maze_surface_area_t、surfaces/surface_count、maze_level_surface_at和maze_level_surface_lab；矩形半开，未覆盖为普通。
- [x] 在maze_model.h/.c集中候选常量，替换速度覆盖为加速度/阻尼/合成限幅，不改扫掠/最早结算算法。
- [x] 更新旧几何/状态中直接速度模型的预期，保留原有断档/代次/计时/暂停与薄墙圆角覆盖。新增地面跨界、滑行距离、反向/转向、状态清速及10ms固定步20/40Hz对照。
- [x] 运行`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`，预期全部PASS；新增独立inertia_harness由该脚本编译真实源码运行。

## Task 2：对比地图与界面接入

- [x] UI按关卡区域画右半冰面，在PLAYING标题显示球心NORMAL/ICE；main选surface_lab、版本0.5.0-steel-terrain。
- [x] 用真实LVGL离屏测试验证两种地面标签、原活动切页刷新上限和暂停校准失败提示，预期全部断言通过。
- [x] 核对原probe回归、快照测试和材料检查；完整SDK激活后运行build_firmware.py，预期退出0、source_unchanged=true、3项产物哈希一致。
- [x] 独立只读复核最终逻辑与变化；仅针对真实问题修复和相关回归，完成本地证据/日志/使用说明。
- [x] 给用户当前固件与最短手感比较步骤；硬件未操作前不填通过，不恢复正式B/C。

## 当前结果

2026-10-31软件增量及必要验证完成，完整构建/主机/离屏/只读复核均有记录，未提交推送。

- [ ] 用户按steel-terrain-use.md短路径确认真实普通/冰面惯性手感（尚未烧录或实测，不等于本轮自动步骤失败）。
