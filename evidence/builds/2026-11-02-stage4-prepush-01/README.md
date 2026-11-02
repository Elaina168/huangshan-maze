# 提交字节与构建源码一致性复核

2026-11-02，0.11.0-basic-mechanisms，normal。在Git暂存发现5个源文件仅换行CRLF/LF差异后，统一为LF并明确.h的LF属性；代码内容不变。原始SDK构建配置与下载脚本用精确属性保留原字节/空行，不修改原始证据。

重新执行完整build_firmware，退出0/source_unchanged=true；逐项独立核对当前所有源码、ASCII快照及三份固件SHA。构建目录：C:\Users\zytx\AppData\Local\Temp\huangshan-maze-ogzfqx9z/project/build_sf32lb52-lchspi-ulp_hcpu。

三产物大小与SHA均与原stage4-01完全相同：main621308B/b3b61b922562680c1fb08c94c7c485beca106a4be5485e964b9312809d9d9e5d，boot58496B/e59d70db44367d524fbb46a8a014ce0092462e86959d101a8e2e00ad27bb5e21，ftab11288B/36b02f55309740be689cadcf8632141b31a454ccdcfdd56a71f56134a016d500。无需因这次换行规范化重新烧录。原[功能/ARM资源证据](../2026-11-02-stage4-01/README.md)仍适用；当前提交产品源码字节以后面这份LF快照为准。

hardware_verified=false保持构建脚本原值，不伪造设备哈希核验；用户先前总体反馈另存。没有修改SDK或玩法。
