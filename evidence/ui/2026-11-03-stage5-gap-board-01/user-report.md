# 用户实际失败反馈

2026-11-03。用户在stage5-use.md交接后反馈：“玩不了，设备异常-1002”。补充原文：

> 校准完之后报错，只有第五关这样

用户提供日志摘录（保留原来的截断，未补造缺失字段）：

```text
[maze_nav] command=4 index=0 page=1 version=14 tick=163688
[maze] event=5 state=4 gen=16 tick=163688
[maze_nav] page=1 version=14 index=4 reversed=0 error=0
[maze] gen=17 state=1 error=0 count=0
[maze] gen=17 state=1 tick=163688 ms=177 pos=25,25 input=0,0 error=0 vel_centi=0,0 surface=0 physics_gap_tick=0 flush_services[maze] gen=17 state=1 tick=164205 ms=177 pos=25,25 input=0,0 error=0 vel_centi=0,0 surface=0 physics_gap_tick=0 flush_services[maze_nav] page=1 version=14 index=4 reversed=0 error=0
[maze] gen=17 state=5 error=0 count=20
[maze] calibrated ref=-240,-292,-16181
[maze_nav] command=4 index=0 page=1 version=14 tick=169449
[maze] event=2 state=5 gen=17 tick=169449
[maze_nav] page=1 version=14 index=4 reversed=0 error=0
[maze] gen=17 state=2 error=0 count=20
[maze] gen=17 state=2 tick=169449 ms=177 pos=25,25 input=0,0 error=0 vel_centi=0,0 surface=0 physics_gap_tick=0 flush_services[maze_nav] page=1 version=14 index=4 reversed=0 error=0
[maze_timing] loop_tick=200 service_tick=191 render_tick=6 previous_seq=437 previous_read=169527
[maze] gen=17 state=4 error=-1002 count=20
[maze] gen=17 state=4 tick=169649 ms=275 pos=25,25 input=0,0 error=-1002 vel_centi=0,0 surface=0 physics_gap_tick=66 flush_ser
```

省略了用户原文中多条相同暂停状态的周期行，未改变本摘录数值。版本关联来自交接上下文，无设备产物哈希。该次TC-070不是通过；校准成功，继续后出现生产者采样间隔保护，服务耗时191ms。不能据此宣布传感器损坏或唯一耗时来源已实测定位。
