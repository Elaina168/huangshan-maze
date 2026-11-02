# 原v1档案夹具

archive-v1.bin在本轮更改编码器前由原maze_archive_encode生成，独立Python struct/zlib逐字节复核相同。353字节，SHA256 `69f8f67cf0e169cd4b4f235c50f4db94c1a93b8668e5f307a3c8683a3cea33ef`。

全部为合成数据，不是用户真实成绩：revision17；第一关两模式已完成，第二关已解锁但未通关。每关每模式losses=(关序号+1)*10+模式，count2，前两条time_cs=1000+关序号*100+模式*10、mistakes2/3；未使用第5槽仍有time_cs=50000+关序号*10+模式、mistakes99，用于验证旧字段不丢失。其余空槽为零，原ID字符串和尾部CRC保留。

测试只在副本上修改completed/ID顺序等并重算CRC，原始golden文件不变。

## 原v2档案夹具

archive-v2.bin在第4阶段修改编码器前由原v2 maze_archive_encode实际生成，并用独立Python struct/zlib逐字节核对。517字节，SHA256 `05c9c934a2811921d9ce14e25d679971da2bdc90ef6216783f6e55a10de5dc3f`。全部为合成数据：revision29；三关已解锁，completed依次3/3/2；每关每模式的losses、count、成绩和第5槽哨兵公式同v1夹具。原v1文件保持原样。
