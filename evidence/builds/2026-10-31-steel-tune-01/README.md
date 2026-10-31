# 0.5.1速度与滑行调参构建

2026-10-31，SiFli SDK v2.5.1、sf32lb52-lchspi-ulp；在用户0.5.0体验反馈后仅调整速度上限/普通冰面阻尼与版本号。完整运行build_firmware.py一次，退出0、source_unchanged=true；当前全部构建源码及三产物SHA-256独立重算一致。

main.bin569724 B，SHA-256 `8f5ca614259e7334309ebc67b3092175919c8b42311fd5a080aa75197cd2032b`。产物目录`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-l398zyut\project\build_sf32lb52-lchspi-ulp_hcpu`。源码/三产物清单见attempt-01-state.json，完整输出attempt-01.log，既有SDK警告保留。

[新增调参断言及原相关回归](../../model/2026-10-31-steel-tune-01/README.md)通过，原始RED/GREEN保留；不代表新参数已上板验证。候选参数180 px/s、普通/冰面3.0/0.6 s⁻¹，加速度600不变。[操作指导](../../../docs/guides/steel-terrain-use.md)只要求短路径比较，不恢复B/C。本轮不提交推送。
