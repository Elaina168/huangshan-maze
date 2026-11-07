# 界面精简候选版构建

2026-11-07，0.16.1-ui-trim，normal；SDK v2.5.1完整构建退出0/source_unchanged=true，当前完整源码集合、ASCII快照及三产物SHA独立一致。构建完成后进入板端使用确认，最终版本于2026-11-07确定。

下载目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-s0fru_k2\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 648804 | `7140d68541ee4d0a8463bb0fdffa4edad04e2954d367e13639b1b19a24ddcb2d` |
| bootloader.bin | 58496 | `e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21` |
| ftab.bin | 11288 | `d251361a24587c403b1bc813b602e2deabf2202848176beb99c9c2138c5a5dce` |

仅删除指定UI文案、无用标签/地面缓存/查询/计数循环并裁剪字体，v7档案与分区不改；226字形、27135位图字节（减少5字形/640字节）。实际ELF不含shown_surface，保留三关和看门狗。静态RAM436156/523264B、余87108B，不是实际free或完整最坏栈证明。

仅下载bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不含存档bank或擦档。保留SDK既有警告，不修改SDK。软件/显示/字体结果见[本轮证据](../../ui/2026-11-07-ui-trim-01/README.md)，板端待[复测](../../../docs/guides/ui-trim-use.md)。程序标签v0.16.1，当前材料标签v0.16.1-docs.1。
