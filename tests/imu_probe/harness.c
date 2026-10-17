/* 用受控总线执行实际诊断源码，不证明真实硬件或调度。 */
#include <assert.h>
#include <stdbool.h>
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
static bool product_owns_sensor;
bool maze_imu_owned(void) { return product_owns_sensor; }

rt_tick_t rt_tick_get(void) { return ticks; }
rt_tick_t rt_tick_from_millisecond(int ms) { return (rt_tick_t)ms; }
void rt_thread_mdelay(int ms)
{
    ticks += (rt_tick_t)ms;
    /* 第一条控制输入之后故意让下一次读取相隔101ms。 */
    if (mode == 15 && samples == 21 && ms == 50) ticks += 51;
}
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
    if (mode == 19 && reg == 0x12) return 0;
    if (reg == 0x28) {
        if (mode == 2 && samples == 3) return 0;
        if ((mode == 13 && samples == 3) || (mode == 18 && samples == 22)) return 0;
        assert(size == 6);
        if (mode >= 10) {
            int32_t axes[3] = {-10, -420, -16187};
            if (mode == 12) axes[0] = samples % 2 ? 400 : 0;
            if (mode == 16 && samples < 10) axes[0] = 1000;
            if (mode == 11 && samples >= 20 && samples < 24) {
                const int32_t tilts[4][3] = {
                    {-10, -8052, -16187}, {-10, 8680, -16187},
                    {-6543, -420, -16187}, {7016, -420, -16187}
                };
                memcpy(axes, tilts[samples - 20], sizeof(axes));
            }
            if (mode == 14) ticks += 101;
            for (unsigned i = 0; i < 3; i++) {
                uint16_t value = (uint16_t)axes[i];
                ((uint8_t *)data)[2 * i] = (uint8_t)value;
                ((uint8_t *)data)[2 * i + 1] = (uint8_t)(value >> 8);
            }
            samples++; return size;
        }
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
    if (mode == 20 && msg->buf[0] == 0x10 && msg->buf[1] == 0x40) return 0;
    if (mode == 21 && msg->buf[0] == 0x12 && msg->buf[1] == 0x22) return 0;
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
    char *owned_control_args[] = {"imu_control_probe"};
    reset(0); product_owns_sensor = true;
    assert(imu_probe(2, args) == -RT_EBUSY);
    assert(imu_control_probe(1, owned_control_args) == -RT_EBUSY);
    assert(reads == 0 && writes == 0);
    puts("PASS: both diagnostic commands reject product IMU ownership without I/O.");
    product_owns_sensor = false;
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

    /* 若跳过稳定校准、反转轴符号或错误地继续消费陈旧样本，这些断言会失败。 */
    char *control_args[] = {"imu_control_probe"};
    reset(10); assert(imu_control_probe(1, control_args) == 0);
    assert(strstr(output, "ref_raw=-10,-420,-16187"));
    assert(strstr(output, "valid=1 xy=0,0"));
    assert(samples == 120 && ticks == 5970);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(11); assert(imu_control_probe(1, control_args) == 0);
    assert(strstr(output, "valid=1 xy=7332,0"));
    assert(strstr(output, "valid=1 xy=-8800,0"));
    assert(strstr(output, "valid=1 xy=0,-6233"));
    assert(strstr(output, "valid=1 xy=0,6726"));
    reset(12); assert(imu_control_probe(1, control_args) != 0);
    assert(strstr(output, "event=calibration_timeout"));
    assert(!strstr(output, "valid=1")); assert(ticks <= 3070);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(13); assert(imu_control_probe(1, control_args) != 0);
    assert(samples == 3 && !strstr(output, "valid=1"));
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(14); assert(imu_control_probe(1, control_args) != 0);
    assert(strstr(output, "valid=0 xy=0,0")); assert(samples == 1);
    reset(15); assert(imu_control_probe(1, control_args) != 0);
    assert(strstr(output, "valid=0 xy=0,0")); assert(samples == 22);
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(16); assert(imu_control_probe(1, control_args) == 0);
    assert(strstr(output, "ref_raw=-10,-420,-16187")); assert(samples == 130);
    reset(18); assert(imu_control_probe(1, control_args) != 0);
    assert(samples == 22 && strstr(output, "event=read_error"));
    assert(regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(19); assert(imu_control_probe(1, control_args) != 0); assert(writes == 0);
    reset(20); assert(imu_control_probe(1, control_args) != 0);
    assert(samples == 0 && regs[0x10] == 0x17 && regs[0x12] == 0x22);
    reset(21); assert(imu_control_probe(1, control_args) != 0);
    assert(strstr(output, "restore_error=8") || strstr(output, "restore_error=-8"));
    assert(regs[0x10] == 0x17 && !strstr(output, "result=ok"));
    reset(1); assert(imu_control_probe(1, control_args) != 0); assert(writes == 0);
    reset(5); assert(imu_control_probe(1, control_args) != 0); assert(reads == 0);
    reset(10); assert(imu_control_probe(2, args) != 0); assert(reads == 0);

    /* 手算死区边界、极值和100ms有效期；不借助实现生成期望值。 */
    const int32_t reference[3] = {0, 0, 0};
    const int32_t cases[][4] = {
        {300, -300, 0, 0}, {301, -301, 1, 1}, {-301, 301, -1, -1},
        {0, 0, 0, 0}, {32767, -32768, 32468, 32467}
    };
    int32_t xy[2];
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int32_t raw[3] = {cases[i][0], cases[i][1], 0};
        assert(control_input(raw, reference, 100, 0, 1, 200, xy));
        assert(xy[0] == cases[i][2] && xy[1] == cases[i][3]);
    }
    int32_t raw[3] = {1000, -1000, 0};
    assert(!control_input(raw, reference, 100, 0, 1, 201, xy));
    assert(xy[0] == 0 && xy[1] == 0);
    assert(!control_input(raw, reference, 101, 0, 1, 101, xy));
    assert(xy[0] == 0 && xy[1] == 0);
    assert(control_input(raw, reference, 20, UINT32_MAX - 29, 1, 70, xy));
    assert(xy[0] == 700 && xy[1] == 700);
    puts("PASS: 14 control command scenarios and dead-zone/freshness boundaries (injected I/O).");
    return 0;
}
