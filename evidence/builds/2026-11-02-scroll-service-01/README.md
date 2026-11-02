# 0.9.2滚动时序修复构建

2026-11-02；SiFli SDK v2.5.1、sf32lb52-lchspi-ulp，显式scrolling-trial。完整scons退出0，source_unchanged=true；独立核对当前44项源码和三产物SHA通过。未烧录。

构建目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-945btk4p\project\build_sf32lb52-lchspi-ulp_hcpu`。

main.bin：596516B，SHA-256 `056490bdcc6297cc6e1df4a3953464d4e81b4690847ccb103bc464d8d8fd51f6`。比0.9.1增加256B。

实际ARM ELF包含maze_ui_set_trial_service与service_during_flush，work36256B；maze_store_open/save不存在。默认下载仍是bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不带档案镜像。详见target-checks.txt及build-state.json。

本次未重建正式固件，正式UI当前源码主机编译/回归通过，试验回调在宏分支中隔离；回退使用前次已核对的正式固件，不能把旧正式构建描述成当前源码再次完整构建。既有SDK警告保留在build.log，未宣称消除。目标时序/帧率/手感需真实复测。
