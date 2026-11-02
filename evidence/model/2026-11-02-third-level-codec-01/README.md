# 三关接入Task 1：档案编解码

2026-11-02，REQ-039/TC-055。仅纯C档案与测试，未改Flash读写、正式地图或UI，无构建/烧录/提交推送。

baseline.log：原353B v1编码器与原测试先通过，再由它生成合成golden archive-v1.bin。独立Python struct/zlib逐字节一致，SHA和字段说明见tests/maze/fixtures/README.md。

red.log：新测试先在旧代码上因档案仍353B/两关失败。green.log：v2 517B/三关、旧字段含未使用榜单槽保留、修订号17不变、第二关completed0/1/2/3映射第三关锁定、v1换序及v2六种ID排列归位、所有截断/超长/CRC/版本/ID/排序/标志边界以及失败输出不变通过。新测试已加入scripts/test_maze.py。

sanitizer.log为ASan/UBSan针对双格式与外部边界验证。full-suite.log保留扩大回归的实际结果，不能只报告局部绿而忽略阶段间尚未适配的业务边界。物理存档层仍是下一任务，不可烧录此中间版本。
