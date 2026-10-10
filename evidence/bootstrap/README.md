# 前期外设示例证据

记录日期：2026-10-10。下列文件由用户在本次对话提供，原字节保存，哈希见 docs/references/import-manifest.json。

- peripheral-build.log：peripheral_lab 编译及索引生成退出码 0。
- peripheral-build-download.log：包含同工程编译、索引及 COM6 下载，启用 --verify，Download 退出码 0。
- board-feedback.md：用户对屏幕、触摸和 IMU 的简短现场反馈，不是原始串口测量。

原工程是 D:/sifli/peripheral_lab，SDK v2.5.1，板型 sf32lb52-lchspi-ulp。烧录包括 bootloader、main、ftab。COM6 只代表该次操作，不作为未来固定串口。

之前用户日志中的 Malformed utf-8 发生于含中文的下载目录；实际生成 Kconfig 严格 UTF-8 解码失败，GBK 解码能恢复中文路径。其后英文路径构建成功。本记录是事后整理的故障摘要，不冒充新执行的构建。

不能推出：本仓库代码已上板、迷宫已实现、IMU 校准/精度/延迟合格、个人修改验收通过。
