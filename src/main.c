/*
 * course_starter 原始入口：串口打印 + 黄山派板载 RGB 灯颜色闪烁。
 *
 * 基于 SiFli-SDK 官方例程（Apache-2.0）：
 *   example/get-started/hello_world/rtt   —— 启动打印
 *   example/rt_device/rgb_led             —— RGB 灯驱动用法（SK6812MINI-HS，PA32，PWMT2 通道1）
 * 板型：sf32lb52-lchspi-ulp（立创·黄山派）。
 *
 * 原RGB个人修改及实测证据保留在Git与evidence；当前改为显示/触摸预研，启动后关灯。
 *
 * 新增 .c 文件直接放进 src/ 即可，src/SConscript 会自动收集。
 */
#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "drivers/rt_drv_pwm.h"
#include "maze_ui.h"
#include "maze_imu.h"

#define APP_NAME        "huangshan_maze"
#define APP_VERSION     "0.2.2-imu-input-probe"

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

static int rgb_led_off(void)
{
    struct rt_rgbled_configuration configuration;
    configuration.color_rgb = 0;
    return rt_device_control(rgbled_device, PWM_CMD_SET_COLOR, &configuration);
}

int main(void)
{
    rt_kprintf("\n[%s] v%s boot ok\n", APP_NAME, APP_VERSION);
    rgb_led_init();
    int error = rgb_led_off();
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
    maze_app_init(&app, RT_NULL, RT_TICK_PER_SECOND);
    maze_app_state_t last_state = app.state;
    rt_tick_t last_log = rt_tick_get();
    rt_tick_t last_ui = last_log;
    while (1)
    {
        maze_ui_process();
        rt_tick_t now = rt_tick_get();
        if (maze_ui_take_sample_toggle())
        {
            if (app.state == MAZE_APP_CALIBRATING || app.state == MAZE_APP_INPUT_READY)
                maze_app_cancel(&app);
            else
                maze_app_begin(&app, now);
        }
        maze_imu_sample_t sample;
        if (maze_imu_latest(&sample)) maze_app_sample(&app, &sample, now);
        maze_app_update(&app, now);
        bool enabled = app.state == MAZE_APP_CALIBRATING || app.state == MAZE_APP_INPUT_READY;
        maze_imu_request(app.generation, enabled);
        bool changed = app.state != last_state;
        if (changed)
        {
            rt_kprintf("[maze_input] gen=%u state=%d error=%d count=%u\n",
                       app.generation, app.state, app.error, app.window_count);
            if (app.state == MAZE_APP_INPUT_READY)
                rt_kprintf("[maze_input] calibrated ref=%d,%d,%d\n",
                           app.reference[0], app.reference[1], app.reference[2]);
            last_state = app.state;
        }
        if (changed || (rt_tick_t)(now-last_log) >= rt_tick_from_millisecond(500))
        {
            if (enabled)
                rt_kprintf("[maze_input] gen=%u state=%d count=%u xy=%ld,%ld\n",
                           app.generation, app.state, app.window_count,
                           (long)app.input.x, (long)app.input.y);
            last_log = now;
        }
        if (changed || (rt_tick_t)(now-last_ui) >= rt_tick_from_millisecond(50))
        {
            maze_ui_show_input(&app);
            last_ui = now;
        }
    }
    return 0;
}
