#ifndef MAZE_UI_H
#define MAZE_UI_H
#include <stdbool.h>
#include "maze_navigation.h"

/* 仅由主线程初始化与服务LVGL。 */
int maze_ui_init(void);
void maze_ui_process(void);
typedef enum { MAZE_UI_SELECT, MAZE_UI_TOGGLE, MAZE_UI_BACK, MAZE_UI_NEXT, MAZE_UI_GAME,
               MAZE_UI_SCORES, MAZE_UI_SCORE_LEVEL, MAZE_UI_SCORE_MODE, MAZE_UI_SAVE } maze_ui_command_kind_t;
typedef struct
{
    maze_ui_command_kind_t kind;
    unsigned index;
    maze_event_t game_event;
} maze_ui_command_t;
/* 装载新地图前解除旧地图场景引用，并丢弃旧页面待处理命令。 */
void maze_ui_clear(void);
bool maze_ui_take_command(const maze_navigation_t *nav, maze_ui_command_t *command);
void maze_ui_render(const maze_navigation_t *nav);

#endif
