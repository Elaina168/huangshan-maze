# 第7阶段正式固件构建

2026-11-04，0.15.0-dark-feedback，normal。完整SDK退出0/source_unchanged=true，所有当前源码与ASCII快照、三产物SHA独立一致。下载目录：

`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-2kjec28s\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 646860 | 2aa8fd0164df5466c5fda2a926c8e220d654c48148e571a0c1af0ea13fe9b4d9 |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | 61251c4b0e9a8c1b46603cffa35c2e20ddf70b3f920ce10092cfdd4e5590801d |

实际ELF包含三固定图/随机生成器、统一前5插入与纯RGB调度；无取消两图或试验图符号。v7记录356B，旧读最大982B，原A/B各32768B/sector8192B，下载三地址仍bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不含存档镜像/擦档。uart_download.bat保留SDK原字节。

静态RAM436000/523264B，余87264B；BSS328796B，较0.14.0减少816B。draw_buffer仍234000B，黑暗用四个LVGL矩形，不新建整图或全屏透明图像。主栈8192B保持，实际ARM序言见prologs.txt，不能当全路径最坏栈证明或运行时free值。

保留SDK原LVGL静态分析/RWX/newlib系统调用/ftab入口等警告，不改SDK或安装工具。软件/真实库1405受控中断、231字形、暗区异步2153提交和复核修复见[阶段证据](../../stage7-2026-11-04/README.md)。本版未烧录，TC-081真实板端待[一轮短测](../../../docs/guides/stage7-use.md)，未提交推送。
