# v0.1.1 启动与重复启动证据

日期：2026-10-10。来源：用户对话粘贴的串口记录，用户此前报告长按尝试与 RGB 切换；未记录精确按下/松开时间，不能根据本记录计算复位按键时长。

源码提交 `092062439aca9f248940120bcaeaacc2e3f1be28`，对应固件哈希见 build-state.json，下载证据见 download-attempt-03.md。

## 关键接收片段摘录

下列保留用户粘贴中的时间与分包形式；只摘录定位需要的行，不冒充完整原始日志。

```text
16:56:56 RX
Serial:c2,Chip:4,Package:3,Rev:3
16:56:56 RX
  Reason:00000040

16:56:56 RX
1


[course_starter] v0.1.1 bo
16:56:56 RX
ot ok
Hello world!
16:56:56 RX
[course_starter] rgb -> red
msh
16:56:56 RX
 />
16:56:57 RX
[course_starter] rgb -> green
16:56:59 RX
[course_starter] rgb -> blue

16:57:05 RX
Serial:c2,Chip:4,Package:3,Rev:3
16:57:05 RX
  Reason:00000000

16:57:05 RX
1


[course_starter] v0.1.1 bo
16:57:05 RX
ot ok
Hello world!
16:57:05 RX
[course_starter] rgb -> red
msh
16:57:05 RX
 />
16:57:07 RX
[course_starter] rgb -> green
16:57:08 RX
[course_starter] rgb -> blue
16:57:09 RX
[course_starter] rgb -> red
16:57:10 RX
[course_starter] rgb -> green
16:57:12 RX
[course_starter] rgb -> blue
```

## 结论与边界

- 相邻接收片段按顺序合并，两次均为 `[course_starter] v0.1.1 boot ok`。串口分包并非缺失启动输出。
- 两次系统启动输出后应用从 red 开始并继续 green、blue，确认本次版本进入 main 以及重复启动后恢复循环；不能仅凭“好像红灯重新开始”判断复位，本次使用启动日志作证。
- 保留 Reason:00000040 与 Reason:00000000，尚未核对复位原因位定义，不解读为具体故障或按键来源。用户是否连续按键、断电等时序未记录，不能推定意外重启。
- 秒级接收时间戳及颜色日志没有记录灭灯时刻，不能验证实际亮 1000 ms/灭 250 ms；源码参数已核对，物理时长仍待观察/测量。
- 本人代码解释尚未取得记录，C-06 完整能力验收不自动标为通过。迷宫产品用例仍未执行。
