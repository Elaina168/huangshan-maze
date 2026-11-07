# 最终确认后软件回归（2026-11-07）

功能版本0.16.1-ui-trim；最终功能源码提交4775b1b99acab5a06d1fc734780d621495a22e7a。此次用户已明确选择最终功能版本，原话见[确认](../../ui/2026-11-07-final-confirm/README.md)。

本目录logic.log为最终确认后重新执行scripts/test_maze.py；main.log/watchdog.log分别为实际main和WDT适配器的受控I/O回归。ui-stage7/ui-refresh/ui-async/ui-scroll四日志为对当前已构建真实SDK LVGL目标重新运行，未修改产品代码。font-raw.txt及PPM是主机构造帧/字形，不是实物成绩或照片。

全部所列脚本/目标退出0。最终源码仍与[成功构建](../../builds/2026-11-07-ui-trim-01/README.md)的project/src集合一致，三产物SHA匹配；Git按.gitattributes对7个文件规范换行，实际构建快照与Git内容仅这些换行不同，交付包恢复快照字节并校验Git等价性。最终终稿提交只改变材料/证据，不改变该代码。

存储仍沿用已核对v7真实SDK库1405次进程中断证据，不伪称此次重新做物理掉电。用户反馈、主机/离屏、构建、未提供的硬件量化分开写在[最终报告](../../../docs/final/test-report.md)。包生成、Git恢复、ZIP解压和固件核验结果在包外核验报告中保存，不声称已代为教学平台提交。
