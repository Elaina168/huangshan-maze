# 0.9.0成绩与存档完整构建

目标sf32lb52-lchspi-ulp，SDK v2.5.1。未烧录，无本版板端证据。

attempt-01.log/state保留首轮完整编译后的链接失败：custom_mem_map.h两行C声明进入link_copy.lds。只在实际FAL_CFG包含环境声明代理，ARM预处理回归见header-regression.txt；不修改SDK或生成链接脚本。

attempt-02.log完整构建退出0，attempt-02-state.json为source_unchanged=true；39份源码和3产物SHA已独立重算全部匹配。main.bin609108字节，SHA-256 `f1950f202e00c21982d3ef6e519c7d5c3114839fa133af14b85aebcbe30dbf5b`。

烧录目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-wgrc2ra3\project\build_sf32lb52-lchspi-ulp_hcpu`。

target-checks.txt核对实际配置FlashDB/FAL/8192/缓存16、A/B地址及32KiB容量、主程序原地址、链接脚本无C声明，默认下载仅bootloader/main/ftab且无游戏区镜像。实际ARM ELF静态nav3416B、store4080B、路径work9204B；这些不是运行时堆/栈峰值。target-fal-table.json从实际ELF读取4项FAL表，确认原dfu/ble地址/flash2设备不变，maze_a/maze_b映射代理相对0/32768，各32KiB。

原SDK警告保留。当前电脑未识别COM6，仅蓝牙COM3/4；需用户连接板子确认实际端口并关闭串口监视器后再烧录，不自动操作设备。
