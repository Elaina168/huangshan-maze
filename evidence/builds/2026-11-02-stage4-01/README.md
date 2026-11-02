# 第4阶段正式四关构建

2026-11-02，0.11.0-basic-mechanisms，SDK2.5.1，sf32lb52-lchspi-ulp，normal。build_firmware完整退出0/source_unchanged=true；独立逐项重算当前源码/ASCII快照及三产物SHA与字节数一致。未烧录、提交或推送。

目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-gz9amdju\project\build_sf32lb52-lchspi-ulp_hcpu

| 产物 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 621308 | b3b61b922562680c1fb08c94c7c485beca106a4be5485e964b9312809d9d9e5d |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | 36b02f55309740be689cadcf8632141b31a454ccdcfdd56a71f56134a016d500 |

实际ELF含maze_map_mechanism_trial、maze_model_respawn、flush_finished，排除独立试验map/runner。ptab.h、rtconfig.h、uart_download.bat与0.10.1逐字节相同：A/B仍0x12DB0000、0x12DB8000各32KiB，擦除8KiB；下载仍仅bootloader@0x12010000、main@0x12020000、ftab@0x12000000，无存档镜像。

## 目标资源与调用链

symbols.txt：绘图缓冲234000B@0x2002B100（64对齐），地图工作区36256B@0x20021618；nav11736B、store4408B；新增非重入open工作区images1362B+records1744B=3106B静态，不再压在主线程嵌套栈。机关UI指针数组64B。BSS尾0x20064C8C到RAM终点0x2007FC00的地址尾区110452B，不是rt_malloc实际free。

实际ARM反汇编stack-disassembly.txt：main1120B（36寄存器+24浮点+1060），open56B，save2312B，read_archive40B，decode1168B。初建main→open→save→read→decode同时在栈的已识别帧4696B，8192B主线程栈余3496B供叶函数/库/驱动等；这是调用链预算，不是完整全路径最坏栈证明。旧open双大数组留栈的方案已避免。model_step包含内联机关逻辑，其帧184B，flush_finished仅8B；实际反汇编仍先等待驱动完成再flush_ready及原纯服务。

新旧主机/实际FlashDB/离屏/字体证据见[整阶段汇总](../../stage4-2026-11-02/README.md)。物理手感、真实加载耗时/100ms服务间隔、实际堆栈与v3升级/重启保持待TC-064板端；hardware_verified=false，不恢复B/C。SDK自身既有警告与快照无Git提示原样保留，无SDK源码修改或新增安装。
