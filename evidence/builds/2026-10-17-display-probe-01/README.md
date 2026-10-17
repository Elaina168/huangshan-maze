# 显示/触摸原型构建记录

日期：2026-10-17。板型sf32lb52-lchspi-ulp；SDK v2.5.1，HEAD为421126d9f476ed8e2a6f0b0ca28a9f241c182e65。应用版本0.2.0-display-probe。基础提交b508f19c72138715c746f72e8794deda31091b26之上的本地未提交改动，不把基础提交当成此固件的完整源码版本。

## 首次失败

- 命令：激活既有SDK环境后运行 `scripts/build_firmware.py`，内部scons板型sf32lb52-lchspi-ulp、并行8。
- [原始构建输出](attempt-01.log)与[源码/产物清单](attempt-01-state.json)保留脚本生成字节。
- 脚本退出1，scons退出2，source_unchanged=true，只有bootloader产物，不能烧录或声称完整构建成功。
- SDK middleware/lvgl/app_mem.c引用struct rt_memheap及rt_memheap_*，但RT_USING_MEMHEAP未开启；rtdef.h把定义放在该条件内。
- 最小修复：项目proj.conf增加CONFIG_RT_USING_MEMHEAP=y。该选项与系统堆算法独立，不启用RT_USING_MEMHEAP_AS_HEAP，不改SDK。

## 修复后完整构建

- [第二次原始输出](attempt-02.log)、[第二次源码与固件清单](attempt-02-state.json)、[生成配置与链接资源摘录](link-resource-summary.txt)。脚本及scons退出0，source_unchanged=true；再次独立核对当前源码清单和三项产物SHA-256全部一致。
- 快照：`C:/Users/zytx/AppData/Local/Temp/huangshan-maze-pu2b5o3i`；烧录工作目录为其下 `project/build_sf32lb52-lchspi-ulp_hcpu`，下载脚本实际存在。未执行下载。

| 产物 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 559856 | 7cb9a9e2b78c00cca2dadb9b228ece841691440bf082b8ae9cf751907e6dfe0d |
| bootloader/bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab/ftab.bin | 11288 | ee8d9cc46839107efae9d87eb8a43813abd9deb4089b9fad1c1408b2704be3b6 |

GNU size报告text555644、data4208、bss91184 B；静态可装载段算术和559852 B，data+bss算术和95392 B。这些不是运行堆/栈余量；main.bin含布局填充，大小不同不混用。nm与map确认两块绘图缓冲各31200 B（0x79e0），地址0x20010780与0x20008da0，位于l1_non_ret_bss_frambuf，总62400 B；不是从堆动态分配。生成配置同时有RT_USING_MEMHEAP与RT_USING_SMALL_MEM，无MEMHEAP_AS_HEAP，系统堆算法保持不变。

警告未清除：SDK LVGL的lv_obj_swap潜在空指针诊断；main/bootloader的RWX段、ftab的syscall stub与缺entry提示，另有dfu镜像缺失提示。日志完整保留，应用main.c/maze_ui.c未出现编译错误或警告。不更改无关SDK以压掉警告；构建成功不能替代板端启动验证。

## 验证边界

构建记录不证明显示方向、触摸位置、运行资源或迷宫玩法。板端按[操作指导](../../../docs/guides/display-probe.md)另测，正式产品TC仍未执行；没有本版烧录与运行证据。
