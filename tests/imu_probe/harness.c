/* 用受控总线执行实际诊断源码，不证明真实硬件或调度。 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "imu_probe.c"

static struct rt_i2c_bus_device bus;
static uint8_t regs[256];
static unsigned writes, reads, samples, mode;
static rt_tick_t ticks;
static char output[24000];
static size_t used;

rt_tick_t rt_tick_get(void) { return ticks; }
rt_tick_t rt_tick_from_millisecond(int ms) { return (rt_tick_t)ms; }
void rt_thread_mdelay(int ms) { ticks += (rt_tick_t)ms; }
int rt_kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(output + used, sizeof(output) - used, fmt, args);
    va_end(args);
    assert(n >= 0 && used + (size_t)n < sizeof(output));
    used += (size_t)n;
    return n;
}
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name)
{ assert(strcmp(name, "i2c3") == 0); return mode == 5 ? NULL : &bus; }
size_t rt_i2c_mem_read(struct rt_i2c_bus_device *b, uint16_t addr, uint16_t reg,
                     uint16_t bits, void *data, size_t size)
{
    assert(b == &bus && addr == 0x6a && bits == 8);
    reads++;
    if (reg == 0x28) {
        if (mode == 2 && samples == 3) return 0;
        assert(size == 6);
        /* 覆盖负极值、正极值和零的符号解码与统计。 */
        const uint8_t raw[] = {0, 128, 255, 127, 0, 0};
        memcpy(data, raw, size); samples++; return size;
    }
    assert(size == 1);
    *(uint8_t *)data = reg == 0x1e ? (mode == 3 ? 0 : 1) : regs[reg];
    return size;
}
size_t rt_i2c_transfer(struct rt_i2c_bus_device *b, struct rt_i2c_msg *msg, unsigned n)
{
    assert(b == &bus && n == 1 && msg->addr == 0x6a && msg->len == 2);
    writes++;
    if (mode == 4 && msg->buf[0] == 0x10 && msg->buf[1] == 0x17) return 0;
    regs[msg->buf[0]] = msg->buf[1];
    return 1;
}
static void reset(unsigned scenario)
{
    memset(regs, 0, sizeof(regs)); regs[0x0f] = scenario == 1 ? 0 : 0x6a;
    regs[0x10] = 0x17; regs[0x12] = 0x22;
    mode = scenario; writes = reads = samples = ticks = 0;
    used = 0; output[0] = 0;
}
int main(void)
{
    char *args[] = {"imu_probe", "neutral"};
    reset(0); assert(imu_probe(2, args) == 0); assert(samples == 40);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    assert(strstr(output, "mean_raw=-32768,32767,0"));
    assert(strstr(output, "result=ok"));
    reset(1); assert(imu_probe(2, args) != 0); assert(writes == 0);
    reset(2); assert(imu_probe(2, args) != 0); assert(samples == 3);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(3); assert(imu_probe(2, args) != 0); assert(samples == 0 && ticks < 1000);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(4); assert(imu_probe(2, args) != 0);
    assert(strstr(output, "restore_error=")); assert(!strstr(output, "result=ok"));
    assert(regs[0x12] == 0x22);
    reset(5); assert(imu_probe(2, args) != 0); assert(reads == 0 && writes == 0);
    reset(0); args[1] = "wrong"; assert(imu_probe(2, args) != 0); assert(reads == 0 && writes == 0);
    puts("PASS: 7 IMU probe scenarios (injected I/O, not hardware).");
    return 0;
}
