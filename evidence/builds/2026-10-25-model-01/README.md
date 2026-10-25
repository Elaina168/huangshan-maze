# Task 4完整SDK构建

日期：2026-10-25。基线提交52346af加本轮尚未提交的Task 4改动。SDK HEAD 421126d9f476ed8e2a6f0b0ca28a9f241c182e65，SiFli v2.5.1，板型sf32lb52-lchspi-ulp，工具版本按原始日志记录。

使用已存在且路径核验的SDK导出脚本激活环境，执行python scripts/build_firmware.py；ASCII快照目录、完整source清单、三项产物字节及SHA-256见attempt-01-state.json。脚本/scons退出0，source_unchanged=true，结束后独立重算当前全部源码与三项固件哈希匹配；原始日志见attempt-01.log。确认maze_model.o/maze_level.o均实际交叉编译，应用新增源码无编译诊断。

当前main仍使用0.2.2-imu-input-probe预研入口；模型还没有被app/UI调用，不用二进制大小或该构建冒称迷宫已运行。未烧录、无新增板端证据。SDK原有LVGL潜在空指针、RWX、ftab syscall/入口及dfu提示保留，不改SDK或屏蔽警告。

纯模型及原有回归结果与RED记录见[模型证据](../../model/2026-10-25-model-01/README.md)。真实方向、完整状态机、显示和性能验收仍在后续任务。
