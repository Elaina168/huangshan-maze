# 后台IMU采样构建与回归记录

日期2026-10-17，应用0.2.1-imu-background-probe；SDK v2.5.1，板型sf32lb52-lchspi-ulp。基础提交仍为b508f19c72138715c746f72e8794deda31091b26，当前为未提交本地源码，最终绑定构建状态中的源码SHA-256，不能把基础提交当作本固件的全部源码。

## 第一轮：构建成功，但主机回归发现缺陷

[构建日志](attempt-01.log)、[源码与产物状态](attempt-01-state.json)保留原文件字节。该次scons/脚本返回0、source_unchanged=true，但不向用户交付：新增queued_failure测试在多次启停唤醒积压后发现同一代次的读失败会被再次尝试，断言raw_reads==1失败。修复前源码保存在[文本快照](before-queued-failure-fix.c.txt)，仅作失败复现，不参与工程编译。

最小修复：线程记住已处理的操作代次，同一代次的积压唤醒不重复启动会话。新请求代次仍可以启动。修复后queued_failure及另外10个场景通过；SDK完整重构建结果单列，不以旧成功产物替代修复后的构建。

## 自动测试的范围

最终主机输出：[11场景](host-maze.log)、[探针回归与所有权限制](host-probe.log)，均退出0。构建快照测试4/4及工程材料/历史检查通过。WSL启动有此前已有的localhost代理提示，未安装或调整工具。

- tests/maze/imu_harness.c执行真实maze_imu.c，桩替代I2C与RT调度，覆盖正常、读取迟到、读期间停止、状态读取后停止、读取失败、配置失败、恢复失败、保存失败、缺失总线、就绪超时、积压唤醒失败。
- 快照代次和符号解码、副本隔离、寄存器恢复、锁外I2C/等待/日志、只有一个线程启动均有断言。
- 原imu_probe测试追加两个命令在产品所有权下返回忙且零I2C访问，原7+14场景继续通过。最初测试代码遗漏stdbool头和重复局部变量先修正，随后实际访问限制断言RED，再加生产守卫转GREEN；不把测试自身编译错误当作功能RED证据。
- 自查发现的stop_during_status与restore_failed新增断言分别RED，随后修复“取消后追加raw读取”和“旧恢复错误标为新代次”，全套GREEN。锁桩和单线程受控调度不能替代真实RT并发、优先级、驱动阻塞或长期可靠性验收。

## 修复后最终构建

[第二次完整日志](attempt-02.log)、[源码/产物清单](attempt-02-state.json)、[ELF静态段统计](elf-size.txt)。脚本与scons退出0，source_unchanged=true；结束后再次独立计算当前全部源码与三项产物SHA-256，均匹配状态清单。快照位于 `C:/Users/zytx/AppData/Local/Temp/huangshan-maze-cz69yc84`，有效下载目录为其下 `project/build_sf32lb52-lchspi-ulp_hcpu`。

| 固件 | 字节 | SHA-256 |
|---|---:|---|
| main.bin | 561848 | e1cbcbac0bdd10326f4051acf7f2ac773ecbfd42cb6bff40fe74e5a88870bf93 |
| bootloader/bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab/ftab.bin | 11288 | 916d91816642e6421ab098e7a90ed2fad6f5f033f3e91a9e504b58525662cf70 |

ELF text557636/data4208/bss93484 B，静态段统计不等于运行堆栈余量。本次仍有SDK lv_obj_swap潜在空指针、RWX、ftab syscall/entry及缺dfu提示，原日志保留；本项目main/maze_ui/maze_imu/imu_probe未出现编译诊断，不声称SDK警告已修复。

## 尚待板端验证

当前用户板端运行的是上一个0.2.0显示原型，其17次点击及资源记录不能证明本版后台采样通过。新固件尚未烧录/实测，操作见[后台预研指导](../../../docs/guides/imu-background-probe.md)。正式产品TC仍未执行，不提交GitHub。
