# 第三关冰环迷宫：设计验证

2026-11-02，固定布局草案，未进产品。地图数据见[layout.json](../../../docs/design/maze-layout-2026-11-02-ice-loops/layout.json)，包括字符串、对象、141段墙、冰面、普通/短路安全参考点及拓扑统计。

layout-check.c是临时设计夹具的原样验证输入，调用现有maze_map.c、maze_model.c、maze_level.c；未改这些产品源码。使用DMAZE_SCROLL_TRIAL只是借用已存在的14×12容量，不是生成新试验固件。

复现命令（已有WSL/GCC，无安装）：

```sh
gcc -std=c99 -Wall -Wextra -Werror -DMAZE_SCROLL_TRIAL -Isrc evidence/design/2026-11-02-ice-loops-01/layout-check.c src/maze_map.c src/maze_model.c src/maze_level.c -lm -o /tmp/ice-loops-layout-check
/tmp/ice-loops-layout-check
```

实际输出见model.log。两条路线从起点连续使用真实加速/阻尼/碰撞规则，不传送或途中清速度，逐步检查完整球不穿墙/不盖坑，最终WON。保守控制器在参考点减速停靠，模型时间不是玩家实测。五坑从安全侧位开始用真实运动滚入并验证LOST/清速/冻结，不把球直接放到坑心冒充运动触发。

JSON验证包含168格连通、2回环、21分岔/18死路、5个陷阱单元逐一移除后断开起终点；安全折线每至多1px采样，最小球体墙余量4px/坑余量3px。预览是Pillow设计图，不是屏幕实测。

未验证第三关UI资源/板端加载和绘图耗时/手感，未实现目录或旧档升级，不声称正式关卡已完成。


## UI内存设计预研

ui-layout-input.c把同一布局作为只供现有UI读取的临时定义，ui-memory-probe.c在主机真实LVGL依次创建准备/校准/游玩/暂停/胜利/设备错误场景，ui-memory.log为实际输出。报告max_used最高43712B，暂停free13776B。主机池统计不等于板端RT-Thread堆；这只支持先使用已有持久场景+LVGL裁剪的设计选择，正式v2存档/第三关菜单/字体加入后仍需重新验证。

编译使用本仓库tests/maze/lvgl-host的现有liblvgl_host.a、SDK existing external/lvgl_v9头，宏LV_CONF_SKIP、LV_KCONFIG_IGNORE、LV_FONT_MONTSERRAT_16=1、LV_FONT_DEFAULT=&lv_font_montserrat_16及MAZE_SCROLL_TRIAL；应用源码为maze_ui/app/model/level/font_zh_16/map/map_frost_fork/map_second/catalog/navigation/archive/progress/camera。只运行预研夹具，不改产品源码，不生成固件。
