#ifndef MAZE_UI_H
#define MAZE_UI_H
#include <stdbool.h>
#include "maze_app.h"

/* 仅由主线程初始化与服务LVGL。 */
int maze_ui_init(void);
void maze_ui_process(void);
bool maze_ui_take_event(const maze_app_t *app, maze_event_t *event);
void maze_ui_render(const maze_app_t *app);

#endif
