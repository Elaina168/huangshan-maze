# 第3阶段存储门槛与软件验证

所有结果为主机/离屏/配置证据，不是板端Flash掉电实测，未烧录。

## 失败与修正

- `archive-red.log`、`store-red.log`、`progress-red.log`：接口尚不存在时RED；档案353字节显式小端、IEEE CRC32、ID/版本/数量/排序边界随后GREEN。
- `store-attempt-01.log`：SDK原样FlashDB的首次创建、交替提交、重启及失败重试通过。库使用fdb.c/fdb_kvdb.c/fdb_utils.c，SHA及实际命令保存在日志。
- `store-attempt-02.log`、`store-attempt-03.log`：普通/GC旧新完整恢复通过，但首次建库中断operation33/prefix1，再boot错误返回前offset16从FF变07。原因是not_formatable不禁止KV恢复标记写入。
- 新增逐副本写闸门，启动探测只读，提交仅目标bank可写。`store-attempt-04.log`为原4KiB候选模型修复后通过；随后实际SDK flash2 FAL查明blk_size8192，不能把该日志当目标粒度结果。
- `store-8k-gate.log`为实际8KiB配置：普通7个写/擦操作×5代表位置、GC14×5、首次初始化22×5，共215次强制进程中断运行。已有档案重启均为完整旧/新档，当前源副本字节不变；首次初始化得到有效默认档或明确错误且原字节保持。小操作的代表位置可能重复，不称穷尽电气故障。
- 未来版本、同序号不同内容、未知非空均保留；读取错误、部分写错误、末次读回失败与同序号重试单独注入通过。NOR模拟只允许1→0和8KiB对齐擦除，跨进程保留镜像，强制退出后不执行库清理。

## 后端与统计

`partition-red.log`保留项目表尚未创建时失败；分区脚本确认原区域不变、A/B各32KiB、无img字段。`proxy-green.log`编译真实maze_store_flash.c，验证物理地址映射、只读/单副本/范围约束、8KiB对齐及rt_flash_*错误传播。实际板级驱动仍须上板验证。

`progress-green.log`、`model-progress.log`及`final-model.log`覆盖原物理/校准/状态/地图回归以及挑战时间累计、暂停重玩、设备恢复排除、排行及共享解锁。代码复核指出原测试设置second后又被start辅助函数覆盖，已补真实第二关落坑/通关、模式隔离和末关边界，并验证完全同分新成绩排在旧同分之后，最终日志为准。

## UI与复核

`ui-red.log`是/fast增量链接旧头布局对象引起的缓存混用崩溃，不当作产品缺陷；正常依赖构建及独立全量编译自身源码复现正确的新功能RED，分别见ui-behavior-red日志。`ui-attempt-01.log`为旧测试仍匹配“已暂停”精确文本，而本版增加失误提示；更新断言后`ui-attempt-02.log`及最终ui-final通过。

真实SDK LVGL验证锁关、空榜/前5、成绩模式、未保存重试、启动错误页及原校准/暂停反馈，活动刷新阈值不变。157字形/18839像素字节，实际LVGL A8与源TTF逐像素核对及生成再现通过，含5个奇数宽glyph；快照4项通过。

PNG由实际离屏PPM转换，不是实物照片。scores-full-uncompleted-fixture.png保留了旧受控输入“有榜但未置完成标记”的不一致fixture画面，后续修正fixture后重新导出正常画面，不当作真实业务状态。

独立只读代码复核无Critical/Important。Minor三项：第二关测试缺口已补；未保存时断电回退说明已补入玩家使用/短测指南；文档同步8KiB及maze_flash代理。SDK原代码未修改，主机64位编译的原库格式警告保留，不冒称已修复。完整目标构建及板端状态另见构建记录/开发日志。
