# 校准、死区与时效串口诊断构建

日期：2026-10-10。当前源码由本记录所在提交及[build-state.json](build-state.json)的完整源码哈希绑定；SDK v2.5.1，HEAD `421126d9f476ed8e2a6f0b0ca28a9f241c182e65`。复用CodeKit已有环境与已有WSL编译器，无安装、SDK修改或共享总线超时变更。

## 验证结果

- 实际板型sf32lb52-lchspi-ulp，构建退出码0；完整源码清单前后及当前文件一致，三项固件存在且重算SHA-256匹配。
- 生成Kconfig严格UTF-8/ASCII通过，BSP_USING_I2C3已启用；main.map包含`__fsym___cmd_imu_control_probe`及`__fsym___cmd_imu_probe`注册项。
- main.bin为300980 B，SHA-256 `e7fb51afa76ecf204cf0de1515285a1fe7f9dbf732738f50c5a865b7c615823f`。
- 静态ROM used 328098 B、RAM used 57128 B；不代表运行堆、msh栈高水位或长期资源验收。
- [last_build.log](last_build.log)保留SDK/快照Git查询、RWX、syscall stub及ftab entry等已有警告，未宣称已修复。
- 未烧录，`hardware_verified=false`；所有迷宫产品TC保持未执行。

先新增实际C源码的主机测试，执行因缺少`imu_control_probe`与`control_input`而编译失败；加入实现后通过。最终[host-tests.log](host-tests.log)包含原7个探针场景、14个控制命令场景及死区/时效边界：稳定/持续晃动/晃动后稳定、四向、校准及控制中读取失败、100/101ms年龄与间隔、总线/ID/参数错误、保存/配置/恢复失败、±300及±301死区、原始极值与输入时效tick回绕。复用WSL的cc/Python，C99及-Wall/-Wextra/-Werror；[snapshot-tests.log](snapshot-tests.log)为快照回归4/4通过。这些是注入I/O与受控时钟，不代替真实传感器或调度。

独立只读审查未发现重要问题。非阻断覆盖缺口：尚未直接测试校准范围300/301、跨度949/950、Y/Z轴不稳定及整个命令跨tick回绕；已测输入时效函数回绕不等于整个命令回绕已测。128字节控制台下最坏有效记录约94字节。SDK阻塞读取可能超出3秒校准逻辑预算；本轮保留这一限制。

## 本轮下载

停止串口监视，在已激活SDK的PowerShell执行：

```powershell
Set-Location 'C:\Users\zytx\AppData\Local\Temp\huangshan-maze-wt7d5cvq\project\build_sf32lb52-lchspi-ulp_hcpu'
.\uart_download.bat
```

当前识别仍是COM6时输入6。先切换到实际构建目录，避免跨盘调用脚本时相对固件路径错误。本证据目录未复制二进制，不能在此运行下载；完整烧录配置见[sftool_param.json](sftool_param.json)。下载结束后重连1000000波特率、8N1、无硬件流控，按[真机指导](../../../docs/guides/imu-control-probe.md)先执行稳定中立组。

后续运行证据：用户提供[首次中立附件](../../imu/2026-10-10-control-neutral-01/README.md)，校准成功反馈及99条有效零输入已核对。build-state.json保留构建时未烧录的历史状态；附件没有下载或设备固件校验值，不改写其hardware_verified，也不把单组预研视为全部诊断或产品验收完成。
