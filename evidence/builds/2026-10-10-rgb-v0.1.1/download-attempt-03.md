# 第三次下载成功与板端观察

日期：2026-10-10。用户在 PowerShell 先进入本次 C 盘构建目录，再运行 uart_download.bat，输入串口数字 6。源码提交：`092062439aca9f248940120bcaeaacc2e3f1be28`；固件哈希见同目录 build-state.json。

以下为用户粘贴输出的关键行摘录，进度条排版省略，不冒充原始附件：

```text
please input the serial port num:6
com6
[0x00]   Connected success!
[0x01]   Download stub success!
[0x02]   No need to re-download, skip!
[0x03]   Need to re-download
[0x04] Downloaded successfully for 0x12020000..0x120689DB
[0x05]   Verify success!
[0x06]   Need to re-download
[0x07] Downloaded successfully for 0x12000000..0x12002C17
[0x08]   Verify success!
```

结合实际生成的下载文件顺序：bootloader 无需重下被跳过；main 与 ftab 写入成功，并分别报告校验成功。这次已取得应用写入与校验证据。日志未提供整体进程退出码，不补造。

用户现场反馈原话：“板子现在在红绿蓝灯切换”。据此记录板端 RGB 切换已观察；尚未取得启动版本串口日志、亮灭时间测量、复位后重复运行或本人代码解释，C-06 完整验收仍待补齐。本次不涉及任何迷宫产品 TC。

此前失败不覆盖：attempt-01 为串口拒绝访问；attempt-02 为跨盘工作目录。先切换构建目录后，本次成功找到并下载固件；具体串口占用者仍未确定。

原 build-state.json 中 hardware_verified=false 是构建当时状态，保留不回写。后续板端观察以本记录为依据，不将该字段改成所有硬件功能已验收。
