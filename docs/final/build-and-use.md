# 构建、烧录与使用说明

作者李天宇（2024211450）；程序0.16.1-ui-trim。

## 环境与源码复现

实际基线：黄山派sf32lb52-lchspi-ulp、SiFli SDK v2.5.1、SDK内RT-Thread版本宏3.1.3、LVGL版本宏9.4.0、arm-none-eabi-gcc14.2.1；Python运行项目脚本，Git管理历史。SDK/RTOS/LVGL来源与许可见[第三方清单](../../THIRD_PARTY.md)，SDK官方仓库为https://github.com/OpenSiFli/SiFli-SDK 。不将SDK和个人Python环境打包。

安装并按实际SDK的说明激活环境后，在工程根目录执行：

```powershell
python scripts/check_project.py
python scripts/build_firmware.py
```

不要直接在中文工程路径调用SCons。build_firmware.py复制project/src到新ASCII临时目录，逐文件核对并构建；完整结果在build/last_build.log和build/build-state.json。仅returncode=0、source_unchanged=true、三固件均存在才能报告构建通过，下载目录从本次state读取，不复用过期临时路径。

普通固件构建不需要重生成字体。需要改中文文字时才使用既有Pillow与SDK内实际TTF运行scripts/generate_ui_font.py，并以scripts/test_ui_font.py验证；生成C字体已随源码提供。

Git按.gitattributes规范部分文件换行；交付包工程的project/src恢复成已验证构建快照原字节，并同时检查Git归一化内容与封版提交一致，manifest记录对应快照SHA。重新检出后可能出现只含换行的字节差异，不应因此伪称产品代码改变。

## 软件验证复现

纯逻辑脚本在已具备cc的环境执行python3 scripts/test_maze.py。main、WDT、FlashDB测试还需给出实际SDK目录：

```text
python3 scripts/test_maze_main.py --sdk <SDK路径>
python3 scripts/test_maze_watchdog.py --sdk <SDK路径>
python3 scripts/test_maze_store.py --sdk <SDK路径>
```

真实LVGL主机目标配置在tests/maze/lvgl-host，LVGL_DIR指向SDK/external/lvgl_v9；目标ui-stage7、ui-refresh、ui-async、ui-scroll。这些均不能替代实物验收。

## 固件下载

最终ZIP根目录的firmware包含bootloader/bootloader.bin、main.bin、ftab/ftab.bin、原uart_download.bat和manifest.json。烧录前关闭占用串口的监视器，确认实际板卡和串口。COM6仅为本项目曾用串口，不能保证其他机器仍相同。

在包内firmware目录，用实际sftool.exe路径和实际串口执行，例如实际串口为COM6时：

```powershell
& '<sftool.exe完整路径>' -p COM6 -c SF32LB52 -m nor write_flash 'bootloader\bootloader.bin@0x12010000' 'main.bin@0x12020000' 'ftab\ftab.bin@0x12000000'
```

尖括号必须替换。当前使用sftool0.2.5；原批处理依赖PATH，找不到sftool时使用实际完整路径。不要整片擦除，不下载存档bank，不用开着的串口监视器同时烧录。三固件SHA/字节与源码提交、tag、配置在manifest和SHA256SUMS中核对。

## 操作

1. 上电查看中文选关页，正常显示方向USB在右侧。默认RGB菜单低亮呼吸，供电BAT指示灯不受灯效控制。
2. 选择已解锁固定关或随机挑战。选关页可设置黑暗/反转组合，进入关卡后模式冻结。
3. 点击开始，放稳板卡，20点稳定窗口约1秒后新局开始。轻倾加速、反向制动，冰面更滑、粗糙地面减速。
4. 暂停可继续、校准、重玩或选关。暂停重校准和故障恢复成功先回暂停，再点继续。
5. 按关卡使用钥匙/开关开门，撞破棕墙、触发检查点或避开动态机关。落坑/激光/挤压按实际失败原因提示，有检查点可复活。
6. 固定关通关显示结果并保存/解锁；每关前5榜显示时间、玩法和本次失误，不再显示累计失误。随机仅娱乐，换图保存的是地图身份。
7. 保存失败按界面提示重试，不把“正在保存”当成功；重启恢复最近已保存资料，不恢复当前速度或校准。

## 常见异常

- -1001：输入过期；-1002：采样间隔异常；-1003：校准超时；-1004：物理更新间隔异常。记录状态和日志，放稳后重试；不要通过删除保护阈值掩盖故障。
- RGB错误：本次启动锁存关闭灯效，菜单显示错误，重启可重试；不影响正常游戏状态。
- 真卡死由WDT兜底。串口maze_watchdog_test confirm是菜单限定主动停顿/重启测试，只有明确进行恢复实验时执行；本交付未宣称已测实物复位时间。
- 串口“拒绝访问”先关闭监视器等占用程序，再核对插接/实际COM号；无需因此擦除存档。
