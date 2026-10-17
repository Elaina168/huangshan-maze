#ifndef PROBE_TEST_RTTHREAD_H
#define PROBE_TEST_RTTHREAD_H
#include <stdint.h>
#include <stddef.h>
typedef int rt_err_t;
typedef uint32_t rt_tick_t;
#define RT_EOK 0
#define RT_ERROR 1
#define RT_EIO 8
#define RT_ETIMEOUT 2
#define RT_EBUSY 7
#define RT_TICK_PER_SECOND 1000
rt_tick_t rt_tick_get(void);
rt_tick_t rt_tick_from_millisecond(int ms);
void rt_thread_mdelay(int ms);
int rt_kprintf(const char *fmt, ...);
#endif
