# 项目工程与 Git 管理建立计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将已确认的课程标准、需求和证据纳入可构建的最小工程，建立 GitHub 仓库并推送真实历史。

**Architecture:** 从课程脚手架原样导入 project/ 和 src/ 的最小 RGB/串口基线，产品代码以后按需求分步加入。文档、原始证据与源码共用仓库；构建使用纯英文临时目录，保留构建日志与源码快照校验记录。

**Tech Stack:** SiFli SDK v2.5.1、RT-Thread、Python 3、Git、GitHub；板型 sf32lb52-lchspi-ulp。

**Spec:** [课程标准](course-requirements.md)、[已审阅的需求范围](product-requirements.md)。本计划仅建立工程，不实现迷宫。

## Global Constraints

- 用户已授权本次建立工程、创建 GitHub 仓库和推送；直接在当前工作区执行，不另建工作树。
- course-requirements.md 作为开发标准；需求范围与玩法已确认，性能目标仍需预研。
- 不补造过去提交、硬件实测或教师审批；不自动烧录本次基线。
- 已有课程工程、资料与证据保持原字节，导入校验记录来源和 SHA-256。
- 新文本 UTF-8，新增代码注释中文；SDK API、板型与配置从实际材料确认。
- 建库默认私有，名称 huangshan-maze；不覆盖现有仓库，不强推，不提交凭据或整个 SDK。

## Review Focus

- 中文路径：新构建不能再次把中文路径写入 Kconfig。
- 原样示例：成功编译不能被说成迷宫已实现或个人修改验收已完成。
- 证据真实性：原始编译/烧录日志与用户口头反馈分别标注。
- Git 身份：使用本机既有身份，GitHub 账号必须与连接器确认结果一致。
- 完整归档：仓库文件引用可移植，源码导入有许可和校验值，生成目录和凭据排除。

## Task 1: 建立可核对的工程基线

- [x] 原样导入 project/ 六个骨架文件、src/ 两个文件及 Apache 许可；核对 SHA-256。
- [x] 归档两份 PDF、两份已有原始日志，记录来源清单；整理用户现场反馈。
- [x] 建立 README、AGENTS、开发日志、第三方清单、项目计划、风险记录和 Git 规范。
- [x] 更新需求状态：记录用户确认，保留性能和校准参数的预研边界。
- [x] 运行文档/证据检查；核对基线 src/main.c 未冒充新增功能。

## Task 2: 构建与提交验证

- [x] 新建构建入口：复制人工维护的 project/src 到 ASCII 临时目录后运行 scons；保留日志、源码快照哈希和结果。
- [x] 在实际 CodeKit 生成环境脚本激活后构建；验收退出码、main/bootloader/ftab 产物及生成 Kconfig 路径；普通 export 的权限问题另记。
- [x] 在构建记录中保留 SDK 警告，不宣称上板通过。
- [x] Git init、配置本地身份/钩子，核查暂存差异和敏感信息；真实提交本次文档及基线。

## Task 3: 创建远程并同步

- [x] 验证账号 Elaina168 与目标仓库不存在，创建私有仓库。
- [x] 配置 origin，推送 main；验证远端提交与本地一致。
- [x] 设置持续检查，查看第一次远端检查结果；记录未完成的产品工作。

## 执行记录

- 用户 2026-10-10 已明确授权执行本计划范围，不重复请求建库或推送许可。
- 源码与 PDF/日志导入完成并逐字节校验。
- 尝试在 D:/sifli 创建 Junction 构建入口被系统拒绝；未更改目录权限，改用 ASCII 临时构建快照，不要求管理员权限。
- 首次真实提交 d60a1de 已推送 main，远端 SHA 与本地一致；GitHub 确认仓库私有，首次 Project checks 成功。构建脚本审查问题经失败复现与 4/4 回归修复。

未来迷宫实现计划在关键技术预研和三图设计后另行制定，本次授权不冒充未来所有功能已验收。
