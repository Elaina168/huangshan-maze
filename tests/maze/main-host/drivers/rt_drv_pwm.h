/* 不连接PWM硬件。 */
#define PWM_CMD_SET_COLOR 1
#define PWM_CMD_GET 2
#define BSP_USING_RGBLED_CH 1
#define RGB_USING_SK6812MINI_HS_PWM_DEV_NAME "pwm"
struct rt_rgbled_configuration { unsigned color_rgb; };
struct rt_pwm_configuration { unsigned channel,period,pulse; };
