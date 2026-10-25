# UI事件输出归档首次失败

先前同一WSL调用编译运行UI事件测试已退出0。后续归档调用试图直接重用/tmp/huangshan-ui-events，返回CreateProcessCommon: execvpe(/tmp/huangshan-ui-events) failed: No such file or directory，退出1。此时临时可执行文件已不存在，未推断它消失的具体原因；不是UI断言失败。该归档调用中的host-maze/probe已退出0，快照测试因前一步失败尚未执行。

改为每次同一WSL调用内编译并运行，不依赖跨调用临时文件。后续host-ui-events.log保存重跑标准输出，快照测试单独执行。
