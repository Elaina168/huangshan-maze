# 本人 RGB 修改：构建与待上板记录

日期：2026-10-10。对象：course_starter RGB/串口例程 v0.1.1。关联课程 C-06、C-18、C-21、C-24。

## 修改与协作

用户实际修改 `src/main.c`：APP_VERSION 从 0.1.0 改为 0.1.1，BLINK_ON_MS 从 500 改为 1000，BLINK_OFF_MS 从 500 改为 250，并更新行为注释。Codex 提供指导、核对四处差异、更新来源记录并编译归档，未代替用户编辑源码。源码版本由本记录所在 Git 提交及 build-state.json 的完整源码哈希对应，不补造历史贡献。

## 实际构建结果

- 激活已核对的 CodeKit 环境脚本，在新的 ASCII 快照执行 `scons --board=sf32lb52-lchspi-ulp -j8`，退出码 0。
- main、bootloader、ftab 三项固件存在且非空，逐项重算 SHA-256 与 build-state.json 一致。
- 完整源码快照与当前文件逐项校验一致，source_unchanged=true。
- 生成 Kconfig 严格 UTF-8 解码和 ASCII 检查通过。
- SDK v2.5.1，HEAD `421126d9f476ed8e2a6f0b0ca28a9f241c182e65`。环境沿用基线记录的方法，未安装或修改 SDK。
- main.bin：297436 B，SHA-256 `86cb49b15fdb427b7ca69aa193cc5b3fdadfae29b648b1617230c479747a0236`。原基线 main.bin 为不同哈希，不能混用。
- 原始日志保留 SDK 的 Git 元数据查询、RWX、syscall stub、ftab entry 等警告，不宣称已修复。

## 烧录对象与操作

本次临时工程：`C:/Users/zytx/AppData/Local/Temp/huangshan-maze-fpy3clmm/project`。

其构建目录：`build_sf32lb52-lchspi-ulp_hcpu`。从该目录的 `uart_download.bat` 或 CodeKit 使用对应完整下载配置；脚本提示输入串口数字，COM6 仅在当前确认为 COM6 时输入 6。文件和地址从实际生成的 sftool_param.json 核对，未猜测。

此证据目录归档了配置与脚本，未包含二进制。不能直接在证据目录运行下载脚本，因为它相对查找三项固件；应使用上述实际构建目录中的脚本。临时目录后续可能被清理，必要时从对应提交重新构建。最终交付固件另按 C-27/C-28 归档。

## 待上板

下载、复位、串口 `[course_starter] v0.1.1 boot ok`、红绿蓝顺序、亮约 1 秒/灭约 0.25 秒、重复启动和本人解释均未执行/未取得证据。hardware_verified=false；C-06 尚未形成完整验收链。本次练习不算迷宫产品实现或任何产品 TC 通过。
