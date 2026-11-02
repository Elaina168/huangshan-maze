# 滚动地图双变体完整构建

2026-11-01；SiFli SDK v2.5.1，sf32lb52-lchspi-ulp，既有激活脚本，无新增安装/SDK修改/刷机。

- normal-build.log/normal-state.json：默认正式构建，完整退出0，source_unchanged=true，44项源码/三产物独立SHA一致。main.bin609604B，比前版609108B增加496B，主要为新增字形及公共像素舍入；正式容量仍7×6，ELF work=9204B，含原存档入口，无试验map/runner符号。
- trial-build.log/trial-state.json：显式scrolling-trial，MAZE_SCROLL_TRIAL宏，完整退出0，44项源码/三产物独立SHA一致。main.bin596260B，ELF含试验map/runner，work=36256B，maze_store_open/maze_store_save不存在。该版本未引入正式目录/排行榜/存档写入。
- normal-target-checks.txt/trial-target-checks.txt：实际ARM ELF符号、源码/固件哈希、下载范围核对。两种构建均使用同一项目分区，默认下载仅bootloader@0x12010000、main@0x12020000、ftab@0x12000000，不含档案镜像。

完整SDK日志保留既有not-a-git-repository、RWX及syscall等警告，未宣称本轮修复。静态工作区不是板端运行资源峰值。试验烧录/时序/流畅度与手感尚待用户操作；见[使用指南](../../../docs/guides/scrolling-map-trial-use.md)。
