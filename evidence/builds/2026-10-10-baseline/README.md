# 最小工程首次构建记录

日期：2026-10-10。对象：从 course_starter 原样导入的串口/RGB 基线，不是迷宫。

## 结果

- `scons --board=sf32lb52-lchspi-ulp -j8` 退出码 0。
- main.bin、bootloader/bootloader.bin、ftab/ftab.bin 存在且非空，哈希与大小见 build-state.json。
- 复制的 8 个 project/src 文件与仓库源码 SHA-256 一致，构建完成后再次核对未变化。
- 生成 Kconfig 能严格按 UTF-8 解码，内容路径为 ASCII，未出现此前中文路径问题。
- SDK v2.5.1，HEAD `421126d9f476ed8e2a6f0b0ca28a9f241c182e65`；交叉编译器 arm-none-eabi-gcc 14.2.1。
- 静态构建表：ROM used 323132 B，RAM used 56672 B，main binary 297436 B。不是运行时堆、任务栈或迷宫资源预算。
- 未烧录，hardware_verified=false；不证明个人修改验收或板端功能。

## 环境处理

普通终端未携带 CodeKit 的 uv 路径和 SIFLI_SDK_TOOLS_PATH，直接 export 时分别报告 uv 缺失和环境实例无效。补上实际工具根 D:/sifli/tools 后，兼容性检查通过，但更新 SDK 状态文件被操作系统拒绝写入。

本次未重装 SDK、未修改权限或 SDK 文件；执行 CodeKit 在用户成功构建时生成的现有环境脚本。已核对脚本记录的 SDK HEAD 与当前 HEAD 相同，工具和 Python 环境位置来自该脚本，随后在新建 ASCII 临时快照目录构建。其他机器优先使用 CodeKit 已激活的终端，或按其实际工具根激活环境，不复制本机的临时脚本路径。

## 保留的警告与边界

原始 build.log 保留了 LOAD 段 RWX、newlib 的 _close/_lseek/_read/_write stub 和 ftab entry 警告。构建快照不是 Git checkout，SDK 的版本查询也打印 not a git repository；原始源码由快照哈希而非临时目录 Git 元数据追溯。本次这些消息未阻止 scons 成功，不表示它们已修复。

build-state.json 的绝对路径仅记录本机那次构建位置，临时目录可能被系统清理；不是最终交付固件位置。冻结版本时需按课程 C-27/C-28 重新构建、烧录、实测，并另外归档固件与完整下载配置。
