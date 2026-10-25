# 0.4.1刷新与采样间隔修复构建

2026-10-25，板型sf32lb52-lchspi-ulp，SDK v2.5.1/HEAD421126d9f476ed8e2a6f0b0ca28a9f241c182e65。执行现有SDK激活脚本后运行python scripts/build_firmware.py，均由原样源码创建ASCII快照，不修改SDK。

attempt-01：持久场景及诊断命令，尚未修复消费端误判；attempt-02：真实间隔元数据，尚未保留被健康样本覆盖的异常；attempt-03：最终含超限累计及回归。三轮成功状态/日志原样保留，中间版本不要求用户烧录。最终scons/脚本退出0、source_unchanged=true、三项固件存在；独立核对当前源码与产物SHA-256匹配。

最终main.bin 568740 B，SHA-256 d3c040c2c0b98830e5cf2e53b8afdc7bcbcbf848ac3b922eb6acacf5a9693ecc，清单与快照位置见attempt-03-state.json。ELF大小elf-size.txt，map确认maze_rgb_off命令已链接。应用无编译诊断，SDK既有LVGL潜在空指针、RWX及ftab/syscall/entry/dfu提示保留，不宣称SDK无警告。

[自动验证与RED/GREEN记录](../../ui/2026-10-25-flush-fix-01/README.md)另存；[原板端失败](../../ui/2026-10-25-first-board-failure-01/README.md)不覆盖。最终版本未烧录，目标时序与物理RGB未确认，不能把本构建写成真机修复通过。未提交推送。
