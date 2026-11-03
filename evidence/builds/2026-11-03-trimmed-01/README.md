# 三固定关＋随机娱乐正式构建

2026-11-03，0.14.0-three-and-casual，normal变体。完整SDK退出0/source_unchanged=true；当前全部源码、ASCII快照、三产物SHA独立一致。目录：

`C:\Users\zytx\AppData\Local\Temp\huangshan-maze-jadgg27u\project\build_sf32lb52-lchspi-ulp_hcpu`

| 产物 | 字节 | SHA-256 |
|---|---|---|
| main.bin | 643252 | c46a2b7141b8a3bdbc3f4ba45ed52acba87d34c2730e64e310430c57abc2efb2 |
| bootloader.bin | 58496 | e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21 |
| ftab.bin | 11288 | 12e3c34dcfb825a41de1988b0f174684186d4f419640949c0315c05de677133e |

实际ELF仅保留ice_loops/mechanism_trial/dynamic_trial及随机生成器，frost_fork/second/scroll_trial图符号不存在；前两图源码已删。旧ID字串仅在存档迁移读取端保留，不是隐藏游戏入口。随机成绩成员、命令/入口及结算写入已删除。

实际静态RAM436816/523264B，余86448B；BSS329612B，比0.13.1少2304B。draw_buffer仍234000B，不增加图像缓存；主栈配置8192B保持，实际序言见prologs.txt，未做完整SDK最坏栈证明。v6记录524B，旧格式读取最大982B；原A/B各32768B/扇区8192B与三文件下载地址保持，uart_download.txt可核。没有存档镜像或整片擦除。

中间构建也已完成，但自查最终准备页旧排行榜文案后重新冻结完整构建；中间元数据保留，不交接旧快照。SDK原LVGL空指针静态分析/RWX/newlib系统调用/ftab入口等警告保留，不修改SDK。

软件/迁移及真实LVGL见[阶段证据](../../trimmed-2026-11-03/README.md)。本版板端未烧录，TC-077待用户，不提交推送。
