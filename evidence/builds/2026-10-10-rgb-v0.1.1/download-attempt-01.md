# 第一次下载失败记录

来源：用户在 2026-10-10 对话中粘贴的下载输出。以下摘录用于定位，不冒充附件原始日志文件。

```text
please input the serial port num:6
com6

thread 'main' (5696) panicked at sftool-lib\src\sf32lb52\mod.rs:247:14:
called `Result::unwrap()` on an `Err` value: Error { kind: NoDevice, description: "拒绝访问。" }
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
```

对象：v0.1.1 构建，源码提交 `092062439aca9f248940120bcaeaacc2e3f1be28`。下载脚本输入 COM6，打开设备失败，未取得写入成功证据；不标记烧录或 C-06 验收通过。

现场只读检查：COM6 当前列为 `USB-SERIAL CH340 (COM6)`，PnP 状态 OK；VS Code 进程正在运行。尚未确认具体持有串口的进程，不能据进程存在判定其占用。

排查顺序：先停止 CodeKit/其他工具的 COM6 串口监视或连接，再从相同已激活 SDK 的终端重试同一个生成下载脚本。若仍拒绝访问，关闭串口工具后重新插拔板卡，核对实际端口并重试。保留新输出，不覆盖本次失败。无需因这次打开串口失败重新编译或更改源码。
