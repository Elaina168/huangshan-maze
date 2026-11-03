# 第5阶段软件验证证据

2026-11-03。对应本地未提交的0.12.0-dynamic-mechanisms；版本绑定以最终构建源码快照与产物SHA为准，尚未烧录、没有板端通过结论。

- core-red.log：动态接口不存在时新测试编译失败；magnet-green.log：最初磁力用例通过。
- dynamic-red.log：激光静止切线漏判；boundary-red.log：整步末端恰好开启的激光被延迟。
- review-red.log：独立复核的步内角切线反例失败；priority-red.log：同刻激光开启先激活检查点反例失败。
- review-green.log及final-dynamic.log：上述修复、真实巡逻推动危险区、挤压、生命周期、20/40Hz一致性、真实第五图57.19秒控制轨迹通关、外部数据拒绝和v3夹具保留通过。控制轨迹时间不是用户通关成绩。
- final-model.log：原逻辑/模型/状态/地图/进度/惯性路线回归通过；final-main.log：正式main直接编译的外部I/O替身回归通过。
- archive-red.log/archive-green.log：五关格式引入前后独立真实旧v3夹具测试。
- store.log：实际SiFli SDK FlashDB结合NOR模型，总755次受控进程中断（v1/v2/v3迁移各180，加普通/GC/初建215）。旧新完整恢复、源bank保持、未知格式保护、同修订冲突、最大revision等通过；不是物理掉电测试。SDK库未改。
- ../ui/2026-11-03-stage5-01：真实LVGL同步/异步、五关菜单及激光/挤压反馈。最终日志以软件整合后的本轮文件为准，主机堆不是板端堆。

独立复核发现两项P2激光边界问题，已分别先RED后GREEN修复；无其他已确认重要问题。保留初始失败，不将设计/离屏/构建写成真实板端验证。原B/C不恢复。


## 板端失败后的0.12.1开销修复

0.12.0第五关实际-1002失败见[用户日志](../ui/2026-11-03-stage5-gap-board-01/README.md)。新增cost-red.log保留4250次精确求交导致预算断言失败；cost-model-green.log包含相同10ms起点工作量降到50及全模型/旧关/动态回归通过。cost-main-green.log包含正式入口与128B诊断完整性通过，log-budget-red.log保留原错误时序行最宽131B的失败。指标是函数调用次数而非目标CPU时间；新版板端待复测。存储/UI/字体未改，不重复无关测试。
