/*
 * course_starter 原始入口：串口打印 + 黄山派板载 RGB 灯颜色闪烁。
 *
 * 基于 SiFli-SDK 官方例程（Apache-2.0）：
 *   example/get-started/hello_world/rtt   —— 启动打印
 *   example/rt_device/rgb_led             —— RGB 灯驱动用法（SK6812MINI-HS，PA32，PWMT2 通道1）
 * 板型：sf32lb52-lchspi-ulp（立创·黄山派）。
 *
 * 原RGB个人修改及实测证据保留在Git与evidence；当前运行体感迷宫，启动后关灯。
 *
 * 新增 .c 文件直接放进 src/ 即可，src/SConscript 会自动收集。
 */
#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "drivers/rt_drv_pwm.h"
#include "maze_ui.h"
#include "maze_imu.h"
#include "finsh.h"

#define APP_NAME        "huangshan_maze"
#define APP_VERSION     "0.4.3-pause-feedback"

#define RGBLED_NAME     "rgbled"

static struct rt_device *rgbled_device;

static void rgb_led_init(void)
{
#ifdef SF32LB52X
    HAL_PIN_Set(PAD_PA32, GPTIM2_CH1, PIN_NOPULL, 1);   /* pwmt2_cc1 */
    /* RGB 灯供电 */
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO3_3V3, true, true);
#endif
    rgbled_device = rt_device_find(RGBLED_NAME);
    if (!rgbled_device)
    {
        rt_kprintf("[%s] ERROR: device '%s' not found, check proj.conf RGB options\n", APP_NAME, RGBLED_NAME);
        RT_ASSERT(0);
    }
}

static int maze_rgb_off(void)
{
#ifdef SF32LB52X
    pin_function function;
    PIN_ModeTypeDef mode;
    int pin_result = HAL_PIN_Get(PAD_PA32, &function, &mode, 1);
    if (pin_result < 0)
    {
        rt_kprintf("[maze_rgb] pin read failed result=%d\n",pin_result);
        return -RT_ERROR;
    }
    rt_kprintf("[maze_rgb] pin_func=%d expected=%d mode=%d\n",function,GPTIM2_CH1,mode);
    if (function != GPTIM2_CH1)
    {
        HAL_PIN_Set(PAD_PA32, GPTIM2_CH1, PIN_NOPULL, 1);
        rt_kprintf("[maze_rgb] restored PA32 PWM mapping\n");
    }
#endif
    struct rt_rgbled_configuration configuration = {0};
    int error = rt_device_control(rgbled_device, PWM_CMD_SET_COLOR, &configuration);
    rt_kprintf("[maze_rgb] black request result=%d tick=%u\n",error,rt_tick_get());
    if (error != RT_EOK) return error;
    struct rt_pwm_configuration pwm = {.channel=BSP_USING_RGBLED_CH};
    struct rt_device *pwm_device = rt_device_find(RGB_USING_SK6812MINI_HS_PWM_DEV_NAME);
    error = rt_device_control(pwm_device, PWM_CMD_GET, &pwm);
    rt_kprintf("[maze_rgb] pwm_read=%d period_ns=%u pulse_ns=%u\n",error,pwm.period,pwm.pulse);
    return error;
}
MSH_CMD_EXPORT(maze_rgb_off, Send a black frame and report the RGB driver result);

int main(void)
{
    rt_kprintf("\n[%s] v%s boot ok\n", APP_NAME, APP_VERSION);
    rgb_led_init();
    int error = maze_rgb_off();
    if (error == RT_EOK)
        error = maze_ui_init();
    if (error == RT_EOK)
        error = maze_imu_init();
    if (error != RT_EOK)
    {
        rt_kprintf("[%s] init failed error=%d\n", APP_NAME, error);
        return error;
    }

    static maze_app_t app;
    maze_app_init(&app, &maze_level_one, RT_TICK_PER_SECOND);
    maze_app_state_t last_state = app.state;
    uint32_t last_generation = app.generation;
    rt_tick_t last_log = rt_tick_get();
    rt_tick_t last_ui = last_log;
    rt_tick_t previous_loop = last_log;
    rt_tick_t last_render_cost = 0;
    maze_ui_render(&app);
    while (1)
    {
        rt_tick_t service_start = rt_tick_get();
        maze_ui_process();
        rt_tick_t now = rt_tick_get();
        rt_tick_t service_cost = now - service_start;
        rt_tick_t loop_gap = now - previous_loop;
        previous_loop = now;
        uint32_t previous_sequence = app.last_sequence;
        uint32_t previous_read = app.last_read_tick;
        maze_event_t event;
        if (maze_ui_take_event(&app, &event))
        {
            rt_kprintf("[maze] event=%d state=%d gen=%u tick=%u\n",event,app.state,app.generation,now);
            maze_app_event(&app, event, now);
        }
        maze_imu_sample_t sample;
        bool have_sample = maze_imu_latest(&sample);
        if (have_sample) maze_app_sample(&app, &sample, now);
        maze_app_update(&app, now);
        bool enabled;
        uint32_t request_generation;
        maze_app_request(&app, &request_generation, &enabled);
        maze_imu_request(request_generation, enabled);
        bool changed = app.state != last_state || app.generation != last_generation;
        if (changed)
        {
            if (app.state == MAZE_APP_DEVICE_ERROR)
            {
                rt_kprintf("[maze_timing] loop_tick=%u service_tick=%u render_tick=%u previous_seq=%u previous_read=%u\n",
                           loop_gap,service_cost,last_render_cost,previous_sequence,previous_read);
                if (have_sample)
                    rt_kprintf("[maze_timing] sample_seq=%u read_tick=%u age_tick=%u consumed_gap_tick=%u producer_gap_tick=%u producer_gap_count=%u\n",
                               sample.sequence,sample.read_start_tick,now-sample.read_start_tick,
                               sample.read_start_tick-previous_read,sample.read_gap_tick,sample.read_gap_count);
            }
            rt_kprintf("[maze] gen=%u state=%d error=%d count=%u\n",
                       app.generation, app.state, app.error, app.window_count);
            if (last_state == MAZE_APP_CALIBRATING && app.state != MAZE_APP_CALIBRATING &&
                app.has_reference && app.error == 0)
                rt_kprintf("[maze] calibrated ref=%d,%d,%d\n",
                           app.reference[0], app.reference[1], app.reference[2]);
            last_state = app.state;
            last_generation = app.generation;
        }
        if (changed || (rt_tick_t)(now-last_log) >= rt_tick_from_millisecond(500))
        {
            if (enabled || changed)
                rt_kprintf("[maze] gen=%u state=%d tick=%u ms=%lu pos=%ld,%ld input=%ld,%ld error=%d\n",
                           app.generation, app.state, now, (unsigned long)app.elapsed_ms,
                           (long)app.model.x, (long)app.model.y,
                           (long)app.input.x, (long)app.input.y, app.error);
            last_log = now;
        }
        if (changed || (rt_tick_t)(now-last_ui) >= rt_tick_from_millisecond(50))
        {
            rt_tick_t render_start = rt_tick_get();
            maze_ui_render(&app);
            last_render_cost = rt_tick_get() - render_start;
            last_ui = now;
        }
    }
    return 0;
}
