# 首次板端故障修复的自动验证

日期2026-10-25，版本0.4.1-ui-flush-fix；基线提交52346af加本地未提交改动。实际失败见[用户聊天转录](../2026-10-25-first-board-failure-01/README.md)。没有修复后真机通过结论。

## 失败→修复→回归

1. 使用SDK真实LVGL v9.4.0离屏渲染（RGB565，单个40行局部缓冲，软件渲染，无显示器/触摸设备），旧版校准→游玩刷新175500像素，断言start_pixels<100000失败。保留旧UI源码before-ui-fix.c.txt。改为一次建立地图/球/文字、切页只重建按钮和显隐提示，初步79792，缩小提示区后70492；暂停→游玩同70492，单像素移动9518。测试严格C99编译，真实输出host-ui-refresh.log；不能由离屏像素数推断实际目标耗时达标。
2. 增加skipped_snapshot复现后台每51ms采样、UI消费102ms跨度但update仅52ms、age0时原app误报-1002，真实断言RED。read_gap_tick改为由生产者记录实际间隔，保留100ms；消费跨样不再冒充传感器断档。
3. 独立审查发现仅记录最近间隔会丢失“101ms坏间隔随后50ms健康间隔”的历史。hidden_read_gap真实断言RED后增加会话内read_gap_count，app消费累计变化，保留超限事实；新generation重新归零。gap_latch测试验证0/110/50ms三条仍保留count1，校准遇历史异常只清窗一次后仍可20点成功。before-gap-latch.c.txt保存中间版本。

最终host-maze.log：后台13/校准8/模型7/状态9组全部通过；host-probe.log：原探针7+14及所有权守卫通过；UI事件3组通过；Windows快照4/4通过；项目材料检查另在最终文档后执行。所有主机测试均不是硬件证明。

## 原生刷新测试复现

已有WSL与CMake/Make/GCC，不安装依赖。SDK作为只读输入，构建缓存在忽略的build内：

```sh
cmake -S /mnt/d/codex/黄山派/tests/maze/lvgl-host -B /mnt/d/codex/黄山派/build/lvgl-host -DLVGL_DIR=/mnt/d/sifli/SiFli-SDK/v2.5.1/external/lvgl_v9
cmake --build /mnt/d/codex/黄山派/build/lvgl-host -j8
/mnt/d/codex/黄山派/build/lvgl-host/ui-refresh
```

一次尝试用直接cc命令代替CMake重链时未找到lvgl.h，未据此修改生产源码或冒称功能RED；使用已验证的CMake配置重建成功。原生测试采用默认软件渲染，目标采用自身SDK/EPIC路径及双缓冲，其时间与资源必须实测；此处只验证真实LVGL失效区域。

## 审查与已知限制

独立Codex只读复查UI持久对象、事件生命周期和新增诊断，无Important/Critical；后续gap元数据复查提出历史断档丢失Important，上述第3项已复现并修复，复查确认代码闭环。审查者不修改文件、不运行测试，不把交接构建信息当实测。执行者完成测试与源/产物hash核验。

RGB黑帧返回不等于物理熄灭；maze_rgb_off仅提供运行后发黑帧与真实返回。共享LDO不动，RGB根因仍未确认。新增[maze_timing]可定位目标侧service/render/loop及生产/消费间隔。最终构建见[记录](../../builds/2026-10-25-ui-flush-fix-01/README.md)，按[指导](../../../docs/guides/maze-use.md)只复测进入游玩、Resume和绿灯，不提前跑完整验收。

Codex实现/测试/分析与记录，用户提供实际故障及Resume补测，未人工逐行修正代码。未提交推送。
