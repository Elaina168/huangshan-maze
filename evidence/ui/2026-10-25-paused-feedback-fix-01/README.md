# 暂停重校准失败提示修复验证

日期2026-10-25。原问题：[A批实测](../2026-10-25-batch-a-01/README.md)。设计见架构附录F，关联REQ-008/TC-011。

## RED与GREEN

使用SDK实际LVGL、真实maze_ui/app/model/level源码和已有离屏CMake工程。新增断言先执行：暂停校准超时，首次显示-1003，后续健康采样使app.error清0后标题仍应有-1003。旧实现在第二次标题断言失败，退出134，见`native-red.log`；不是缺依赖或编译失败。

仅UI新增暂停校准结果缓存，当前实时错误优先；在离开暂停或下一次校准时清除。app的参考、实时错误、Resume校验和旧参考恢复完全不改。

修复后`native-green.log`退出0：健康采样后失败标题仍保留；位置/用时不变；Resume正常转游玩并清提示；下一次成功校准仍暂停且无旧失败提示。原切换刷新33292/33292、移动9518像素断言通过，没有扩大活动切页失效范围。不将软件渲染像素数当作目标显示耗时。

同轮主机逻辑回归退出0，后台13/校准8/模型7/状态9 PASS，输出`host-regression.log`。这些是本次修复必要回归，不是重启用户已停止的B/C测试安排。

命令：`wsl --exec sh -c 'cmake --build /mnt/d/codex/黄山派/build/lvgl-host -j8 && /mnt/d/codex/黄山派/build/lvgl-host/ui-refresh'`；原输出分别保存为native-red/green-build.log及native-red/green.log。`wsl --exec python3 /mnt/d/codex/黄山派/scripts/test_maze.py`用于逻辑回归。原WSL提示编码异常保留，不伪称无工具提示。

0.4.3-pause-feedback完整构建与一致性结果见[构建证据](../../builds/2026-10-25-paused-feedback-01/README.md)。新版未烧录，实际屏幕提示效果未验证，不据主机结果宣称板端通过。未提交推送。
