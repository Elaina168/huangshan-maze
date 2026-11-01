# 0.7.0地图基础完整构建

2026-10-31，SiFli SDK v2.5.1 / sf32lb52-lchspi-ulp，完整build_firmware.py退出0、source_unchanged=true，三项固件齐全。独立重算当前全部25个构建源码与三产物SHA256，均匹配。完整输出attempt-01.log、清单attempt-01-state.json，既有SDK警告保留。

main.bin 591492 B，相较0.6.1增加5536 B，SHA256 `dd1482173b6aedf0fa11ecb5a40e16ee804a28f0107e9eb37afa69ff3b255b38`。
产物目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-7e7tacue\project\build_sf32lb52-lchspi-ulp_hcpu`。

使用本次激活SDK中的arm-none-eabi-nm -S读取实际main.elf：静态加载结果map.2大小0x9f0=2544B，工作区work大小0x23f4=9204B，均为b即BSS，原始输出target-static-symbols.txt。这是目标产物静态布局，不是运行时剩余堆或栈测量；主机sizeof装载结果2576B与目标不同，未混用。

[主机与真实LVGL证据](../../model/2026-10-31-map-foundation-01/README.md)通过，ASan/UBSan边界测试无报告，独立代码复核无重要问题。尚未烧录，启动检查耗时/屏幕和原手感需用户短测；不恢复B/C，不提交推送。
