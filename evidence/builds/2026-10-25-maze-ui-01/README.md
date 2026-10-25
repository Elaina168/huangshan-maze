# Task 6迷宫UI构建

日期：2026-10-25，版本0.4.0-maze-ui；基线提交52346af加本地未提交Task 4–6改动。板型sf32lb52-lchspi-ulp，SDK v2.5.1/HEAD 421126d9f476ed8e2a6f0b0ca28a9f241c182e65。尚未烧录或真机验收。

两轮均为激活SDK后执行python scripts/build_firmware.py，scons/脚本退出0，source_unchanged=true。首轮完成后发现校准失败提示“Could not find a steady pose”不能涵盖I/O失败，改为“Calibration did not complete”，保留真实error；这是提示准确性修正，不修改状态规则。首轮日志/state原样保留，交付测试使用第二轮。

最终状态及三固件大小/哈希见attempt-02-state.json，完整日志attempt-02.log，ELF容量elf-size.txt。结束后独立重算当前全部源码和三项产物SHA-256一致。main.bin为568160 B，SHA-256 c6459a977f2e00ec603066d8a423eafe97b05ef89ad072517d6f1f4bb7fd9a2d。应用无编译诊断，SDK原有LVGL潜在空指针、RWX、ftab syscall/entry和dfu提示保留，不宣称SDK警告已修复。

页面主线程单一所有者，地图几何与模型共用只读关卡；只有状态变化才清理/重建页面，游玩更新球/用时。UI事件测试不覆盖渲染，真实可见方向、按钮、页面切换、错误恢复及堆栈仍需用户上板操作；[使用指导](../../../docs/guides/maze-use.md)。[自动验证与审查](../../ui/2026-10-25-maze-ui-01/README.md)另存，不能替代板端通过。
