/* 桩只替代RT调度和I2C边界，执行真实后台线程入口直至空闲等待。 */
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "maze_imu.c"

static struct rt_i2c_bus_device fake_bus;
static struct rt_thread *started_thread;
static jmp_buf idle;
static uint8_t registers[256];
static unsigned raw_reads, writes, lock_depth, starts;
static rt_tick_t ticks;
static const char *scenario;
static char output[4096];
static size_t used;

rt_tick_t rt_tick_get(void) { return ticks; }
rt_tick_t rt_tick_from_millisecond(int ms) { return (rt_tick_t)ms; }
int rt_kprintf(const char *fmt, ...)
{
    assert(lock_depth == 0);
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(output + used, sizeof(output) - used, fmt, args);
    va_end(args);
    assert(n >= 0 && used + (size_t)n < sizeof(output));
    used += (size_t)n;
    return n;
}
rt_err_t rt_mutex_init(struct rt_mutex *m, const char *n, rt_uint8_t f)
{ (void)n; (void)f; m->locked = 0; return 0; }
rt_err_t rt_mutex_take(struct rt_mutex *m, rt_int32_t timeout)
{ (void)timeout; assert(!m->locked); m->locked = 1; lock_depth++; return 0; }
rt_err_t rt_mutex_release(struct rt_mutex *m)
{ assert(m->locked && lock_depth == 1); m->locked = 0; lock_depth--; return 0; }
rt_err_t rt_sem_init(struct rt_semaphore *s, const char *n, unsigned value, rt_uint8_t f)
{ (void)n; (void)f; s->count = value; return 0; }
rt_err_t rt_sem_release(struct rt_semaphore *s) { s->count++; return 0; }
rt_err_t rt_sem_take(struct rt_semaphore *s, rt_int32_t timeout)
{
    (void)timeout; assert(lock_depth == 0);
    if (s->count == 0) longjmp(idle, 1);
    s->count--; return 0;
}
rt_err_t rt_thread_init(struct rt_thread *t, const char *n, void (*entry)(void *),
                        void *p, void *stack, rt_uint32_t size, rt_uint8_t priority, rt_uint32_t slice)
{ (void)n; (void)stack; (void)size; (void)priority; (void)slice; t->entry=entry; t->parameter=p; return 0; }
rt_err_t rt_thread_startup(struct rt_thread *t) { starts++; started_thread=t; return 0; }

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name)
{ assert(lock_depth==0 && strcmp(name,"i2c3")==0); return strcmp(scenario,"bus_missing")==0 ? NULL : &fake_bus; }
size_t rt_i2c_mem_read(struct rt_i2c_bus_device *b, uint16_t addr, uint16_t reg,
                       uint16_t bits, void *data, size_t size)
{
    assert(lock_depth==0 && b==&fake_bus && addr==0x6a && bits==8);
    if (reg==0x12 && strcmp(scenario,"save_failed")==0) return 0;
    if (reg==0x28)
    {
        raw_reads++;
        if (strcmp(scenario,"read_failed")==0 || strcmp(scenario,"queued_failure")==0) return 0;
        if (raw_reads==1 && strcmp(scenario,"late_read")==0) maze_imu_request(8,true);
        if (raw_reads==1 && strcmp(scenario,"stop_during_read")==0) maze_imu_request(8,false);
        const uint8_t axes[6]={0,128,255,127,255,255};
        assert(size==6); memcpy(data,axes,6); return size;
    }
    assert(size==1);
    if (reg==0x1e && strcmp(scenario,"stop_during_status")==0) maze_imu_request(8,false);
    *(uint8_t *)data = reg==0x1e ? (strcmp(scenario,"not_ready")==0 ? 0 : 1) : registers[reg];
    return size;
}
size_t rt_i2c_transfer(struct rt_i2c_bus_device *b, struct rt_i2c_msg *m, unsigned n)
{
    assert(lock_depth==0 && b==&fake_bus && n==1 && m->addr==0x6a && m->len==2);
    writes++;
    if (strcmp(scenario,"configure_failed")==0 && m->buf[0]==0x10 && m->buf[1]==0x40) return 0;
    if (strcmp(scenario,"restore_failed")==0 && m->buf[0]==0x10 && m->buf[1]==0x17) return 0;
    registers[m->buf[0]]=m->buf[1]; return 1;
}
void rt_thread_mdelay(int ms)
{
    assert(lock_depth==0); ticks+=(rt_tick_t)ms;
    if (ms!=50) return;
    maze_imu_sample_t sample;
    assert(maze_imu_latest(&sample));
    assert(sample.error==0 && sample.sequence==raw_reads);
    assert(sample.raw[0]==-32768 && sample.raw[1]==32767 && sample.raw[2]==-1);
    if (strcmp(scenario,"gap_latch")==0)
    {
        assert(sample.read_gap_tick==(raw_reads==1?0u:raw_reads==2?110u:50u));
        assert(sample.read_gap_count==(raw_reads==1?0u:1u));
        if (raw_reads==1) { ticks+=60; return; }
        if (raw_reads==2) return;
    }
    if (strcmp(scenario,"cadence")==0)
    {
        assert(sample.read_gap_tick==(raw_reads==1?0u:50u));
        if (raw_reads==1) return;
    }
    if (strcmp(scenario,"late_read")==0 && raw_reads==1)
    { assert(sample.generation==7); return; }
    assert(sample.generation==(strcmp(scenario,"late_read")==0 ? 8u : 7u));
    maze_imu_sample_t copy=sample;
    sample.raw[0]=0;
    assert(maze_imu_latest(&sample) && sample.raw[0]==copy.raw[0]);
    maze_imu_request(9,false);
    assert(!maze_imu_latest(&sample));
}
int main(int argc, char **argv)
{
    assert(argc==2); scenario=argv[1];
    registers[0x0f]=0x6a; registers[0x10]=0x17; registers[0x12]=0x22;
    assert(!maze_imu_owned());
    assert(maze_imu_init()==0 && maze_imu_owned() && starts==1);
    maze_imu_sample_t sample;
    assert(!maze_imu_latest(&sample));
    if (strcmp(scenario,"queued_failure")==0)
    { maze_imu_request(5,true); maze_imu_request(6,false); }
    maze_imu_request(7,true);
    maze_imu_request(7,true);
    if (setjmp(idle)==0) started_thread->entry(started_thread->parameter);
    assert(lock_depth==0 && starts==1);
    if (strcmp(scenario,"read_failed")==0 || strcmp(scenario,"configure_failed")==0 ||
        strcmp(scenario,"save_failed")==0 || strcmp(scenario,"bus_missing")==0 || strcmp(scenario,"not_ready")==0 ||
        strcmp(scenario,"queued_failure")==0)
    {
        assert(maze_imu_latest(&sample) && sample.generation==7 && sample.error!=0);
        if (strcmp(scenario,"not_ready")==0) assert(sample.error==-RT_ETIMEOUT && ticks>=200);
        if (strcmp(scenario,"save_failed")==0 || strcmp(scenario,"bus_missing")==0) assert(writes==0);
    }
    if (strcmp(scenario,"restore_failed")==0)
    {
        assert(strstr(output,"restore_error=-8"));
        assert(maze_imu_latest(&sample) && sample.generation==7 && sample.error==-RT_EIO);
    }
    else assert(registers[0x10]==0x17 && registers[0x12]==0x22);
    if (strcmp(scenario,"late_read")==0) assert(raw_reads==2);
    if (strcmp(scenario,"cadence")==0) assert(raw_reads==2);
    if (strcmp(scenario,"gap_latch")==0) assert(raw_reads==3);
    if (strcmp(scenario,"stop_during_read")==0) assert(raw_reads==1);
    if (strcmp(scenario,"stop_during_status")==0) assert(raw_reads==0);
    if (strcmp(scenario,"queued_failure")==0) assert(raw_reads==1);
    printf("PASS: %s (injected I/O and scheduler, not hardware)\n",scenario);
    return 0;
}
