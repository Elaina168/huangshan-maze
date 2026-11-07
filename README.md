# 黄山派体感迷宫

李天宇 · 2024211450 · 单人课程项目 · 程序版本`0.16.1-ui-trim`。

项目实现了运行在黄山派上的离线体感迷宫。玩家通过倾斜控制钢珠运动，使用触摸屏完成选关、校准、暂停、重玩和检查点复活。固定关卡包括冰环迷宫、机关试炼、动力机关，另提供随机挑战娱乐模式。

## 功能与实现

- 钢珠惯性、多种地面阻力、墙壁碰撞、陷阱和终点判定。
- 钥匙与开关、门、可破坏墙、检查点、磁力区和动态障碍。
- 大地图滚动视角，黑暗和方向反转四种玩法组合。
- 固定关卡前5排行榜、关卡解锁、Flash存档和旧版数据迁移。
- 中文交互、RGB反馈，以及主循环硬件看门狗。

## 课程验收材料

从[验收文件索引](docs/final/review-index.md)开始查看，索引同时列出GitHub和本地文件位置。

| 材料 | 内容 |
|---|---|
| [产品需求规格](docs/final/product-spec.md) | 产品场景、最终功能和验收追踪 |
| [方案设计](docs/final/design.md) | 系统架构、数据流和状态机三图 |
| [测试报告](docs/final/test-report.md) | 测试方法、结果、问题修复和专项验证计划 |
| [研制总结](docs/final/development-summary.md) | 研发过程、关键技术、个人工作与开发协作 |
| [构建与使用](docs/final/build-and-use.md) | 环境、构建、烧录和操作步骤 |
| [交付清单](docs/final/delivery-checklist.md) | 固件、完整Git历史、ZIP与校验文件 |
| [答辩提纲](docs/final/defense-notes.md) | 实物演示与代码讲解安排 |

程序标签`v0.16.1`，当前材料标签`v0.16.1-docs.1`。[构建记录](evidence/builds/2026-11-07-ui-trim-01/README.md)提供固件校验值，[最终回归](evidence/release/2026-11-07-final/README.md)提供自动验证记录。

## 构建与工程目录

激活SiFli SDK v2.5.1环境后，在工程根目录运行：

```powershell
python scripts/check_project.py
python scripts/build_firmware.py
```

| 目录 | 内容 |
|---|---|
| src/ | 采样、状态、物理、地图、渲染、成绩、存储与看门狗 |
| project/ | SCons/Kconfig工程配置及分区表 |
| docs/final/ | 课程终稿和验收索引 |
| docs/、evidence/ | 需求演进、设计、开发日志和验证记录 |
| tests/、scripts/ | 自动测试、构建与工程检查 |

课程交付包为`黄山派体感迷宫-2024211450.zip`，包含Git工程、完整项目历史备份、可烧录固件和课程材料。SDK与工具链按[构建说明](docs/final/build-and-use.md)配置。
