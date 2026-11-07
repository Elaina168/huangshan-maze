# 黄山派体感迷宫

李天宇 · 2024211450 · 单人课程项目 · 最终功能版本`0.16.1-ui-trim`。

用户已确认此版并进入封版归档，交付tag为`v0.16.1`。[课程最终材料入口](docs/final/README.md)含需求、设计三图、测试与限制、研制总结、构建使用、答辩提纲和交付清单。正式教师验收/平台提交与未提供的硬件量化结果不因封版自动完成。

产品在黄山派上独立运行，倾斜操控钢珠，触摸完成选关/校准/暂停/重玩/复活；三固定关为冰环迷宫、机关试炼、动力机关，另有随机娱乐。支持惯性、多地面、钥匙开关/门、破墙、检查点、磁力/动态机关、滚动视角、黑暗/反转组合、RGB与WDT。固定关各保存真实前5，随机不记成绩。最终界面已按用户清单精简。

## 查看与复现

- [最终产品需求](docs/final/product-spec.md)、[方案设计](docs/final/design.md)、[研制总结](docs/final/development-summary.md)。
- [最终测试报告](docs/final/test-report.md)：软件/真实库/离屏/构建与用户反馈分开记录。
- [构建、烧录与操作](docs/final/build-and-use.md)：实际SDK基线、ASCII快照构建、下载地址及异常处理。
- [课程标准](docs/course-requirements.md)、[第三方资源](THIRD_PARTY.md)、[开发日志](docs/devlog/)。
- [最终构建证据](evidence/builds/2026-11-07-ui-trim-01/README.md)、[用户最终确认](evidence/ui/2026-11-07-final-confirm/README.md)。

```powershell
python scripts/check_project.py
# 实际SDK环境激活后：
python scripts/build_firmware.py
```

## 目录

| 目录 | 内容 |
|---|---|
| src/ | 传感器、输入/状态、物理/地图、渲染、成绩/存档、反馈与看门狗 |
| project/ | SCons/Kconfig配置及分区表 |
| docs/final/ | 当前课程终稿入口 |
| docs/其他、evidence/ | 原始课程资料、分阶段设计、日志和真实证据 |
| tests/、scripts/ | 纯逻辑、实际SDK库/真实LVGL测试与构建/检查脚本 |

源码仓库忽略SDK、缓存和固件bin。课程最终ZIP另含可烧录固件及完整Git历史，不能仅用GitHub链接或git archive代替。克隆后可配置`core.hooksPath=.githooks`及`commit.template=.gitmessage`，不绕过提交检查。
