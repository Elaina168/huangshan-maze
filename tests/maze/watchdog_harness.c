/* 实际适配器配实际SDK看门狗头文件；设备/复位由受控替身提供。 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "rtthread.h"
#include "drv_io.h"
#include "drivers/watchdog.h"
#include "maze_watchdog.h"
static struct rt_device device;
static uint32_t tick,status=1,timeout;
static unsigned feeds,hooks;
static bool missing;
static int fail_command;
static jmp_buf reset;
rt_tick_t rt_tick_get(void){return tick;}
struct rt_device *rt_device_find(const char*n){assert(!strcmp(n,"wdt"));return missing?NULL:&device;}
void rt_hw_watchdog_hook(int enabled){assert(!enabled);hooks++;}
int rt_device_control(struct rt_device*d,int cmd,void*p){assert(d==&device);if(cmd==fail_command)return -8;if(cmd==RT_DEVICE_CTRL_WDT_GET_STATUS)*(uint32_t*)p=status;else if(cmd==RT_DEVICE_CTRL_WDT_SET_TIMEOUT)timeout=*(uint32_t*)p;else {assert(cmd==RT_DEVICE_CTRL_WDT_KEEPALIVE);feeds++;}return 0;}
void HAL_PMU_Reboot(void){longjmp(reset,1);}
extern void wdt_store_exception_information(void);
int main(void){
 assert(!maze_watchdog_boot()&&hooks==1&&timeout==30&&feeds==1);
 assert(!maze_watchdog_running()&&timeout==5&&feeds==2);
 assert(!maze_watchdog_checkpoint(249)&&feeds==2);
 assert(!maze_watchdog_checkpoint(250)&&feeds==3);
 assert(!maze_watchdog_checkpoint(251)&&feeds==3);
 /* 一个完成点晚于5秒，即使设备尚未复位，也不能继续喂。 */
 assert(maze_watchdog_checkpoint(5252)==-RT_ETIMEOUT&&feeds==3);
 assert(maze_watchdog_checkpoint(5253)==-RT_ETIMEOUT&&feeds==3);
 tick=UINT32_MAX-100;assert(!maze_watchdog_boot());assert(!maze_watchdog_running());unsigned before=feeds;
 assert(!maze_watchdog_checkpoint(148)&&feeds==before);
 assert(!maze_watchdog_checkpoint(149)&&feeds==before+1);
 fail_command=RT_DEVICE_CTRL_WDT_KEEPALIVE;assert(maze_watchdog_checkpoint(399)==-8);fail_command=0;
 assert(maze_watchdog_checkpoint(649)==-8);
 missing=true;assert(maze_watchdog_boot()==-RT_ERROR);missing=false;
 status=0;assert(maze_watchdog_boot()==-RT_ERROR);status=1;
 fail_command=RT_DEVICE_CTRL_WDT_SET_TIMEOUT;assert(maze_watchdog_boot()==-8);fail_command=0;
 if(!setjmp(reset)){wdt_store_exception_information();assert(0);}
 puts("PASS: actual watchdog adapter removes idle feed; startup30/run5, 250ms throttle, wrap, late completion latch, device failures, timeout hook calls HAL reboot");
}
