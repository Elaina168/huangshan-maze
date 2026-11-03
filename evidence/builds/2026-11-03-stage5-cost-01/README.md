# 第五关动态碰撞开销修复构建

2026-11-03，版本0.12.1-dynamic-cost。完整normal SDK退出0/source_unchanged，当前全部源码与快照、三产物SHA独立一致。相对原0.12.0快照仅src/main.c与src/maze_model.c变化：保守包围盒排除远墙、物理耗时诊断及128B有界日志；配置、线程优先级、1ms子片、100ms保护、存档与关卡数据未变。

下载目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-0wav_i07\project\build_sf32lb52-lchspi-ulp_hcpu

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 631836 | e04ac7609ba3f156c0a28b9aa9bdd490e43ef504c745b92ade684a8d3a6552bd |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | 20276f928fb370bba130f2e9143db2528431bb97fc4c8c7a0e4d7072943da932 |

原第五关软件用例、旧关与惯性路线全回归通过，实际main业务和128B最宽日志预算通过；新真实图工作量用例4250次先RED，修复后50次GREEN。不是板端耗时或修复成功实测。源配置/目录未改，不重复无关存档755切断/字体/离屏测试；原源文件哈希保持可核。

原始SDK警告保留于build.log，未改SDK、未安装、未烧录、未提交推送。板端只复测原第五关校准/继续后-1002路径；详细失败与软件修复见../../ui/2026-11-03-stage5-gap-board-01/README.md。main实际序言附main-prolog.txt，尚无本版free/ps，不能声明全路径栈或实际堆已通过。
