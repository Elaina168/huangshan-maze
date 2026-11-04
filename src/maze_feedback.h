#ifndef MAZE_FEEDBACK_H
#define MAZE_FEEDBACK_H
#include "maze_navigation.h"
typedef enum {MAZE_FEEDBACK_NONE,MAZE_FEEDBACK_MENU,MAZE_FEEDBACK_WIN,MAZE_FEEDBACK_LOSS,MAZE_FEEDBACK_ACTION} maze_feedback_kind_t;
typedef struct {
 bool known,rgb_known;
 maze_navigation_page_t page;maze_app_state_t state;maze_feedback_kind_t kind;
 uint32_t generation,started_tick,keys,switches,checkpoint,last_rgb_tick,last_color;
} maze_feedback_t;
typedef struct {uint32_t color;unsigned border_alpha,menu_width;} maze_feedback_frame_t;
void maze_feedback_observe(maze_feedback_t*,const maze_navigation_t*,uint32_t now);
maze_feedback_frame_t maze_feedback_frame(const maze_feedback_t*,uint32_t now,uint32_t tick_hz);
bool maze_feedback_rgb(maze_feedback_t*,uint32_t now,uint32_t tick_hz,bool enabled,uint32_t *color);
#endif
