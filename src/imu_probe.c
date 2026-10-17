/* 串口IMU预研：记录指定姿态的原始加速度，不实现迷宫控制。 */
#include "rtthread.h"
#include "drivers/i2c.h"
#include "bf0_hal.h"
#include "finsh.h"
#include "maze_imu.h"
#include <stdint.h>
#include <string.h>

#define PROBE_ADDR 0x6a
#define PROBE_COUNT 40

static rt_err_t probe_read(struct rt_i2c_bus_device *bus, uint8_t reg,
                           void *data, uint16_t size)
{
    return rt_i2c_mem_read(bus, PROBE_ADDR, reg, 8, data, size) == size ? RT_EOK : -RT_EIO;
}

static rt_err_t probe_write(struct rt_i2c_bus_device *bus, uint8_t reg, uint8_t value)
{
    uint8_t bytes[2] = {reg, value};
    struct rt_i2c_msg msg = {.addr = PROBE_ADDR, .flags = RT_I2C_WR, .len = 2, .buf = bytes};
    return rt_i2c_transfer(bus, &msg, 1) == 1 ? RT_EOK : -RT_EIO;
}

/* 原始字节按小端有符号16位解码，不依赖超范围的有符号强制转换。 */
static int32_t probe_axis(const uint8_t *p)
{
    uint32_t value = (uint32_t)p[0] | ((uint32_t)p[1] << 8);
    return value >= 32768 ? (int32_t)value - 65536 : (int32_t)value;
}

/* 200ms是轮询截止；不能抢占SDK中已进入的阻塞I2C调用。 */
static rt_err_t probe_ready(struct rt_i2c_bus_device *bus)
{
    rt_tick_t start = rt_tick_get();
    do
    {
        uint8_t status = 0;
        rt_err_t error = probe_read(bus, 0x1e, &status, 1);
        if (error != RT_EOK) return error;
        if (status & 1) return RT_EOK;
        rt_thread_mdelay(5);
    } while ((rt_tick_t)(rt_tick_get() - start) < rt_tick_from_millisecond(200));
    return -RT_ETIMEOUT;
}

static int imu_probe(int argc, char **argv)
{
    if (maze_imu_owned())
    {
        rt_kprintf("[imu_probe] rejected: product sampler owns IMU\n");
        return -RT_EBUSY;
    }
    const char *poses[] = {"neutral", "right", "left", "up", "down"};
    unsigned pose;
    if (argc != 2)
    {
        rt_kprintf("Usage: imu_probe neutral|right|left|up|down\n");
        return -RT_ERROR;
    }
    for (pose = 0; pose < sizeof(poses) / sizeof(poses[0]); pose++)
        if (strcmp(argv[1], poses[pose]) == 0) break;
    if (pose == sizeof(poses) / sizeof(poses[0]))
    {
        rt_kprintf("[imu_probe] result=fail event=invalid_pose\n");
        return -RT_ERROR;
    }

    /* 按参考外设工程映射配置I2C3，只有msh诊断线程访问本探针。 */
    HAL_PIN_Set(PAD_PA40, I2C3_SCL, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA39, I2C3_SDA, PIN_PULLUP, 1);
    struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find("i2c3");
    if (!bus)
    {
        rt_kprintf("[imu_probe] result=fail event=bus_missing bus=i2c3\n");
        return -RT_ERROR;
    }
    uint8_t id = 0, old_ctrl1 = 0, old_ctrl3 = 0;
    rt_err_t error = probe_read(bus, 0x0f, &id, 1);
    if (error != RT_EOK || id != 0x6a)
    {
        rt_kprintf("[imu_probe] result=fail event=identity id=0x%02x error=%d\n", id, error);
        return -RT_ERROR;
    }
    error = probe_read(bus, 0x10, &old_ctrl1, 1);
    if (error == RT_EOK) error = probe_read(bus, 0x12, &old_ctrl3, 1);
    if (error != RT_EOK)
    {
        rt_kprintf("[imu_probe] result=fail event=save_config error=%d\n", error);
        return error;
    }
    /* 保存完整配置后才写入；只启用加速度104Hz、±2g，BDU和地址自增。 */
    error = probe_write(bus, 0x12, (old_ctrl3 & ~0x02) | 0x44);
    if (error == RT_EOK) error = probe_write(bus, 0x10, 0x40);
    rt_kprintf("[imu_probe] begin pose=%s id=0x%02x count=%d tick_hz=%d units=raw odr_hz=104\n",
               poses[pose], id, PROBE_COUNT, RT_TICK_PER_SECOND);
    if (error == RT_EOK) rt_thread_mdelay(20);
    int32_t sums[3] = {0}, lows[3] = {0}, highs[3] = {0};
    unsigned count = 0;
    while (error == RT_EOK && count < PROBE_COUNT)
    {
        uint8_t data[6];
        error = probe_ready(bus);
        if (error == RT_EOK) error = probe_read(bus, 0x28, data, sizeof(data));
        if (error != RT_EOK) break;
        int32_t axes[3];
        for (unsigned axis = 0; axis < 3; axis++)
        {
            axes[axis] = probe_axis(data + 2 * axis);
            sums[axis] += axes[axis];
            if (count == 0 || axes[axis] < lows[axis]) lows[axis] = axes[axis];
            if (count == 0 || axes[axis] > highs[axis]) highs[axis] = axes[axis];
        }
        count++;
        rt_kprintf("[imu_probe] sample pose=%s n=%u tick=%u raw=%d,%d,%d\n",
                   poses[pose], count, (unsigned)rt_tick_get(),
                   (int)axes[0], (int)axes[1], (int)axes[2]);
        rt_thread_mdelay(50);
    }
    /* 即使采样失败也尝试恢复两项配置，分别保留采样与恢复错误。 */
    rt_err_t restore = probe_write(bus, 0x10, old_ctrl1);
    rt_err_t second_restore = probe_write(bus, 0x12, old_ctrl3);
    if (restore == RT_EOK) restore = second_restore;
    if (error != RT_EOK || restore != RT_EOK)
    {
        rt_kprintf("[imu_probe] end pose=%s result=fail count=%u sample_error=%d restore_error=%d\n",
                   poses[pose], count, error, restore);
        return -RT_ERROR;
    }
    /* 控制台单次缓冲128字节，统计拆行防止极值时被截断。 */
    rt_kprintf("[imu_probe] range pose=%s min_raw=%d,%d,%d max_raw=%d,%d,%d\n",
               poses[pose], (int)lows[0], (int)lows[1], (int)lows[2],
               (int)highs[0], (int)highs[1], (int)highs[2]);
    rt_kprintf("[imu_probe] end pose=%s result=ok count=%u mean_raw=%d,%d,%d\n",
               poses[pose], count, (int)(sums[0] / PROBE_COUNT), (int)(sums[1] / PROBE_COUNT),
               (int)(sums[2] / PROBE_COUNT));
    return RT_EOK;
}
MSH_CMD_EXPORT(imu_probe, Capture 40 raw acceleration samples for a named pose);

#define CONTROL_WINDOW 20
#define CONTROL_DEAD_ZONE 300

struct control_window
{
    int32_t raw[CONTROL_WINDOW][3];
    rt_tick_t tick[CONTROL_WINDOW];
    unsigned count;
    unsigned next;
};

/* 软件读取开始时间用于保守检查；不是传感器内部采样时间。 */
static int control_input(const int32_t raw[3], const int32_t reference[3],
                         rt_tick_t sample_tick, rt_tick_t previous_tick,
                         int have_previous, rt_tick_t now, int32_t xy[2])
{
    rt_tick_t limit = rt_tick_from_millisecond(100);
    xy[0] = xy[1] = 0;
    if ((rt_tick_t)(now - sample_tick) > limit ||
        (have_previous && (rt_tick_t)(sample_tick - previous_tick) > limit)) return 0;
    int32_t delta[2] = {reference[1] - raw[1], raw[0] - reference[0]};
    for (unsigned i = 0; i < 2; i++)
    {
        if (delta[i] > CONTROL_DEAD_ZONE) xy[i] = delta[i] - CONTROL_DEAD_ZONE;
        else if (delta[i] < -CONTROL_DEAD_ZONE) xy[i] = delta[i] + CONTROL_DEAD_ZONE;
    }
    return 1;
}

/* 每次滑动一格，只在完整且稳定的20点窗口上发布参考。 */
static int control_calibrate(struct control_window *window, const int32_t raw[3],
                             rt_tick_t tick, int32_t reference[3])
{
    memcpy(window->raw[window->next], raw, sizeof(window->raw[0]));
    window->tick[window->next] = tick;
    window->next = (window->next + 1) % CONTROL_WINDOW;
    if (window->count < CONTROL_WINDOW) window->count++;
    if (window->count < CONTROL_WINDOW ||
        (rt_tick_t)(tick - window->tick[window->next]) < rt_tick_from_millisecond(950)) return 0;
    int32_t mean[3];
    for (unsigned axis = 0; axis < 3; axis++)
    {
        int32_t low = window->raw[0][axis], high = low, sum = 0;
        for (unsigned i = 0; i < CONTROL_WINDOW; i++)
        {
            int32_t value = window->raw[i][axis];
            if (value < low) low = value;
            if (value > high) high = value;
            sum += value;
        }
        if (high - low > 300) return 0;
        mean[axis] = sum / CONTROL_WINDOW;
    }
    memcpy(reference, mean, sizeof(mean));
    return 1;
}

static int imu_control_probe(int argc, char **argv)
{
    if (maze_imu_owned())
    {
        rt_kprintf("[imu_control] rejected: product sampler owns IMU\n");
        return -RT_EBUSY;
    }
    (void)argv;
    if (argc != 1)
    {
        rt_kprintf("Usage: imu_control_probe\n");
        return -RT_ERROR;
    }
    HAL_PIN_Set(PAD_PA40, I2C3_SCL, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA39, I2C3_SDA, PIN_PULLUP, 1);
    struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find("i2c3");
    if (!bus)
    {
        rt_kprintf("[imu_control] result=fail event=bus_missing\n");
        return -RT_ERROR;
    }
    uint8_t id = 0, old_ctrl1 = 0, old_ctrl3 = 0;
    rt_err_t error = probe_read(bus, 0x0f, &id, 1);
    if (error != RT_EOK || id != PROBE_ADDR)
    {
        rt_kprintf("[imu_control] result=fail event=identity id=0x%02x error=%d\n", id, error);
        return -RT_ERROR;
    }
    error = probe_read(bus, 0x10, &old_ctrl1, 1);
    if (error == RT_EOK) error = probe_read(bus, 0x12, &old_ctrl3, 1);
    if (error != RT_EOK)
    {
        rt_kprintf("[imu_control] result=fail event=save_config error=%d\n", error);
        return error;
    }
    error = probe_write(bus, 0x12, (old_ctrl3 & ~0x02) | 0x44);
    if (error == RT_EOK) error = probe_write(bus, 0x10, 0x40);
    const char *event = "configure";
    if (error == RT_EOK) rt_thread_mdelay(20);
    rt_kprintf("[imu_control] begin tick_hz=%d units=raw odr_hz=104\n", RT_TICK_PER_SECOND);
    rt_kprintf("[imu_control] calibration count=20 span_ms=950 range_raw=300 budget_ms=3000\n");
    rt_kprintf("[imu_control] input dead_zone_raw=300 max_gap_ms=100 max_age_ms=100 run_ms=5000\n");

    struct control_window window = {0};
    int32_t reference[3] = {0};
    rt_tick_t start = rt_tick_get(), previous_tick = 0;
    unsigned count = 0;
    int have_previous = 0, calibrated = 0;
    while (error == RT_EOK)
    {
        rt_tick_t budget = rt_tick_from_millisecond(calibrated ? 5000 : 3000);
        if ((rt_tick_t)(rt_tick_get() - start) >= budget)
        {
            if (!calibrated) { error = -RT_ETIMEOUT; event = "calibration_timeout"; }
            else event = "complete";
            break;
        }
        rt_tick_t sample_tick = rt_tick_get();
        uint8_t data[6];
        error = probe_ready(bus);
        if (error == RT_EOK) error = probe_read(bus, 0x28, data, sizeof(data));
        if (error != RT_EOK) { event = "read_error"; break; }
        int32_t raw[3], xy[2];
        for (unsigned axis = 0; axis < 3; axis++) raw[axis] = probe_axis(data + 2 * axis);
        rt_tick_t now = rt_tick_get();
        if (!control_input(raw, reference, sample_tick, previous_tick, have_previous, now, xy))
        {
            rt_kprintf("[imu_control] sample tick=%u valid=0 xy=0,0 event=stale_input\n", (unsigned)now);
            rt_kprintf("[imu_control] age_tick=%u gap_tick=%u\n", (unsigned)(now - sample_tick),
                       have_previous ? (unsigned)(sample_tick - previous_tick) : 0);
            error = -RT_ETIMEOUT; event = "stale_input"; break;
        }
        rt_tick_t gap = have_previous ? (rt_tick_t)(sample_tick - previous_tick) : 0;
        previous_tick = sample_tick;
        have_previous = 1;
        /* 阻塞读取返回后再次检查预算，不接受预算结束后的校准结果。 */
        if ((rt_tick_t)(now - start) >= budget)
        {
            if (!calibrated) { error = -RT_ETIMEOUT; event = "calibration_timeout"; }
            else event = "complete";
            break;
        }
        if (!calibrated)
        {
            if (control_calibrate(&window, raw, sample_tick, reference))
            {
                calibrated = 1;
                start = now;
                rt_kprintf("[imu_control] calibrated tick=%u ref_raw=%d,%d,%d\n", (unsigned)now,
                           (int)reference[0], (int)reference[1], (int)reference[2]);
                continue;
            }
        }
        else
        {
            count++;
            rt_kprintf("[imu_control] sample n=%u tick=%u valid=1 xy=%d,%d age_tick=%u gap_tick=%u\n", count,
                       (unsigned)now, (int)xy[0], (int)xy[1], (unsigned)(now - sample_tick), (unsigned)gap);
        }
        rt_thread_mdelay(50);
    }
    /* 无论本次操作如何结束，都恢复已完整保存的配置。 */
    rt_err_t restore = probe_write(bus, 0x10, old_ctrl1);
    rt_err_t second_restore = probe_write(bus, 0x12, old_ctrl3);
    if (restore == RT_EOK) restore = second_restore;
    if (error != RT_EOK || restore != RT_EOK)
    {
        rt_kprintf("[imu_control] end result=fail event=%s count=%u\n", event, count);
        rt_kprintf("[imu_control] sample_error=%d restore_error=%d\n", error, restore);
        return -RT_ERROR;
    }
    rt_kprintf("[imu_control] end result=ok count=%u\n", count);
    return RT_EOK;
}
MSH_CMD_EXPORT(imu_control_probe, Calibrate and capture 5 seconds of candidate control inputs);
