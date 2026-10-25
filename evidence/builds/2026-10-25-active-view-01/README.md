# 0.4.2活动视图构建

日期2026-10-25；板型sf32lb52-lchspi-ulp，SiFli SDK v2.5.1/HEAD421126d9f476ed8e2a6f0b0ca28a9f241c182e65。激活已有SDK导出脚本，运行python scripts/build_firmware.py，原样ASCII快照编译。

脚本/scons退出0、source_unchanged=true、三项固件存在；结束后独立重算当前所有构建源码与产物SHA-256一致。main.bin 568884 B，SHA-256 59c8a0e05414d8dc90f0002d939473bfe6bb8970b420343eb33f6f5fdf128f60。完整源码/固件/快照信息见attempt-01-state.json，日志attempt-01.log，ELF大小elf-size.txt。应用无编译诊断，既有SDK LVGL/RWX/ftab/syscall/entry/dfu警告保留，不伪称已修复。

[0.4.1实测与0.4.2回归](../../ui/2026-10-25-active-transition-01/README.md)分开记录；本版本未烧录，实际时序和RGB未确认，不冒充修复通过。未提交推送。
