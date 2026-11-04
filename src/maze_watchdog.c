/* SDK启动时已注册并打开wdt；应用接管喂狗，不另设线程或定时器。 */
#include "rtthread.h"
#include "drv_io.h"
#include "drivers/watchdog.h"
#include "bf0_hal.h"
#include "maze_config.h"
#include "maze_watchdog.h"
static struct rt_device *device;
static uint32_t last_completed,last_feed;
static int failed;

int maze_watchdog_boot(void)
{
    rt_hw_watchdog_hook(0);
    failed=0;
    device=rt_device_find("wdt");
    if(!device)return -RT_ERROR;
    uint32_t status=0;
    int error=rt_device_control(device,RT_DEVICE_CTRL_WDT_GET_STATUS,&status);
    if(error)return error;
    if(status!=1)return -RT_ERROR;
    uint32_t seconds=MAZE_WATCHDOG_BOOT_SECONDS;
    error=rt_device_control(device,RT_DEVICE_CTRL_WDT_SET_TIMEOUT,&seconds);
    if(error)return error;
    return rt_device_control(device,RT_DEVICE_CTRL_WDT_KEEPALIVE,RT_NULL);
}

int maze_watchdog_running(void)
{
    uint32_t seconds=MAZE_WATCHDOG_RUN_SECONDS;
    int error=rt_device_control(device,RT_DEVICE_CTRL_WDT_SET_TIMEOUT,&seconds);
    if(error)return error;
    last_completed=last_feed=rt_tick_get();
    return rt_device_control(device,RT_DEVICE_CTRL_WDT_KEEPALIVE,RT_NULL);
}

int maze_watchdog_checkpoint(uint32_t now)
{
    if(failed)return failed;
    if(now-last_completed>=MAZE_WATCHDOG_RUN_SECONDS*RT_TICK_PER_SECOND)
        return failed=-RT_ETIMEOUT;
    last_completed=now;
    if(now-last_feed<MAZE_WATCHDOG_FEED_MS*RT_TICK_PER_SECOND/1000u)return 0;
    int error=rt_device_control(device,RT_DEVICE_CTRL_WDT_KEEPALIVE,RT_NULL);
    if(error)return failed=error;
    last_feed=now;
    return 0;
}

/* IRQ内不写档、不打印、不访问UI；沿用实际PMU全芯片复位接口。 */
void wdt_store_exception_information(void)
{
    HAL_PMU_Reboot();
}
