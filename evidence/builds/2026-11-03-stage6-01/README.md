# 第6阶段正式固件构建

2026-11-03，0.13.0-random-maps，normal变体。完整SDK退出0、source_unchanged=true；当前全部源码与ASCII快照、三产物SHA-256独立一致。未烧录。构建目录：

`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-2dojn963\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 643700 | c13286d3087070611c0a0d05bb7464b5ec2d170e2b67bf759e23c6460a3b2d58 |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | bd6909670c7ff7c52688980f69109c178452f85524874571469ecd0746230ba1 |

## 目标资源与范围

实际ELF含固定五图及生成器，无scroll_trial图/runner。BSS331916B，相对0.12.1的306956B增加24960B。构建摘要RAM523264B、静态Used439120B、余量84144B；不是运行时free值。nav静态31672B，含双地图/双生成定义；work40522B，只有一个共享校验工作区；draw_buffer234000B保持视口尺寸。静态读档双image1964B、records2544B，未引入全图图像缓冲。

8192B主栈保持。实际ARM序言：main1464B，store_save3312B，read_archive40B，archive_decode1544B，已识别读回/解码主干帧合计6360B，未含内部小函数、异常现场及所有SDK最坏调用；开档帧另56B。generator_step232B、map_step112B；大地图/生成/队列存静态，不堆放在主栈。目标free/ps仍待实测，不能把序言相加称为完整最坏栈证明。见symbols.txt与prologs.txt。

原分区/A-B各32768B/8192B扇区保持，partition.log验证通过，下载脚本仍为bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不含存档镜像或擦档。982B v5成功保存后旧固件不支持读取。

## 失败与最终核对

首次编译链接退出0，但期间最终同步字体，source_unchanged=false，按构建脚本整体判失败；该中间产物不交接。保存intermediate-build-state.json及intermediate-build.log，之后冻结源码重新完整构建，最终状态如上。保留SDK已有LVGL空指针分析、RWX、newlib未实现系统调用/ftab入口等警告，未修改SDK或安装工具。

主机软件、实际SDK库受控中断、真LVGL和工程材料检查见[阶段记录](../../stage6-2026-11-03/README.md)。TC-075真实板端仍待[指南](../../../docs/guides/stage6-use.md)，未提交或推送。
