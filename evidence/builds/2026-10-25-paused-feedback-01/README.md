# 0.4.3暂停校准提示构建

2026-10-25，板型sf32lb52-lchspi-ulp、SiFli SDK v2.5.1。沿用已有SDK导出环境执行`python scripts/build_firmware.py`，完整退出0，source_unchanged=true，全部构建源码和3项固件SHA-256独立复算一致。

版本0.4.3-pause-feedback，仅UI保留暂停校准失败反馈及main版本号变化，未修改app判据、实时错误/恢复条件或SDK。main.bin为568964 B，SHA-256 `641f9134aeb5d0a11bb5cf353b1e724c0f9b34815bf81cf1d8257238c8480f5a`。三产物完整清单及源码哈希在`attempt-01-state.json`，原始构建输出`attempt-01.log`。应用无编译诊断；原SDK LVGL潜在空指针、RWX、syscall stub与ftab entry等警告保留，不声称已消除。

ASCII工程路径：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-y8f018j0\project\build_sf32lb52-lchspi-ulp_hcpu`。未烧录、未从设备读取版本或固件哈希；[必要回归](../../ui/2026-10-25-paused-feedback-fix-01/README.md)通过不能替代屏幕实测。用户已停止B/C，不要求重新执行完整A。无提交推送。
