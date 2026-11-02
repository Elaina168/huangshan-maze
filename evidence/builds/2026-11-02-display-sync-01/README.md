# 0.10.1显示完成同步修复构建

2026-11-02，SDK v2.5.1，sf32lb52-lchspi-ulp，normal，APP_VERSION=0.10.1-display-sync。完整build_firmware退出0且source_unchanged=true，未烧录、提交或推送。

实际目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-ccn8gy48\project\build_sf32lb52-lchspi-ulp_hcpu

| 产物 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 613508 | 946211627277703e28a1c7a54773ae4794bac768e22ed034e455473c42aa92b9 |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | ad2f61a39bb8da182e8bf3110b350e7232a39c864cd9531d4e69df5a68600d3a |

逐项独立重算当前工程/ASCII快照所有源码SHA、三二进制SHA与长度均一致。定位时保存的4项SDK源码SHA未变；生成rtconfig.h/ptab.h/uart_download.bat与0.10.0逐字节一致，原分区/三文件范围未变。正式ELF仍包含第三图、不包含独立试验map/runner。

flush-disassembly.txt证实实际ARM flush_finished先间接调用wait_cb，后flush_ready，再尾调用原服务。实际新增该函数仅8B寄存器栈，未新增静态缓冲；draw_buffer234000B@0x2002A300、work36256B@0x20021478、nav11488B/store4240B及BSS尾0x20063E8C均与0.10.0相同。旧版嵌套存档帧分析不变；没有把静态尾区当真实free。等待真实LCD的运行时长仍须板端日志确认。

软件证据：[异步RED/GREEN及复核](../../ui/2026-11-02-flush-race-01/README.md)。完整构建保留SDK既有警告；本次不改SDK、不改物理/输入阈值或存档。新固件的实屏闪烁/黑块与服务间隔未验证，hardware_verified=false，需用户同路径短测。
