# 中文界面实施计划

> **For agentic workers:** Use superpowers:executing-plans in the current session. No commit/push until the user requests it.

**Goal:** 用已授权的静态字形完整显示简体中文界面。

**Architecture:** 保留唯一UI线程及持久场景，只替换显示文字并显式设置项目字体。离线Pillow生成16px/4bpp中文子集，ASCII fallback为既有Montserrat 16，固件不读TTF。

**Spec:** [已确认短设计](../specs/2026-10-31-chinese-ui-design.md)。

## Global Constraints

保留地图、物理、校准/时序阈值和串口字段；不安装依赖、不改SDK、不烧录、不恢复B/C、不提交推送。当前codex/maze-ui专用目录继续，既存课程清单改动保持原样。

## Task 1：中文文案与字体

- [x] 先在真实LVGL测试中把标题预期改为冰岔迷宫，观察旧英文界面RED。
- [x] 将src/maze_ui.c全部显示文案翻译，16px字体名maze_font_zh_16，line_height=20、base_line=4；保持ASCII fallback。src/maze_font_zh_16.c及头声明由scripts/generate_ui_font.py从实际文案生成，使用明确输入字体路径、校验源SHA，不携带原TTF。
- [x] 生成程序输出字形清单/源哈希/Pillow版本，保留原OFL版权，衍生字体使用项目名称；记录THIRD_PARTY.md。
- [x] 真实LVGL逐一验证所有UI非ASCII字形由项目字体解析且非占位，ASCII fallback来自Montserrat；枚举全部状态/按钮，检查标签内容、布局边界、动态错误码和计时、原暂停提示缓存、40000/10000刷新阈值。
- [x] 增加字形再现检查：同输入生成字节相同；文案新增字形必须使现有覆盖测试失败。

## Task 2：可烧录增量与记录

- [x] REQ-025关联TC-031字体/授权/构建、TC-032真实LVGL文案布局、TC-033板端中文短测；版本0.6.1-chinese-ui。
- [x] 运行相关主机回归、快照4测试、check_project及独立只读代码复核；记录原始RED/GREEN。
- [x] 冻结产品源码后完整SDK构建，退出0、source_unchanged=true、三产物存在，独立重算全部源码/产物哈希，归档evidence。
- [x] 更新开发日志、测试报告和中文短测指导；板端未执行保留，不把主机截图当实物证据。

## Review Focus

ASCII/中文混排基线、奇数像素宽的4bpp打包、全部状态/动态错误码的漏字、主题按钮字体覆盖、暂停失败缓存不得被健康采样抹掉。均纳入真实LVGL测试；实际可读性/刷新耗时仍由短测确认。
