/* 主机设备接口替身，仅运行入口时序测试。 */
struct rt_device { int unused; };
struct rt_device *rt_device_find(const char *name);
int rt_device_control(struct rt_device *,int,void *);
