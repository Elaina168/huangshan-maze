# 构建、烧录与使用说明

作者李天宇（2024211450）；程序0.16.1-ui-trim。

## 环境与源码复现

实际基线：黄山派sf32lb52-lchspi-ulp、SiFli SDK v2.5.1、SDK内RT-Thread版本宏3.1.3、LVGL版本宏9.4.0、arm-none-eabi-gcc14.2.1；Python运行项目脚本，Git管理历史。SDK/RTOS/LVGL来源与许可见[第三方清单](../../THIRD_PARTY.md)，SDK官方仓库为https://github.com/OpenSiFli/SiFli-SDK 。SDK与Python环境按对应版本在开发机配置。

安装并按实际SDK的说明激活环境后，在工程根目录执行：

```powershell
python scripts/check_project.py
python scripts/build_firmware.py
```

项目统一通过ASCII快照脚本调用SCons。build_firmware.py复制project/src到新ASCII临时目录，逐文件核对并构建；完整结果在build/last_build.log和build/build-state.json。成功条件为returncode=0、source_unchanged=true且三固件齐全，下载目录从本次state读取。

普通固件构建直接使用已生成的C字体。需要改中文文字时才使用既有Pillow与SDK内实际TTF运行scripts/generate_ui_font.py，并以scripts/test_ui_font.py验证；生成C字体已随源码提供。

Git按.gitattributes规范部分文件换行；交付包工程的project/src恢复成已验证构建快照原字节，并同时检查Git归一化内容与封版提交一致，manifest记录对应快照SHA。重新检出后可能出现只含换行的字节差异，可通过.gitattributes归一化和manifest核对源码内容。

## 软件验证复现

纯逻辑脚本在已具备cc的环境执行python3 scripts/test_maze.py。main、WDT、FlashDB测试还需给出实际SDK目录：

```text
python3 scripts/test_maze_main.py --sdk <SDK路径>
python3 scripts/test_maze_watchdog.py --sdk <SDK路径>
python3 scripts/test_maze_store.py --sdk <SDK路径>
```

真实LVGL主机目标配置在tests/maze/lvgl-host，LVGL_DIR指向SDK/external/lvgl_v9；目标ui-stage7、ui-refresh、ui-async、ui-scroll。实物操作按板端演示流程执行。

## 固件下载

最终ZIP根目录的firmware包含bootloader/bootloader.bin、main.bin、ftab/ftab.bin、原uart_download.bat和manifest.json。烧录前关闭占用串口的监视器，确认实际板卡和串口。本项目使用过COM6，实际下载时按设备管理器确认端口。

在包内firmware目录，用实际sftool.exe路径和实际串口执行，例如实际串口为COM6时：

```powershell
& '<sftool.exe完整路径>' -p COM6 -c SF32LB52 -m nor write_flash 'bootloader\bootloader.bin@0x12010000' 'main.bin@0x12020000' 'ftab\ftab.bin@0x12000000'
```

尖括号必须替换。当前使用sftool0.2.5；原批处理依赖PATH，找不到sftool时使用实际完整路径。下载范围限定为下列三项固件，保留独立存档分区；串口监视器在下载期间保持关闭。三固件SHA/字节与源码提交、tag、配置在manifest和SHA256SUMS中核对。

## 操作

1. 上电查看中文选关页，正常显示方向USB在右侧。默认RGB菜单低亮呼吸，供电BAT指示灯由供电状态控制。
2. 选择已解锁固定关或随机挑战。选关页可设置黑暗/反转组合，进入关卡后模式冻结。
3. 点击开始，放稳板卡，20点稳定窗口约1秒后新局开始。轻倾加速、反向制动，冰面更滑、粗糙地面减速。
4. 暂停可继续、校准、重玩或选关。暂停重校准和故障恢复成功先回暂停，再点继续。
5. 按关卡使用钥匙/开关开门，撞破棕墙、触发检查点或避开动态机关。落坑/激光/挤压按实际失败原因提示，有检查点可复活。
6. 固定关通关显示结果并保存/解锁；每关前5榜显示时间、玩法和本次失误，采用精简成绩布局。随机仅娱乐，换图保存的是地图身份。
7. 保存失败按界面提示重试，以“成绩已保存”等完成提示确认结果；重启恢复最近已保存资料，下一局重新校准并初始化速度。

## 常见异常

- -1001：输入过期；-1002：采样间隔异常；-1003：校准超时；-1004：物理更新间隔异常。记录状态和日志，放稳后重试；结合日志中的采样和主循环时序定位原因。
- RGB错误：本次启动锁存关闭灯效，菜单显示错误，重启可重试；游戏状态继续由原流程管理。
- 真卡死由WDT兜底。串口maze_watchdog_test confirm是菜单限定主动停顿/重启测试，只有明确进行恢复实验时执行；实物复位时延列入专项测试计划。
- 串口“拒绝访问”先关闭监视器等占用程序，再核对插接/实际COM号；无需因此擦除存档。
