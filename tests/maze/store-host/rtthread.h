#ifndef STORE_HOST_RTTHREAD_H
#define STORE_HOST_RTTHREAD_H
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#define RT_ASSERT(x) assert(x)
#define RT_EOK 0
int rt_kprintf(const char *format,...);
uint32_t rt_tick_get_millisecond(void);
#define rt_malloc malloc
#define rt_free free
#endif
