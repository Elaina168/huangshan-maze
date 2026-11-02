# 0.9.3小球绘制修复完整构建

2026-11-02，SiFli SDK v2.5.1、sf32lb52-lchspi-ulp，scrolling-trial。完整构建退出0、source_unchanged=true；独立核对44源码及三产物SHA通过。未烧录，未提交推送。

目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-a2islhhs\project\build_sf32lb52-lchspi-ulp_hcpu`。
main.bin 597068B，SHA256 `db4a049dabc69203b8d241442b5329c76b96ee75cf8aa7e2e5c03cd4941adfdd`，比0.9.2增加552B。

实际ARM ELF：trial_draw_buffer位于0x20028f40，大小234000B，64B对齐；bss_end=0x20062acc，RAM_end=0x2007fc00，未分配尾区119092B，尚未扣运行分配，不能写作板端可用堆。保留service_during_flush，maze_store_open/save不存在。默认下载仍是原三文件/地址，无档案镜像。详见target-checks.txt与build-state.json。

仅重建试验固件。当前正式UI主机编译/正常及地图错误页回归通过，不冒充正式固件再次完整构建。既有SDK警告保留。大单块时序、实际LCD观感和free/ps资源仍待[板端短测](../../../docs/guides/scrolling-map-trial-use.md)。
