# 第1阶段地图与参数基础实施计划

> **For agentic workers:** Use superpowers:executing-plans in this session. User authorized implementation; do not ask for execution method again. No commit/push until requested.

**Goal:** 保持冰岔布局/手感，将地图改为板端装载的字符串和对象表，并集中参数、执行静态合法性检查。

**Architecture:** 新maze_map.c/.h负责加载边界及保守5px路径检查；maze_map_frost_fork.c只放定义。加载结果静态持有，向现有model/app提供const几何视图，UI以稳定字符串ID识别。加载失败走独立中文页面，不初始化无效模型。

**Spec:** [已批准设计](../specs/2026-10-31-map-foundation-design.md)。C99/现有SiFli SDK与LVGL，无安装。

## Global Constraints

最大7×6格、格50px、墙8px、检查步5px；原运动/校准宏名和值不改。保留旧关卡和开放场；本关移除重复生产坐标。只做本阶段，不加选关/机关/随机生成，不改SDK或既存课程清单，不恢复B/C、不自动烧录或推送。

## Task 1 数据与加载

- [x] tests/maze/map_load_harness.c先以新接口编译，记录缺失实现的RED。
- [x] maze_config.h移入现有宏及有限地图容量；各旧头include配置，不改行为。
- [x] maze_level_t加稳定id，旧图赋值；新增maze_map_definition_t（id/列行/字符串行数/对象表及数量）、START/GOAL/TRAP对象、maze_map_t（level/固定墙冰区陷阱数组/checked_nodes）。公开maze_map_load与maze_map_workspace_size。
- [x] 严格检查地图维度/字符位置/封闭边界/对象与几何，返回FORMAT=-2001、OBJECT=-2002、GEOMETRY=-2003、NO_ROUTE=-2004或0；输入结构指针及字符串存储须由调用方提供有效生命周期，错误不会被吞掉。
- [x] 以静态4331项uint16队列+位图执行5px BFS；轴向线段到墙的最短距离按球半径判断，球心段到坑按坑半径判断，外边界与终点按真实几何。
- [x] 测试正确几何/ID、重复加载、尺寸和行长/非法字符/未封闭、缺失/重复/未知对象、NaN/半径/越界/非格心/重叠/埋墙、断路/坑堵路/边段切坑、旧图/开放场回归。JSON生成独立测试fixture，比较全部29墙顺序与对象/冰区。

## Task 2 接入与可烧录验证

- [x] main静态加载结果，记录返回值、节点、工作区sizeof和tick耗时；成功后app初始化，版本0.7.0-map-foundation。失败显示独立中文错误页、继续UI服务、复位重试。
- [x] UI全部原地址判断换为稳定ID。map_harness与真实LVGL harness从真实loader初始化，保留路线及刷新测试；增加独立错误页进程用例，不渲染无效地图。
- [x] 重新生成必要字体，字集/实际LVGL解码像素及原中文/暂停提示回归。主机测试导出实际C几何与设计JSON比较，重复加载不搬迁结果。
- [x] 运行model/probe/native LVGL/快照/check_project；独立只读复核完整增量，修复重要发现后相关回归。
- [x] 冻结源码、SDK完整构建，退出0/源码未变/三产物及源码SHA独立匹配，归档。板端仅新装载日志/地图及操作短测，真实结果待用户反馈。

## Review Focus

检查队列/位图实际静态存储；生成视图内部指针生命周期；窄坑在格点间的连续边安全；所有原UI指针判断均替换；错误页无需有效地图且所有新中文被字形覆盖。用例分别纳入对应任务。固定采样只证明找到的安全路径，不是连续空间完备求解。
