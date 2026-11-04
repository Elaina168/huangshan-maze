#ifndef MAZE_WATCHDOG_H
#define MAZE_WATCHDOG_H
#include <stdint.h>
/* 仅main拥有喂狗权；flush服务和其他线程不能调用完成点。 */
int maze_watchdog_boot(void);
int maze_watchdog_running(void);
int maze_watchdog_checkpoint(uint32_t now);
#endif
