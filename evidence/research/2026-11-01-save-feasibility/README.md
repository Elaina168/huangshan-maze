# 第3阶段存档接口核查

日期2026-11-01，只读SDK v2.5.1及0.8.0生成配置，未写板端Flash、未改SDK或项目配置。

- 实际板表：`D:/sifli/SiFli-SDK/v2.5.1/customer/boards/sf32lb52-lchspi-ulp/ptab.json`，Flash基址0x12000000，FS偏移0x009A0000/4MiB，DFU/BLE KV偏移0x00DA0000/0x00DA4000各16KiB；最后定义末端0x00DA8000。候选新增游戏区域偏移0x00DB0000/64KiB，未修改既有范围。
- 实际板级hcpu/custom_mem_map.h定义FAL表，仅dfu/ble，设备NOR_FLASH2_DEV_NAME。通用fal_cfg.h中的flash1示例地址不适用，不照抄。
- tools/build/building.py的GetCustomMemMapSrc优先查项目/板名、项目/芯片及项目根，再查板级custom_mem_map.h。tools/build/ptab.py支持项目本地完整ptab；当前板表是旧JSON格式，不默认套用v3 overlay。
- external/FlashDB/Kconfig有PKG_USING_FLASHDB、PKG_FDB_USING_FAL_MODE与PKG_FLASHDB_ERASE_GRAN，后者NOR默认4096。inc/flashdb.h提供KV初始化/blob读写/control；fdb_def.h有FDB_KVDB_CTRL_SET_NOT_FORMAT。
- src/fdb_kvdb.c中写blob失败会内部重试一次，kvdb_assert默认0；KV与sector恢复/格式化路径需实测。禁止仅凭API名字宣称掉电一致性通过。

0.8.0生成ptab.h与板表FS/DFU/BLE地址一致。板上未分配区域是否已擦除、真实擦写粒度/耗时及本次持久化均未实测。后续以构建生成表、真实库故障注入和板端重启结果补证。

独立复核发现not_formatable在任一扇区头损坏时会于GC/KV恢复前返回FDB_READ_ERR（实际源码1814/1884行）。具体设计因此提出两个32KiB独立数据库交替保存，不依赖单库禁止格式化来推断恢复保证。`partition-check.json`记录实际板表SHA及对候选A/B区域的静态地址/对齐/容量检查，未读写设备。真实库中断验证仍是实施第一步的门槛。
