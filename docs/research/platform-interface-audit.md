# 平台接口静态核对与预研入口

日期：2026-10-10。性质：读取现有源码的静态核对，不是本项目 IMU、LVGL 或控制效果实测。关联 C-11、C-15、C-16、C-17，REQ-002、REQ-003、REQ-011。

## 1. 已核对来源

本机参考外设工程 `D:/sifli/peripheral_lab`，SDK `D:/sifli/SiFli-SDK/v2.5.1`。以下路径相对相应根目录；参考工程不随本项目整体复制，未来复用时登记具体文件、许可和修改。

| 来源 | 当前材料中的精确内容 | 结论边界 |
|---|---|---|
| 外设工程 `src/modules/test_imu.c` | `lab_i2c("i2c3")`；I2C 地址 `0x6A`；WHO_AM_I 寄存器 `0x0F`，预期 ID `0x6A` | 参考实现的器件探测方式；当前板卡 ID 未保存实测 |
| 外设工程 `src/lab_bus.c` | PA40→I2C3_SCL、PA39→I2C3_SDA；`rt_i2c_mem_read(bus, addr, reg, 8, data, size)`；读满字节才成功 | 引脚来自参考应用，未来仍需核对实际 BSP 和板卡，不能将应用代码当作电路证明 |
| SDK `rtos/rtthread/components/drivers/include/drivers/i2c.h`、`rtos/rtthread/components/drivers/i2c/i2c_core.c` | 存在 `rt_i2c_mem_read` 的声明与实现 | 该 SDK 版本具备接口；不证明本工程启用了 I2C3 |
| SDK `customer/peripherals/sensor/LSM6DSL/libraries/lsm6dsl_reg.h` | `LSM6DSL_ID=0x6AU`；`LSM6DSL_WHO_AM_I=0x0F`；角速度起始 `0x22`，加速度起始 `0x28` | SDK 寄存器定义与参考读取顺序一致 |
| 外设工程 `src/modules/test_imu.c` | 保存 0x10/0x11/0x12；配置 104 Hz、±2 g/±245 dps；等待就绪后读取 12 字节，结束尝试恢复配置 | 参考采样方法；104 Hz 为器件配置，不是应用处理或屏幕帧率 |
| 外设工程 `src/modules/test_game.c`、`src/lab_game.h` | 后台读取并发布 `lab_motion_t` 副本；字段 `a[3]`、`g[3]`、`sequence`、`valid`；循环延时 20 ms | 可研究后台与 UI 分离；这些不是本项目已建立的接口 |
| 外设工程 `src/main.c` | `littlevgl2rtt_init("lcd")`；主线程执行 `lv_timer_handler()`；等待限于 2–20 ms | SDK 显示接入参考；不用其等待值直接承诺本项目响应指标 |
| SDK `middleware/lvgl/littlevgl2rtt.h`、`littlevgl2rtt.c` | `rt_err_t littlevgl2rtt_init(const char *name)` | 初始化 API 已核对；本工程还没有显示配置和界面 |
| 外设工程 `project/proj.conf` | `CONFIG_BSP_USING_I2C3=y`、`CONFIG_PKG_USING_LITTLEVGL2RTT=y`、`CONFIG_LVGL_V9=y` | 启用配置的候选来源；依赖仍需按 SDK 核对，不能只复制三个键就声称完整 |

初始仓库 `project/proj.conf` 只有四条 RGB 配置。本轮增加I2C3和串口IMU诊断命令，main.c仍为RGB例程；未加入LVGL界面或迷宫功能。实际取样按 [操作指导](../guides/imu-mapping-probe.md) 执行，不把主机注入或构建当作真实IMU通过。

## 2. 待测问题与最小取证

| 问题 | 取证方法 | 当前状态 |
|---|---|---|
| 四向映射 | 固定屏幕朝向与握姿，记录中立、右低、左低、上低、下低各段原始加速度；对比相对中立变化 | 未执行；不套用自行车示例的符号 |
| 中立波动与校准 | 固定姿态连续取样，保留时间戳、有效样本数、各轴范围和失败；之后才确定校准判据、死区 | 未执行 |
| 采样时效 | 记录采集和消费时刻、序号、读失败；测量延迟与陈旧情况，再确定有效时限 | 未执行；参考快照字段没有采集时间戳，未来需设计 |
| UI 与采样隔离 | 设计后台采样与有保护的快照；UI 主线程创建对象并刷新；明确停止和迟到数据处理 | 未实现，三图与接口尚待设计 |
| 配置与资源 | 逐步启用所需显示/I2C配置，实际构建并读取堆、栈等数据 | 未执行；RGB 静态占用不代表迷宫预算 |

下一步先完成 [本人修改练习](../guides/first-personal-change.md)，再形成预研代码的具体设计与测量计划。数值结果不足时保持待测，不将静态源码核对填入 TC-004/005/014/021 的通过结果。
