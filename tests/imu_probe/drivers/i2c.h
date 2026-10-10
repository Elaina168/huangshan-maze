#include "rtthread.h"
struct rt_i2c_bus_device { int fixture; };
struct rt_i2c_msg { uint16_t addr, flags, len; uint8_t *buf; };
#define RT_I2C_WR 0
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name);
size_t rt_i2c_mem_read(struct rt_i2c_bus_device *, uint16_t, uint16_t, uint16_t, void *, size_t);
size_t rt_i2c_transfer(struct rt_i2c_bus_device *, struct rt_i2c_msg *, unsigned);
