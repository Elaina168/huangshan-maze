# 第二次下载失败：跨盘工作目录

日期：2026-10-10。源码/固件仍为提交 `092062439aca9f248940120bcaeaacc2e3f1be28` 的 v0.1.1。来源：用户粘贴输出摘录，不冒充原始附件。

```text
PS D:\sifli\peripheral_lab>
please input the serial port num:6
com6
[0x00]   Connected success!
[0x01]   Download stub success!Error: Failed to parse file bootloader\bootloader.bin@0x12010000
Caused by:
    系统找不到指定的路径。 (os error 3)
```

串口连接与下载 stub 成功，之后解析固件路径失败，未取得应用固件写入成功证据。不能将 stub 下载算作应用烧录完成。

实际生成脚本使用 `cd %WORK_PATH%`，没有 `/d`。在 D 盘执行时只设置 C 盘的目录，没有切换当前驱动器。只读复现：普通 cd 后当前目录仍是 D 盘，检查相对 bootloader 路径得到 MISSING；`cd /d` 后进入 C 盘实际构建目录，得到 FOUND。三项实际固件重新计算 SHA-256 均匹配构建记录，未重新构建。

Codex 前次指导直接跨盘调用脚本，遗漏了这项工作目录要求。修正操作为 PowerShell 先 `Set-Location` 到实际构建目录，再调用 `./uart_download.bat`；保留原生成脚本和失败记录。用户重试与真机观察待执行。
