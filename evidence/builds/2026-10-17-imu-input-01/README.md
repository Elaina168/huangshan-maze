# 校准与控制输入构建记录

日期2026-10-17，版本0.2.2-imu-input-probe，板型sf32lb52-lchspi-ulp，SDK v2.5.1。当前代码基于b508f19c72138715c746f72e8794deda31091b26之上的本地未提交改动，源码版本以构建清单中的哈希识别，不把基础提交当作当前完整源码。

## 主机验证

[后台与校准输入输出](host-maze.log)包含后台11个注入场景和纯C输入8组断言；[原探针回归](host-probe.log)包含所有权守卫及原7+14场景，均退出0。另执行构建快照4/4测试。测试前新增模块不存在，编译按预期失败；补实现后全部通过。

输入断言覆盖19/20样本、序号去重、950ms窗口、范围300/301、3秒超时、USB右死区符号、年龄100/101、间隔100/101、旧代次、跳号重收、时钟回绕、100Hz时基换算、失败不替换参考、读错误、取消失效、短跨度不通过及迟到结果不能赶在截止后提交。它们是主机逻辑证据，不是显示负载下的真实校准或物理响应验收。

## 第一轮：编译成功但源码一致性失败

[第一轮输出](attempt-01.log)、[第一轮状态](attempt-01-state.json)保留原字节。scons退出0、三项固件存在，但脚本退出1、source_unchanged=false，不交付该产物。

原因：Codex以Python写main.c时默认换行为CRLF，而原文件使用LF且Git设为-text；差异检查因此报告行尾空白。构建期间恢复了LF，导致源码字节与启动快照不一致。已独立验证快照main.c将CRLF替换为LF后与当前文件逐字节相同，逻辑未变；仍严格按源码哈希策略另做完整构建，不篡改状态或放宽检查。换行修复后git diff --check通过。

## 第二轮：当前源码完整构建通过

[最终日志](attempt-02.log)、[最终源码/固件清单](attempt-02-state.json)、[ELF静态段统计](elf-size.txt)。脚本与scons退出0，source_unchanged=true；构建后独立重算当前源码与三项固件SHA-256，全部匹配。快照 `C:/Users/zytx/AppData/Local/Temp/huangshan-maze-j146p3u0`，下载目录为其下project/build_sf32lb52-lchspi-ulp_hcpu，未执行烧录。

| 产物 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 562832 | 7edabee8f8db817cb11972fbf1c188a805aeee5affcbb7c0c75b1f819d498655 |
| bootloader/bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab/ftab.bin | 11288 | e1b85f4f9598522d3649cbcf8b4567cf76d99216465ffde6bcb4189c9dc40713 |

ELF text558620/data4208/bss93784 B；这些是静态段，不是运行堆栈余量。SDK仍有lv_obj_swap潜在空指针、RWX、ftab syscall/entry与dfu缺失提示，日志保留；本项目新增/修改的应用源未出现编译诊断，不宣称SDK警告清零。

## 尚待硬件验证

用户尚未运行本版，不把上一版后台采样31条error0和线程栈峰值当作本版校准成功。按[校准输入指导](../../../docs/guides/imu-input-probe.md)只做一轮基本操作，原始四向日志不重复采集。球心漂移、可见运动、正式稳定性与全部产品TC仍未执行。
