# 第8阶段正式固件构建

2026-11-04，0.16.0-watchdog，normal。完整SDK退出0/source_unchanged=true；所有当前源码/ASCII快照及三产物SHA独立核对一致。下载目录：

`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-gk1sfruh\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 649876 | `8ef335dcdc63a7d65361328042f5c9c4db7dd028074d584c13834c891d9da478` |
| bootloader.bin | 58496 | `e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21` |
| ftab.bin | 11288 | `a129bf0fd9da306889b5b99577cd7e1c9e15327187e701f1bb3733a0a975c1b1` |

实际ELF有WDT适配器及强T类型超时回调、三固定图/随机生成/前5/RGB调度；无取消图/试验图/PM定时代喂符号。实际配置WDT启用、启动30秒、PM关闭、主栈8192B，见watchdog-config.txt和symbols.txt。默认idle hook由实际main删除，flush内不喂狗，由主机合同覆盖。

静态RAM436156/523264B，余87108B；这是链接分配，不是运行时free或完整最坏栈证明。draw_buffer仍234000B。保留SDK原警告，不修改SDK/安装工具。

v7及原A/B各32768B、sector8192B保持。下载仍仅bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不含存档bank或擦档。原uart_download.bat字节留存。

两次预检因源码文案/换行更新source_unchanged=false，见[阶段证据](../../stage8-2026-11-04/README.md)，不作为本次通过证据。TC-085真实板端尚待[一轮短测](../../../docs/guides/stage8-use.md)。没有烧录或提交推送。
