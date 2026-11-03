# 成绩返回修复固件

2026-11-03，0.13.1-score-return。完整normal SDK退出0、source_unchanged=true，全部当前源码/ASCII快照/三产物SHA独立一致。相对第6阶段0.13.0仅main、maze_navigation、maze_ui三产品文件变化：保留本图成绩入口的挑战，按原入口返回及按钮文案。固定五图/生成器在ELF，旧滚动试验排除；配置、分区、存档/生成器版本、物理、字形均保持，不扩大资源或重跑无关受控中断。

目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-3tj6_3r5\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 643788 | 674c4c622ee384a9b212680c035559538fcbc78669df63174c7e59ecf7250f9b |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | 8a7c27281b9cc9af4ea42f5622dc69a9f97deab66147864ea970007078e4219b |

实际下载脚本三地址bootloader@0x12010000、main@0x12020000、ftab@0x12000000保持，无擦档或存档镜像。SDK原警告保留于build.log。

[RED/GREEN与真实LVGL](../../score-return-2026-11-03/README.md)。目标板返回短测尚待用户，无自动烧录、提交或推送。
