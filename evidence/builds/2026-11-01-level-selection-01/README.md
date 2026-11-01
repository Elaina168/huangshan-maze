# 0.8.0两关与方向设置完整构建

目标sf32lb52-lchspi-ulp，SiFli SDK v2.5.1。`attempt-01.log`完整构建退出0，`attempt-01-state.json`为source_unchanged=true；当前30份源码及3产物SHA独立重算全部一致。未烧录或板端验收。没有修改SDK，原SDK警告保留。

main.bin592148字节，SHA-256为`e2abea6c663b20cafb1a5ee9c10ce3bc39ee57f53b4efd6d083d07d93fb56c8b`。目录：`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-2cwkj3h2\project\build_sf32lb52-lchspi-ulp_hcpu`。

`target-symbols.txt`来自本SDK ARM nm：nav.1为2928字节静态BSS，work为9204字节静态BSS；两关目录和第二关定义存在，`maze_level_one`/`maze_level_surface_lab`均不在目标ELF。静态符号大小不是实际堆/栈运行峰值。
