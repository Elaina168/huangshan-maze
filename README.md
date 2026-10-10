# 黄山派体感迷宫

单人课程项目，目标是在黄山派上通过倾斜控制小球避开墙壁和固定陷阱，到达终点。使用触摸完成校准、暂停、继续、重玩和退出。

**当前状态：本人修改与预研阶段。** 已确认选题和第一版玩法，已整理课程标准、需求与验收计划。`src/main.c` 基于课程串口/RGB 例程，用户已修改版本与闪烁节奏，构建通过，板端验证待执行；迷宫尚未实现。本仓库首次导入此前的真实资料和记录，不补造历史提交。

## 开发标准

- [课程要求与检查清单](docs/course-requirements.md)：用户指定的开发标准。
- [产品需求规格](docs/product-requirements.md)：已确认范围及仍需预研的数值。
- [验收测试计划](docs/acceptance-test-plan.md)：需求-用例关联，当前产品用例未执行。
- [工程建立计划](docs/project-setup-plan.md)、[项目进度](docs/project-plan.md)、[风险记录](docs/risks.md)。
- [开发规范](AGENTS.md)、[Git 工作流](docs/guides/git-workflow.md)、[第三方清单](THIRD_PARTY.md)。
- 当前下一步：[本人修改练习](docs/guides/first-personal-change.md)、[平台接口核对](docs/research/platform-interface-audit.md)。

## 目录

| 路径 | 职责 |
|---|---|
| project/ | SiFli SCons/Kconfig 人工维护的工程配置 |
| src/ | 应用源码，目前仅导入的课程基线 |
| docs/ | 课程标准、需求、设计/测试材料、开发日志与课程 PDF |
| evidence/ | 原始日志、用户反馈与后续真实测试证据 |
| scripts/ | 文档/提交检查、ASCII 快照构建 |
| .githooks/、.github/workflows/ | 提交规范与远端文档检查 |

## 环境与检查

课程基线：SiFli SDK **v2.5.1**，板型 **sf32lb52-lchspi-ulp**，RT-Thread；后续迷宫 UI 使用 LVGL v9。另需 Git、Python 3.8+ 和 SDK 提供的工具链。SDK 不随仓库分发，按课程 PDF/官方文档安装。

```powershell
python scripts/check_project.py
```

克隆到其他机器后，在项目根目录设置钩子与提交模板：

```powershell
git config core.hooksPath .githooks
git config commit.template .gitmessage
```

## 编译

在 PowerShell 激活本机 SDK 后，从项目根目录执行：

```powershell
# SDK 路径替换为本机实际位置；不要复制整个 SDK 到仓库。
$env:SIFLI_SDK_TOOLS_PATH = 'D:\sifli\tools'
. D:\sifli\SiFli-SDK\v2.5.1\export.ps1
python scripts/build_firmware.py
```

仓库路径可能包含中文，构建脚本会将人工维护源码复制到纯英文临时目录，校验复制前后字节，随后执行 `scons --board=sf32lb52-lchspi-ulp -j8`。完整日志和构建状态存于被忽略的 `build/`，状态记录含临时工程路径、源码哈希与固件哈希。源码有修改时必须重新构建。

本脚本只编译，不烧录。临时工程可在 CodeKit 中打开，下载时使用它生成的完整配置并确认实际串口；不要只猜单个 bin 地址。最终固件和配置按课程交付要求另行归档。

上面的 SDK 和工具根路径均是本机示例，其他机器按实际安装位置替换。`export.ps1` 还需 `uv` 位于 PATH；CodeKit 已激活的终端通常已配置。如操作系统禁止 SDK 状态写入，应使用 CodeKit 的已激活终端，不修改全局权限。此次实际环境处理和构建结果见 [首次构建记录](evidence/builds/2026-10-10-baseline/README.md)。

## 已有证据与限制

[已有外设示例记录](evidence/bootstrap/README.md)包含成功编译、烧录日志和用户反馈。它们属于 `peripheral_lab`，不证明本仓库的 RGB 基线或迷宫已上板。个人最低修改验收、方向映射、校准、手感和产品验收尚未完成。

课程 PDF 用于本课程开发与核对，源代码许可见 [第三方清单](THIRD_PARTY.md)。不得将课程资料或未知许可素材误标为本项目原创或统一 Apache 许可。
