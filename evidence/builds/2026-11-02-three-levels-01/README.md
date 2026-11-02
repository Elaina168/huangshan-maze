# 正式三关构建与目标核对

2026-11-02，版本0.10.0-three-levels，sf32lb52-lchspi-ulp，SDK v2.5.1。执行python scripts/build_firmware.py --variant normal完整退出0，source_unchanged=true；独立重算每项当前源码/ASCII快照及3个二进制SHA全部一致。无烧录、提交或推送。构建状态hardware_verified=false。

目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-wbool1zd\project\build_sf32lb52-lchspi-ulp_hcpu

| 产物 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 613460 | f78b41abd46b9ecd106064feb8df55e887b5275a9bb5cee4939a590a8c1427b9 |
| bootloader/bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab/ftab.bin | 11288 | 2f1268a37b60bb7853ee03c8d83148c301a2b858c5e2734552a4af9957ea5ba8 |

实际ELF含ice_loops、maze_ui_set_service、service_during_flush和maze_store_open/save，排除maze_map_scroll_trial与maze_scroll_trial_run。生成ptab.h的A/B位置仍0x12DB0000/0x12DB8000，各0x8000；配置擦除8192。实际下载脚本仍仅bootloader@0x12010000、main@0x12020000、ftab@0x12000000，无游戏档案镜像。项目FAL逐bank写闸门和实际NOR代理未改。

实际ELF符号：draw_buffer@0x2002A300，234000B且地址64对齐；work@0x20021478，36256B；nav.2=11488B，store.1=4240B。BSS尾0x20063E8C，RAM终点0x2007FC00，未分配地址尾区114036B。这不是rt_malloc实际free，不扣除动态LCD/LVGL/FDB分配，更不能当板端资源通过。

## 栈预算检查

由同一实际ARM ELF的push/vpush/sub sp反汇编读取，生成RT_MAIN_THREAD_STACK_SIZE=8192。主main帧888B（36寄存器+24浮点+828）；store_open2400B；store_save1768B；read_archive40B；decode952B。初次空档路径main→open→save→read_archive→decode的已识别同时存在帧合计6048B，距8192剩2144B，另需考虑叶函数/库/驱动；不是拿open单帧当总峰值。普通旧档读回路径少一层save，直接保存路径少一层open。

stack-disassembly.txt、stack-vendor-disassembly.txt同时保留实际main、库入口及内部读写调用。显示回调经实际56B服务帧推进navigation/app/model，不触发存档嵌套；实际model帧160B。通过静态应用层预算检查，不宣称已证明所有库间接调用、异常压栈的全路径最坏界；TC-058必须看本版main的ps高水位，不能沿用旧42%。

完整build.log保留SDK自身lv_obj_tree等已有警告与无Git快照提示，不修改SDK。未发现本次项目源码编译错误。主机/离屏/构建与真实板端分别记录，板端资源与实际加载/physics_gap仍待短测。
