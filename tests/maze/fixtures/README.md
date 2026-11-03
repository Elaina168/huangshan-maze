# 原v1档案夹具

archive-v1.bin在本轮更改编码器前由原maze_archive_encode生成，独立Python struct/zlib逐字节复核相同。353字节，SHA256 `69f8f67cf0e169cd4b4f235c50f4db94c1a93b8668e5f307a3c8683a3cea33ef`。

全部为合成数据，不是用户真实成绩：revision17；第一关两模式已完成，第二关已解锁但未通关。每关每模式losses=(关序号+1)*10+模式，count2，前两条time_cs=1000+关序号*100+模式*10、mistakes2/3；未使用第5槽仍有time_cs=50000+关序号*10+模式、mistakes99，用于验证旧字段不丢失。其余空槽为零，原ID字符串和尾部CRC保留。

测试只在副本上修改completed/ID顺序等并重算CRC，原始golden文件不变。

## 原v2档案夹具

archive-v2.bin在第4阶段修改编码器前由原v2 maze_archive_encode实际生成，并用独立Python struct/zlib逐字节核对。517字节，SHA256 `05c9c934a2811921d9ce14e25d679971da2bdc90ef6216783f6e55a10de5dc3f`。全部为合成数据：revision29；三关已解锁，completed依次3/3/2；每关每模式的losses、count、成绩和第5槽哨兵公式同v1夹具。原v1文件保持原样。


2026-11-03新增archive-v3.bin（681B）：在修改v3编码器前，使用cc6f33a版本的真实maze_archive_decode/encode读取已有合成v2夹具，设置revision=41、第四关completed=3及正常模式成绩7654cs/2失误后生成。独立Python复核长度、版本及CRC；stage5_archive_harness验证全部原四关656记录字节保持与第五解锁。合成数据，不是用户真实存档或固件。


## 原v4档案夹具

2026-11-03在第6阶段修改编码器之前，由当时的真实v4编码器生成archive-v4.bin（845B）。全部合成数据：revision53，第五关completed3、正常模式13579cs/4失误。独立CRC复核及random_archive_harness验证原五关820B保持；不是用户档案。SHA256 `d9a881ffafea8ad1b8005c32db197a834ea27ef1ae777590a3c43c96406cc3f3`。


## 原v5档案夹具

2026-11-03在精简前使用实际v5编解码器读取v4合成夹具，设revision61、random seed3/version1及合成777cs/2失误后生成982B archive-v5.bin。仅作旧版迁移回归，旧随机成绩按新范围丢弃，不是用户真实档案。SHA-256 `a7bfb027f8d13908a8670dfb09f42518d4590d94ca1990ea5745e0d4cbbd23b2`。
