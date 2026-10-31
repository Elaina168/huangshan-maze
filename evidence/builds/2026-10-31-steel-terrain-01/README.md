# 0.5.0钢珠/地面对比原型构建

2026-10-31，基于HEAD91760ec的未提交增量；SiFli SDK v2.5.1，板型sf32lb52-lchspi-ulp。沿用已有SDK环境，执行`python scripts/build_firmware.py`一次完整构建。退出0、source_unchanged=true，3项产物存在；结束后独立重算当前全部构建源码及三产物SHA-256，与记录逐项匹配。

- main.bin：569724 B，SHA-256 `d6e07189afea3c61e422608281cdccc079d7e6370e6d92ae8b0585d063bbf5fb`。
- ASCII产物目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-shlktvsv\project\build_sf32lb52-lchspi-ulp_hcpu`。
- 全部源码、SDK路径、三项固件及哈希见`attempt-01-state.json`；完整输出`attempt-01.log`。

应用无编译诊断，SDK既有LVGL潜在空指针、RWX、syscall stub及ftab entry等警告保留，不声称已修复。[主机及真实LVGL离屏验证](../../model/2026-10-31-steel-terrain-01/README.md)通过，不等于板端已烧录或手感/帧率达标。板端按[新指导](../../../docs/guides/steel-terrain-use.md)短测，B/C不恢复。未暂存、提交或推送。
