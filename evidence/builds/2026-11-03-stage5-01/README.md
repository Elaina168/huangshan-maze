# 第5阶段正式构建

2026-11-03，0.12.0-dynamic-mechanisms，SDK2.5.1/GCC14.2.1，sf32lb52-lchspi-ulp。完整normal构建退出0、source_unchanged=true；另逐项独立核对当前源码SHA与ASCII快照及三产物SHA一致。未烧录、无本版板端结果。

目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-wwuqdnqg\project\build_sf32lb52-lchspi-ulp_hcpu

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 631540 | 41b10bd3de16b9c5c05441f836419f6e2226e20df01009904866c59fbb748429 |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | b1795fe5f199546e411a4bfcec004e99ecdf1fdabf82e679e0087e8a28bc4207 |

实际ELF含maze_map_dynamic_trial，不含maze_scroll_trial_run/maze_map_scroll_trial。原分区、A/B每bank32KiB和8KiB擦除粒度不改，无清档镜像；实际三文件下载地址沿用当前生成配置。

静态目标：draw_buffer=234000B、work=36256B；store_open images=1690B、records=2176B均静态；.bss=306956B、.data=4156B。不能将主机堆或静态地址尾区视为板端可用堆。

ARM实际序言见target-prologs.txt：main=1336B（寄存器36+浮点24+局部1276），store_save=2856B，read_archive=40B，archive_decode=1384B；这一可识别存储链为5616B，主栈8192B。未包含实际库/驱动等所有最坏链，需新固件ps高水位；不是全路径最坏栈证明。model_step=176B、mechanism_motion=288B（含保存浮点寄存器），仍需实际运行资源与服务间隔。

原SDK RWX/syscall stub/版本查询等构建输出原样保留，不宣称消除。当前SDK离线export实例校验不匹配，未安装或修改SDK，复用既存sdk-env-2xt_ho3q.ps1激活的已装工具完成构建；构建记录含实际路径/配置。详见build-state.json和build.log。
