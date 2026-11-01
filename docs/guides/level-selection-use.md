# 0.8.0双关与方向设置短测

USB在屏幕右侧。使用本版对应构建目录烧录，先关闭串口监视器释放实际串口；COM6只是此前使用的端口，若设备端口变化以实际识别为准。不自动烧录。

本次已构建固件目录如下。接好板子后先确认实际端口，若仍是COM6，执行：

```powershell
Set-Location 'C:\Users\zytx\AppData\Local\Temp\huangshan-maze-2cwkj3h2\project\build_sf32lb52-lchspi-ulp_hcpu'
& 'D:\sifli\tools\tools\sftool\0.2.5\sftool.exe' -p COM6 -c SF32LB52 -m nor write_flash 'bootloader\bootloader.bin@0x12010000' 'main.bin@0x12020000' 'ftab\ftab.bin@0x12000000'
```

main.bin592148字节，SHA-256为`e2abea6c663b20cafb1a5ee9c10ce3bc39ee57f53b4efd6d083d07d93fb56c8b`，启动标识`v0.8.0-level-selection`。完整构建与哈希不代替设备运行证据。

一批完成以下新增检查，原有A/B/C不用重做：

1. 上电看到两关选关页，默认“控制方向：正常”。选择冰岔迷宫，点击开始、放稳校准，确认正常四向、暂停/继续仍可用。
2. 暂停后点选关，选择冰弯迷宫。确认新图起点和用时归零，必须重新开始校准，无上一关残留滑动；在冰面转角、陷阱旁体验新图。
3. 返回选关切换反转，开始任一关，确认左右和上下均反向，标题显示反转；重玩仍保持反转。
4. 第一关通关点下一关，确认进入第二关准备页、零用时且仍需显式开始；第二关通关显示全部完成，无下一关按钮。
5. 最后返回选关，重启确认默认正常。保留此次`[maze_nav]`、`[maze_map]`与异常时的`[maze_timing]`日志，说明触摸、显示及操作是否正常。

这是新增功能短测，不将口头“正常”扩展为逐项量化通过。`check_ticks`需结合启动日志`tick_hz`换算实际装载耗时，不用主机耗时替代。错误装载/迟到事件已在受控主机/离屏验证，不要求故意烧录损坏地图。
