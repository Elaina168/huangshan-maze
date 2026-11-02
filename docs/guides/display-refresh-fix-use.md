当前正式第4阶段见[0.11.0四关基础机关指南](stage4-use.md)。以下保留历史版本说明；v3保存后旧正式固件不能读取新档。

# 0.10.1小球闪烁与黑块复测

本版针对第三关球闪烁、遗留黑块后重绘才恢复的问题，在每笔LCD提交后等待实际读取完成，再允许复用绘图缓冲。仍是原三关、原手感、原存档；无需清档。

关闭串口监视器，确认当前板子仍对应COM6后执行；若端口变了，只替换实际端口：

```powershell
Set-Location 'C:\Users\zytx\AppData\Local\Temp\huangshan-maze-ccn8gy48\project\build_sf32lb52-lchspi-ulp_hcpu'
& 'D:\sifli\tools\tools\sftool\0.2.5\sftool.exe' -p COM6 -c SF32LB52 -m nor write_flash 'bootloader\bootloader.bin@0x12010000' 'main.bin@0x12020000' 'ftab\ftab.bin@0x12000000'
```

启动应为`v0.10.1-display-sync`，并看到`[maze_draw] buffer_bytes=234000 single=1 rows=300 completion_wait=1`。main.bin613508B，SHA-256 `946211627277703e28a1c7a54773ae4794bac768e22ed034e455473c42aa92b9`，详见[本次完整构建](../../evidence/builds/2026-11-02-display-sync-01/README.md)。只写上述三项，不整片擦除。

只补这一轮相关测试，不用重做整批：

1. 第三关走原来容易闪球/黑块的位置，既试镜头不动时的小球移动，也试横纵滚动和折返。球是否仍闪、走过的地图是否留下黑块；停住后看地图是否完整。
2. 暂停/继续、重玩各一次，确认位置和视角正常。
3. 贴几行原路径的`[maze]`日志，保留`physics_gap_tick`、`flush_services`；若出现-1004或其他错误，连同`[maze_timing]`前后输出贴来。新等待必须仍满足原100ms限制，不调大阈值。

主机异步替身354笔已验证源覆盖0、每次球128完整且黑洞0；这不替代实屏结果。TC-058本轮显示缺陷待新板端反馈，其他未提供的量化资源仍待证，旧B/C不恢复。没有自动烧录、提交或推送。

2026-11-02复测反馈：用户回复“OK没问题了”，本轮显示问题总体正常，见[归档](../../evidence/ui/2026-11-02-display-sync-board-01/README.md)。未提供的量化日志仍待证，不要求重复这一轮已反馈正常的观察。
