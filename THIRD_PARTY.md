# 第三方资源与复用清单

| 资源 | 来源与版本 | 许可/使用边界 | 本项目使用与修改 |
|---|---|---|---|
| course_starter 工程骨架与 main.c | 用户提供的课程资料，SDK v2.5.1 基线 | main.c 明示基于 SiFli SDK Apache-2.0 例程；保留原注释和附带许可。课程包没有独立可核验的版本标签，不编造 | 初始原样导入 project/ 六个文件、src/ 两个文件；2026-10-10 用户修改 main.c 的版本、RGB 亮灭时长和对应注释。清单保留原始来源 SHA-256，并标记 main.c 已修改；当前源码由 Git 与构建快照追溯 |
| SiFli SDK | https://github.com/OpenSiFli/SiFli-SDK ，v2.5.1 | SDK 主体及内部第三方组件分别遵守其许可；仓库不打包 SDK | 外部构建依赖，本机实际路径由激活环境提供 |
| RT-Thread、LVGL | 当前SDK内RT-Thread版本宏3.1.3；external/lvgl_v9/lv_version.h为9.4.0 | RT-Thread根LICENSE为Apache-2.0；LVGL的LICENCE.txt为MIT，内部组件/字体仍遵守各自许可 | 不单独复制；2026-10-17首次启用LVGL显示/触摸预研页，尚非迷宫。LVGL Kconfig开头v9.2.0注释与版本宏不一致，以实际版本宏记录 |
| 两份课程 PDF | 用户提供的课程指导手册、课程简介 | 未提供开放许可；仅在私有课程项目中归档作为依据，不赋予 Apache 许可 | 原字节归档，无修改 |
| peripheral_lab 示例日志 | 用户提供的两份文本附件 | 用户提交的操作证据；不是本项目构建/运行结果 | 原字节归档，记录哈希；口头硬件反馈另行标注 |

原样导入清单：[import-manifest.json](docs/references/import-manifest.json)。素材、字体、声音与后续参考代码在采用前追加来源、版本、许可和修改记录。

IMU预研补充：src/imu_probe.c由Codex编写，参考用户提供的peripheral_lab中lab_bus.c/test_imu.c的I2C3引脚、器件探测和寄存器访问方法，未复制整套验证台。寄存器定义另核对SDK v2.5.1的LSM6DSL头文件。SDK依赖遵守其原许可；课程参考包不被本项目擅自赋予统一许可。project/proj.conf已增加I2C3配置，清单标记已修改并保留原始来源哈希。main.c的本人RGB修改本轮不改动。

AI 协作记录保存在开发日志。需求、检查脚本和说明由 Codex 协助编写，用户选择玩法并审阅范围；不将复制代码计为个人新增功能。

后台采样补充（2026-10-17）：maze_imu.c/.h由Codex依据已核对的RT-Thread IPC/线程API和现有IMU探针寄存器行为编写；新增UI启停事件、代次快照、原诊断命令访问限制和受控主机测试。测试复用tests/imu_probe的最小I2C/HAL声明，RT调度桩只用于验证生产线程入口，不引入新的外部依赖。用户批准方案并完成板端操作，未将这些新代码记录为本人独立手写成果。

校准输入补充（2026-10-17）：maze_app.c/.h、tests/maze/harness.c由Codex按已批准候选参数实现，复用既有预研规则而非导入外部算法库；USB右轴关系依据用户确认的180°朝向变换。UI与主循环接入校准、取消/重试及限频反馈，没有新增外部依赖、素材或字体。

迷宫集成补充（2026-10-25）：maze_level/model与app状态、计时、碰撞测试由Codex依据已批准设计实现；maze_ui使用SDK现有LVGL几何对象和内置Montserrat 16绘制地图、小球及按钮，不导入游戏模板、图片、音效或新字体。UI事件只在主线程消费，用户本轮未逐行修改代码；构建、主机测试和后续真机操作分别记入开发日志，不将AI实现伪称本人独立手写。

显示预研补充（2026-10-17）：src/maze_ui.c/.h由Codex编写，沿用SDK内LVGL v9 API，参考peripheral_lab/src/main.c的主线程初始化与2–20ms等待方式，没有复制外设验证台。main.c保留原SDK例程来源说明，原RGB循环替换为关灯与显示服务；project/proj.conf启用LVGL、局部缓冲和首次试验栈配置。仅使用SDK内置字体，无外部图片、音频或字体下载。SDK的external/lvgl_v9/src/font/lv_font_montserrat_16.c文件头列出Montserrat与FontAwesome5来源；许可分别见external/lvgl_v9/scripts/built_in_font/font_license/Montserrat/OFL.txt（SIL OFL 1.1）、FontAwesome5/LICENSE.txt（Font Awesome Free分项许可），正式交付保留依赖声明与相关许可。

## 中文界面字形（2026-10-31）

原字体来自SiFli SDK v2.5.1的`external/lvgl_v9/tests/src/test_files/fonts/noto/NotoSansSC-Regular.ttf`，Copyright 2014-2021 Adobe、SIL OFL 1.1；原始SHA256 `ae82f4e2a55e1316a55bcc1d05e9555ce08d8bda07e893b486896b626fd852ff`。[原版权与完整许可](licenses/NotoSansSC-OFL.txt)随衍生字形保留。项目不重新分发原10MB TTF，构建固件不依赖运行时字体引擎。

Codex使用现有Pillow 12.3.0（仅离线工具）按实际UI字符串栅格化16px/4bpp字形，衍生字体名`maze_font_zh_16`，不使用原字体保留名称命名。脚本为`scripts/generate_ui_font.py`，源哈希、字符表和体积见[清单](docs/design/chinese-ui-font.json)。ASCII复用SDK Montserrat 16 fallback，授权沿用上方声明。字体的字形设计是第三方成果；本项目仅实现离线转换与接入，不算学生独立设计字体。
