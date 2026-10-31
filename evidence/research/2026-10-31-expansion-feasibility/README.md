# 扩展范围软硬件可行性证据

2026-10-31；只读硬件/SDK核查及报告，不是板端功能通过。

官方资料来源及SHA见inspection.json。PDF共6页为图像型，pypdf提取每页均无文本；保存原文并用现有pypdfium2渲染，实际查看第1、2、3页。曾尝试的fitz未安装，随后使用已有工具完成，没有安装依赖。第2页明确列出RGB WS2812B-2020、NS4150B 3W功放、喇叭接口，以及“板载马达驱动电路，预留外接马达焊接点”；不能据此认定马达或喇叭已经连接。第3页列出240MHz HCPU、512KB+64KB SRAM、8MB PSRAM、16MB NOR；不把宣传刷新率当本项目实测。

本地核对：SDK customer/boards/sf32lb52-lchspi-ulp/ptab.json、hcpu/custom_mem_map.h，example/storage/flashdb、example/rt_device/wdt及当前0.6.1生成.config。Flash分区为8MiB HCPU代码、4MiB FS_REGION及DFU/BLE专用KV区域；当前FAL仅暴露dfu/ble，未建立本游戏存档分区。当前音频/FlashDB/马达/看门狗关闭，PSRAM启用而LVGL专用PSRAM堆关闭。

课程peripheral_lab README、src/modules/test_gpio.c提供PA20短脉冲控制参考；src/lab_audio.c提供audio_open2/AUDIO_DEVICE_SPEAKER播放参考，PA42功放路径与板级pinmux一致。示例说明必须接好喇叭并人工确认；示例代码可用不等于本项目已播放/振动通过。SDK bsp_lcd_tp.c的LCD电源操作也控制PA42，音频与屏幕初始化的配合需设计时核对。

用户补充只有标记XHXDZ-30B01、疑似可按的圆柱物，身份未确认；精确型号联网检索无结果，不据此归类为喇叭/马达。后续需清晰照片确认外观、标识和引线，再安排硬件短测。不采购、不安装、不改SDK、不烧录、不推送。
