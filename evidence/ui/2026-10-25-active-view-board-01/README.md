# 0.4.2用户板端反馈

来源：2026-10-25用户聊天粘贴及描述，以下为选取的原文转录，并非串口原文件或自动采集。关联REQ-002/REQ-005/REQ-006及TC-002/TC-003/TC-008/TC-009，胜败与重玩仅记录基本联调，不替代正式用例验收。版本依据本轮交付及用户上下文，未从板卡独立读取固件哈希。

```text
[maze] event=0 state=0 gen=10 tick=97243
[maze] gen=11 state=1 error=0 count=0
[maze] gen=11 state=3 error=-1003 count=20
[maze] gen=11 state=3 tick=100244 ms=0 pos=24,24 input=0,0 error=-1003
[maze] event=5 state=3 gen=11 tick=101702
[maze] gen=12 state=1 error=0 count=0
[maze] gen=12 state=2 error=0 count=20
[maze] calibrated ref=-232,-739,-16233
[maze] gen=12 state=2 tick=102733 ms=0 pos=24,24 input=0,0 error=0
[maze] gen=12 state=2 tick=110783 ms=8050 pos=312,255 input=3243,10948 error=0
[maze] gen=12 state=6 error=0 count=20
[maze] gen=12 state=6 tick=110884 ms=8147 pos=315,266 input=0,0 error=0
[maze] event=4 state=6 gen=12 tick=119515
[maze] gen=13 state=1 error=0 count=0
[maze] gen=13 state=2 error=0 count=20
[maze] calibrated ref=-223,-408,-16245
[maze] gen=13 state=2 tick=120566 ms=0 pos=24,24 input=0,0 error=0
[maze] gen=13 state=7 error=0 count=20
[maze] gen=13 state=7 tick=127447 ms=6875 pos=168,279 input=0,0 error=0
[maze] event=6 state=7 gen=13 tick=129710
[maze] gen=14 state=0 error=0 count=0
[maze] gen=14 state=0 tick=129710 ms=0 pos=24,24 input=0,0 error=0
[maze_rgb] pin_func=643 expected=477 mode=1
[maze_rgb] restored PA32 PWM mapping
[maze_rgb] black request result=0 tick=183845
[maze_rgb] pwm_read=0 period_ns=1583 pulse_ns=41
```

用户反馈：“这次倒是没问题了”，但RGB仍亮绿。

## 分析与边界

- gen11在3001ms后校准超时；count=20只表示已有20点，不保证时间跨度、稳定范围、采样连续性同时满足。具体哪个条件未满足，现有日志不能确定。
- Retry后1031ms、Replay后1051ms完成校准并直接进入游玩。20点是一次自动校准窗口，不是20次手动校准。
- gen12胜利8147ms，gen13陷阱失败6875ms，随后退出准备。用户提供的本段未出现-1002/-1004；不推定长时稳定、暂停恢复、资源或正式验收通过。
- RGB物理熄灭失败。SDK `drivers/hal/bf0_hal_pinmux.c`的`HAL_PIN_SetGptimFunc`把逻辑GPTIM功能转换为pad对应TIM功能；`drivers/cmsis/sf32lb52x/bf0_pin_const.h`中`PA32_TIM=643`、`GPTIM2_CH1=477`。两者不同不是异常证据。当前`src/main.c`比较二者并输出“restored”存在诊断错误，尚未修改该固件；不能将日志解释为已发现或修复引脚故障。
- PWM读回及黑帧返回0不证明DMA传输、输出波形或光学状态正常。没有足够证据定位RGB根因；不修改共享供电、不猜改波形。
- 下一步完全断开所有供电后观察重新上电时RGB何时亮绿，以区分旧状态保持与启动时产生；尚无该项实测。

## 上电观察补充

用户随后反馈：“插上就立即亮”。此为主观时序观察，尚无视频，不能据此断定发生在应用执行前；也未单独确认是否存在电池供电。复核SDK板型配置确认RGB控制脚32，`bsp_pinmux.c`初始化PA32为GPIO下拉；在本次搜索的板级及`example/boot_loader`源码中未发现设置绿色的RGB调用。仍不能确定物理根因，下一步需要亮灯位置及板上丝印照片，不重复要求原始四向或重新烧录。
