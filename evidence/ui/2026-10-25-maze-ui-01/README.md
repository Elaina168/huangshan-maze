# Task 6自动验证与待实测

2026-10-25，0.4.0-maze-ui，基线52346af加本地未提交Task 4–6。REQ/TC按设计附录C与使用指导关联；本文不是硬件验收报告。

## 自动结果

host-maze.log：后台11、校准8、模型7、状态7组，退出0；host-probe.log：原探针7+14和产品占用守卫，退出0。Windows Python运行test_build_snapshot.py为4/4通过。SDK完整构建与当前源/产物哈希核验见[最终构建](../../builds/2026-10-25-maze-ui-01/README.md)。

host-ui-events.log：UI队列3组，退出0。使用实际SDK v9.4.0头文件及lv_event.c，LV_CONF_SKIP/LV_KCONFIG_IGNORE下编译真实maze_ui.c，链接时丢弃未调用的绘图函数；没有复制队列算法或制作假渲染器。验证回调不改变app、首事件优先/只消费一次、旧状态/旧代次丢弃、新页面事件仍可用。它不验证按钮实际触摸、页面绘图、内存释放或帧率。准确编译运行命令见ui-events-command.txt，测试源码ui_event_harness.c。临时可执行跨调用消失导致的首次归档失败另见capture-attempt-01.md；改为同次调用编译与运行后成功，不隐藏失败。

## 静态核对与审查

SDK实际声明核对lv_obj_clean、lv_obj_remove_style_all、位置/尺寸/填充及文本对齐。物理坐标使用地图局部像素，容器去掉主题内距/边框，额外地图边框绘在外部；几何圆无阴影扩张。实际内置Montserrat 16 glyph与kerning表按SDK算法核算Calibrate为72px、Resume为68px，80px按钮各4px内距可容纳；这是静态字形核算，不替代真实文字可见性。

[独立只读审查](review.md)未发现Critical/Important项；未重跑测试或实测。执行者核对最终源码与成功构建哈希一致；源码/测试摘要见source-hashes.json。校准失败提示在第二轮改为不假定故障原因的“Calibration did not complete”，实际错误号保留。原始RGB及预研证据保留，不覆盖历史。

## 下一步

用户按[指导](../../../docs/guides/maze-use.md)烧录，先确认TILT MAZE/Start、校准、四向与Pause，再做胜败/重玩/退出与同准备态free/ps。2026-10-25本次查询仅蓝牙COM3/COM4，未识别到板卡USB串口，必须插好后确认实际端口，不照抄历史COM6。未执行项继续为未执行，Task 6尚未全部完成。

Codex实现、测试、审查与整理；用户本轮尚无人工代码修正或新增硬件证据。没有新增外部图片/字体/音频或线程，不提交推送。
