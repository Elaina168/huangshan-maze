# 正式三关界面与主入口软件验证

日期2026-11-02，工作区未提交。需求REQ-037/038/039，用例TC-053/054/057。使用现有SDK v2.5.1真实LVGL静态库，全部应用源重新编译，无旧ABI对象复用。

新增third_ui_harness在旧接口阶段编译失败：maze_ui_set_service、maze_ui_loading未声明（另有测试自身遗漏math.h，已补齐）。正式实现后完整测试通过。

- refresh.log：原几何/字体/全部状态/局部刷新；小图一步6510、校准或继续32850像素。
- map-error.log：四类地图载入失败均有原因/代码/返回按钮。
- third.log：三入口布局/加载反馈丢弃旧命令/真实大图裁剪/最终关边界；270次提交小球最小128=完整128，主机堆16448B前后不变，主机max42248B。
- scroll.log：共用试验路径500次提交小球128保持，HUD/单球/对象数/主机堆稳定，max122100像素。
- main.log：scripts/test_maze_main.py直接包含正式main.c，用外部I/O替身运行实际启动循环；旧档n+1在选关前写、失败同修订重试、retry_open后升级、三关成绩循环、加载顺序；实际显示服务回调WON再外层observe只结算一次。该替身不验证Flash物理操作；实际SDK存储门槛见research/2026-11-02-third-level-store-01。
- font-raw.txt：161字形通过test_ui_font.py与固定SHA源Noto字体逐像素核对（4bpp误差<=8）。PPM为真实LVGL输出，不是概念图。

主机像素/内存数据不是板端帧率、物理扫描、目标堆栈或实际Flash掉电通过。板端TC-058待执行，不恢复B/C。
