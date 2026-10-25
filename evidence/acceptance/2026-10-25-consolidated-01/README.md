# 集中验收准备与主机回归

2026-10-25，用户要求“一次把要测的全测完”。板端按[集中清单](../../../docs/guides/maze-final-test-batch.md)一批执行，当前尚未收到该批结果，不填写正式全部通过。

本轮主机运行：`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`，退出0，后台13、校准8、模型7、状态9全部PASS，原输出见`host-maze.log`。包含真实源码在受控I/O/调度下的读取失败、历史gap、校准稳定窗、状态恢复、碰撞及20/40Hz app更新对照；不冒充真实传感器故障、显示刷新对照或板端性能。PowerShell保存的WSL提示有编码显示异常及末尾空字符，原日志保留，PASS和退出结果可辨。

同轮现有Python执行`scripts/test_build_snapshot.py`退出0，4 tests OK。没有源码/配置变化，不重做SDK构建，也没有新固件或烧录。本轮主机结果不覆盖用户尚未操作的50轮/30分钟与视频性能。
