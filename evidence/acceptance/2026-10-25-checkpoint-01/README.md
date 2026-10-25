# 2026-10-25收工检查点验证

用户明确授权今天工作提交推送；目标为现有`codex/maze-ui`分支，不合并main、不打交付tag。0.4.3已构建但屏幕提示未上板确认，B/C用户已停止；未来钢珠/冰面方向仅入文档，不是本提交实现。

本轮最终验证：

- `wsl --exec sh -c 'python3 /mnt/d/codex/黄山派/scripts/test_maze.py && python3 /mnt/d/codex/黄山派/scripts/test_imu_probe.py && /mnt/d/codex/黄山派/build/lvgl-host/ui-refresh'`退出0，原输出`host-regression.log`。后台13、校准8、模型7、状态9、旧探针7+14及所有权守卫、真实LVGL刷新和失败提示均通过。
- 现有Python执行`scripts/test_build_snapshot.py`，4 tests OK；`scripts/check_project.py --history`通过。材料检查不等同产品验收。
- 未改构建源码后再独立重算当前全部源码及3项固件SHA-256，与[0.4.3完整构建](../../builds/2026-10-25-paused-feedback-01/README.md)一致，无需重编译仅文档的变化。
- 使用真实SDK头/事件代码的UI事件交接测试此前3组通过；本轮沿用该层证据，不称本轮重跑。显示刷新统计为软件渲染，不是目标物理帧率或耗时。
- 按requesting-code-review技能做一次新上下文整批只读审查，范围为HEAD52346af以来model/app/UI/IMU/main，未报告新增Critical/Important。已知Minor为main的RGB引脚功能枚举直接比较导致误导日志，已有问题记录，不冒称已修复。
- 待提交文件高置信敏感信息模式扫描无命中；忽略SDK、固件、缓存和本机配置。`docs/course-requirements.md`既有回退改动保留本地，不纳入本提交。暂存差异检查及钩子在实际提交时执行；最终提交/推送状态以Git记录和远端分支为准。

测试输出中的WSL环境提示存在编码显示异常及末尾空字符，原样保留；没有因此伪造测试输出。学生负责今天真机操作/反馈和产品选择，Codex负责设计、实现、分析、文档、测试和只读复核协作；本次无人工源码修正。

暂存检查最初因原始日志的行尾空白/末尾空行失败。为保持证据原字节，增加仅作用于evidence原始log和serial-log.txt的Git属性：不转换换行，并将原输出空白作为证据保留；代码/文档仍执行正常空白检查。提交前再次运行`git diff --cached --check`并比较索引原始串口附件哈希，不篡改日志或跳过提交钩子。
