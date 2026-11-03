# 第6阶段真实LVGL离屏验证

现有SiFli SDK2.5.1 LVGL9源码，390×450/RGB565，七目标：ui-refresh、ui-third、ui-stage4、ui-stage5、ui-async、ui-scroll、ui-random。分别见同名log，编译日志在../../stage6-2026-11-03/ui-build.log。

固定五关、九按钮菜单、生成取消、随机准备/暂停/通关/成绩及保存失败提示通过。随机异步加入真实生成图：累计1335提交，源覆写0、球最少128像素、黑洞0；滚动试验修复后堆空闲38528B前后不变。是主机LVGL堆及延迟传输模型，不是目标LCD、DMA或主机外堆的测量。

`random-*.ppm/png`是实际离屏帧；准备/暂停/成绩截图特意注入-3102，展示未保存状态，并非正常设备自发报错。`font-raw.txt`为真实LVGL解码字形，字体逐像素结果另记录。

实际实屏闪烁、黑块、free/ps与生成时序仍待TC-075。
