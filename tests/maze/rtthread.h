#ifndef MAZE_TEST_RTTHREAD_H
#define MAZE_TEST_RTTHREAD_H
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
typedef int rt_err_t;
typedef uint32_t rt_tick_t;
typedef uint32_t rt_uint32_t;
typedef int32_t rt_int32_t;
typedef int16_t rt_int16_t;
typedef uint8_t rt_uint8_t;
typedef size_t rt_size_t;
struct rt_mutex { int locked; };
struct rt_semaphore { unsigned count; };
struct rt_thread { void (*entry)(void *); void *parameter; };
#define RT_NULL NULL
#define RT_EOK 0
#define RT_ERROR 1
#define RT_ETIMEOUT 2
#define RT_EIO 8
#define RT_EBUSY 7
#define RT_TICK_PER_SECOND 1000
#define RT_WAITING_FOREVER -1
#define RT_IPC_FLAG_PRIO 1
#define RT_ASSERT assert
rt_tick_t rt_tick_get(void);
rt_tick_t rt_tick_from_millisecond(int ms);
void rt_thread_mdelay(int ms);
int rt_kprintf(const char *fmt, ...);
rt_err_t rt_mutex_init(struct rt_mutex *, const char *, rt_uint8_t);
rt_err_t rt_mutex_take(struct rt_mutex *, rt_int32_t);
rt_err_t rt_mutex_release(struct rt_mutex *);
rt_err_t rt_sem_init(struct rt_semaphore *, const char *, unsigned, rt_uint8_t);
rt_err_t rt_sem_take(struct rt_semaphore *, rt_int32_t);
rt_err_t rt_sem_release(struct rt_semaphore *);
rt_err_t rt_thread_init(struct rt_thread *, const char *, void (*)(void *), void *, void *, rt_uint32_t, rt_uint8_t, rt_uint32_t);
rt_err_t rt_thread_startup(struct rt_thread *);
#endif
