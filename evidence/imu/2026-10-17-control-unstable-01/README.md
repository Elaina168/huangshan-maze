# 持续晃动：校准超时记录

归档日期2026-10-17。用户按此前持续改变倾角的指导，直接粘贴串口输出；转录为[raw.log](raw.log)，不是附件原字节。转录文件SHA-256 `f6b4f408c9bd8e69cc4829134530f44831f78fcb8ee9633f9822804927c3b21b`，来源登记在import-manifest.json；[analysis.json](analysis.json)保存事件核对和证据边界。

日志包含begin与参数行，随后`end result=fail event=calibration_timeout count=0`及`sample_error=-2 restore_error=0`。没有calibrated反馈或有效控制记录。实际SDK rtdef.h定义RT_ETIMEOUT=2，与代码的-RT_ETIMEOUT校准失败路径一致；这里sample_error字段沿用通用错误汇总，不代表已经证明I2C读取失败。count=0指未输出控制阶段记录，不代表没有读取校准样本。

本次符合“不满足校准稳定条件时拒绝进入控制阶段”的候选预研预期，result=fail在该场景属于预期保护行为。restore_error=0表示程序报告两项配置恢复写入成功，未另行回读寄存器。RGB日志在命令后继续，不把它当作IMU恢复成功证据。

日志标注3000ms校准逻辑预算，但没有开始/结束tick或原始校准窗口，不能独立重算实际墙钟耗时、窗口范围或物理晃动幅度。具体未满足的范围/跨度不能从该汇总单独分离。没有设备固件校验值，构建关联见[记录](../../builds/2026-10-10-imu-control-01/README.md)。

下一步保持中立、重新发送imu_control_probe，全程放稳，保存校准反馈、控制输出与end，验证失败后可重新成功。尚未得到这一步的实际日志，不能宣称恢复重试通过。产品TC-003仍未执行；本组仅本地保存，等用户说“今天到这”后再提交推送。
