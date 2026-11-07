# 方案设计（最终实现）

本设计对应src现状，初版方案和历次取舍见[历史架构](../superpowers/specs/2026-10-17-maze-architecture-design.md)。只有main/UI线程操作LVGL；IMU线程发布受保护快照。

## 系统架构

```mermaid
flowchart TB
    touch[触摸与LVGL] --> ui[maze_ui 显示与命令]
    ui --> main[main 唯一UI线程与调度]
    main --> nav[maze_navigation 页面与地图生命周期]
    nav --> app[maze_app 状态 校准 输入时效]
    app --> model[maze_model 物理与机关]
    nav --> maps[maze_map / maze_catalog / maze_random]
    main --> progress[maze_progress 结算 排行 解锁]
    progress --> archive[maze_archive v7 编解码]
    main --> store[maze_store / FlashDB / FAL]
    store --> archive
    model --> ui
    main --> rgb[maze_feedback 纯时序 + RGB设备]
    main --> wdt[maze_watchdog 完整轮次喂狗]
    imu[maze_imu 后台线程 I2C3] -->|受保护采样快照| app
    app -->|代次与采样使能| imu
```

## 数据流与时间

```mermaid
flowchart LR
    raw[IMU raw三轴/读起始tick/序号/代次/错误] --> check[拒绝旧代次/重复/超时采样]
    check --> calib[稳定窗口求参考值]
    calib --> input[相对参考值 轴映射 反转 死区]
    input --> physics[固定步推进 速度 阻力 碰撞与机关]
    physics --> snapshot[坐标 速度 键/开关/破墙 检查点 胜负]
    snapshot --> camera[相机与局部黑暗遮挡]
    camera --> frame[LVGL单视口缓冲]
    frame --> done[实际flush完成屏障]
    done --> loop[返回外层完成点才喂狗]
```

IMU通过I2C3访问0x6a，发布加速度raw三轴等字段；应用工作循环每次读后请求等待50ms，不把硬件ODR104Hz等同应用104Hz。校准窗口、死区和时效由config维护。物理常规10ms步；动态碰撞采用更细步进以防穿过移动机关。真实服务/显示时间仍须区分请求周期与板端测量。

main在等待LCD提交时仅推进采样/导航纯逻辑，不做LVGL重入、存储、随机生成或RGB硬件I/O，也不喂狗。只有完整主循环返回才证明主循环有进展。已识别的设备异常仍显示可操作错误页；传感器错误不是立即重启的理由。

## 状态机

```mermaid
stateDiagram-v2
    [*] --> READY
    READY --> CALIBRATING: START
    CALIBRATING --> PLAYING: 新局校准成功
    CALIBRATING --> PAUSED: 暂停重校准或故障恢复成功
    CALIBRATING --> CALIBRATION_FAILED: 超时
    CALIBRATION_FAILED --> CALIBRATING: RETRY
    PLAYING --> PAUSED: PAUSE
    PAUSED --> PLAYING: RESUME且采样有效
    PAUSED --> CALIBRATING: RECALIBRATE
    PLAYING --> DEVICE_ERROR: 输入或调度错误
    DEVICE_ERROR --> CALIBRATING: RETRY
    PLAYING --> WON: 到终点
    PLAYING --> LOST: 陷阱或危险机关
    LOST --> PLAYING: RESPAWN检查点复活
    LOST --> CALIBRATING: REPLAY
    WON --> CALIBRATING: REPLAY
```

图中复活事件对应实际MAZE_EVENT_RESPAWN。页面层另有MAZE_NAV_SELECT/GAME/SCORES/GENERATING和错误页；返回/取消/换图由navigation结束旧会话并递增代次，迟到数据不能覆盖新关。原状态枚举以maze_app.h为准。

## 存储与资源

固定目录按稳定ID装载；地图有独立几何/对象表和检查工作区。随机采用受限骨架/模板、分片检查和双地图发布，静态连通性不被描述为所有随机种子的完整动态可解性证明。

当前写356B v7，读旧v1至v6（最大982B）；旧正常/反转各榜合并后只保留5条，保留三关解锁及随机身份。未知版本保护，失败不冒充保存成功。Flash独立A/B各32768B、擦除8192B，下载不覆盖存档；真实SDK库配NOR受控中断不是物理断电试验。

显示390×450，游戏视口350×300，单份390×300 RGB565缓冲234000B。相机只移动局部视口对象，黑暗用四块不透明遮挡；合并球的新旧损伤区域并等待实际提交完成，避免球闪烁、源缓冲覆写和残留黑块。最终静态RAM436156/523264B，余87108B；主栈配置8192B。链接余量不等于运行free或完整调用路径最坏栈证明。
