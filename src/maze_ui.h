#ifndef MAZE_UI_H
#define MAZE_UI_H
#include <stdbool.h>
#include "maze_navigation.h"

/* 仅由主线程初始化与服务LVGL。 */
int maze_ui_init(void);
void maze_ui_process(void);
typedef enum { MAZE_UI_SELECT, MAZE_UI_TOGGLE, MAZE_UI_BACK, MAZE_UI_NEXT, MAZE_UI_GAME,
               MAZE_UI_SCORES, MAZE_UI_SCORE_LEVEL, MAZE_UI_DARK, MAZE_UI_SAVE, MAZE_UI_RANDOM, MAZE_UI_RANDOM_REPLACE, MAZE_UI_RANDOM_CANCEL } maze_ui_command_kind_t;
typedef struct
{
    maze_ui_command_kind_t kind;
    unsigned index;
    maze_event_t game_event;
} maze_ui_command_t;
/* 装载新地图前解除旧地图场景引用，并丢弃旧页面待处理命令。 */
void maze_ui_clear(void);
void maze_ui_loading(void);
bool maze_ui_take_command(const maze_navigation_t *nav, maze_ui_command_t *command);
void maze_ui_render(const maze_navigation_t *nav);
/* 外层主循环取可选RGB颜色；只返回状态，不执行设备I/O。 */
bool maze_ui_rgb(uint32_t now,uint32_t tick_hz,bool enabled,uint32_t *color);

/* 条带提交后的协作服务只处理采样/物理，不得调用LVGL或消费触摸事件。 */
void maze_ui_set_service(void (*service)(void *),void *context);

#ifdef MAZE_SCROLL_TRIAL
void maze_ui_render_trial(const maze_app_t *app);
bool maze_ui_take_trial_event(const maze_app_t *app,maze_event_t *event);
#endif
#endif
