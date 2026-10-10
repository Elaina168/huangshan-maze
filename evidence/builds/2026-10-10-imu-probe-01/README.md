# 串口IMU诊断构建记录

日期：2026-10-10。诊断代码与人工维护配置由本记录所在提交及build-state.json的源码哈希绑定。SDK v2.5.1，HEAD `421126d9f476ed8e2a6f0b0ca28a9f241c182e65`；原CodeKit环境脚本激活，无安装或SDK修改。

## 实际结果

- 最终构建退出码0，三项固件非空且重算哈希匹配，完整源码快照与当前文件一致。
- Kconfig严格UTF-8/ASCII检查通过；实际生成配置启用BSP_USING_I2C3，main.elf包含命令注册符号__cmd_imu_probe。
- main.bin 299024 B，SHA-256 `454c192f90080d698c9ba652e5e8545e11baa6fe168c206b9541f76e2a8573ee`。
- 静态ROM used 325331 B、RAM used 57128 B，仅构建占用，不是运行堆或栈高水位。
- 日志保留SDK/快照Git查询、RWX、syscall stub与ftab entry等警告，未宣称已修复。
- 未烧录本次诊断固件；hardware_verified=false。RGB旧版本的板端证据不证明新IMU读取通过。

## 主机验证

先执行主机测试，因探针源码尚不存在而编译失败；加入实现后，真实src/imu_probe.c通过7个受控I/O场景：完整40点与正负极值解码统计、错误ID不写配置、读失败恢复、就绪超时恢复、恢复写失败不报成功、总线不存在、非法姿态不访问I2C。随后拆短统计日志以适配128字节控制台，重新运行主机测试仍通过并重新构建最终固件。

WSL使用已有/usr/bin/cc与python3，不安装依赖。快照回归4/4通过。主机桩没有真实总线超时、板端调度和电气，因此不替代器件读取和方向映射证据。

## 本次下载

已激活SDK的PowerShell中，先停止串口监视，再执行：

```powershell
Set-Location 'C:\Users\zytx\AppData\Local\Temp\huangshan-maze-jsvpfspl\project\build_sf32lb52-lchspi-ulp_hcpu'
.\uart_download.bat
```

当前确认为COM6才输入6。实际sftool_param.json与日志已归档；本目录没有二进制，不在此目录运行下载。启动仍显示v0.1.1，诊断构建按新命令与此固件哈希辨认。用户取样方法见 [IMU指导](../../../docs/guides/imu-mapping-probe.md)。
