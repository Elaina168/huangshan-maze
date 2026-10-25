# Task 5状态与计时完整构建

日期：2026-10-25，版本0.3.0-state-core；基线提交52346af加本地Task 4/5改动，尚未提交。SDK HEAD 421126d9f476ed8e2a6f0b0ca28a9f241c182e65，v2.5.1，板型sf32lb52-lchspi-ulp。

使用现有SDK导出脚本激活后运行python scripts/build_firmware.py；脚本/scons退出0，source_unchanged=true，三项固件存在，结束后独立重算全部构建源码、测试源码记录及固件哈希匹配。完整日志attempt-01.log、源码/固件清单attempt-01-state.json、ELF容量elf-size.txt。main.bin为566560 B，SHA-256 1948f7d852d87d254e082c8274b6b497c0ef49e8e90f6e377acffd197d9f94d6。

应用源码无编译诊断；SDK原有LVGL潜在空指针、RWX、ftab syscall/入口和dfu提示保留，不宣称SDK无警告。main初始化实际关卡，按新状态/事件/采样请求接口调用app，原预研页面仅适配状态。本次尚无完整游戏UI，不要求用户烧录此中间版本，未进行板端验收。

主机后台11、校准8、模型7、状态7组与旧探针7+14/所有权守卫、快照4/4均通过；见[状态证据](../../state/2026-10-25-state-01/README.md)。实际方向、显示帧率、独立计时及堆栈仍待完整UI接入后实测。
