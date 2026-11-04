#include "maze_feedback.h"
#include <assert.h>
#include <stdio.h>
static maze_navigation_t n;static maze_feedback_t f;
int main(void){uint32_t color;n.tick_hz=1000;n.page=MAZE_NAV_SELECT;maze_feedback_observe(&f,&n,0);
 assert(maze_feedback_rgb(&f,200,1000,true,&color)&&color);
 n.page=MAZE_NAV_GAME;n.app.generation=1;n.app.state=MAZE_APP_READY;maze_feedback_observe(&f,&n,210);
 assert(!maze_feedback_rgb(&f,210,1000,true,&color)); /* 切页也不得10ms内再次写。 */
 assert(maze_feedback_rgb(&f,250,1000,true,&color)&&color==0);
 n.app.state=MAZE_APP_CALIBRATING;maze_feedback_observe(&f,&n,260);assert(!maze_feedback_rgb(&f,260,1000,true,&color));assert(!maze_feedback_rgb(&f,300,1000,true,&color));
 n.app.state=MAZE_APP_PAUSED;maze_feedback_observe(&f,&n,310);assert(!maze_feedback_rgb(&f,360,1000,true,&color));
 n.page=MAZE_NAV_SELECT;maze_feedback_observe(&f,&n,400);assert(maze_feedback_rgb(&f,600,1000,true,&color)&&color);
 assert(maze_feedback_rgb(&f,610,1000,false,&color)&&color==0);assert(!maze_feedback_rgb(&f,611,1000,false,&color));
 assert(!maze_feedback_rgb(&f,620,1000,true,&color));assert(maze_feedback_rgb(&f,660,1000,true,&color)&&color);
 puts("PASS: cross-page/state RGB writes obey50ms, identical black not repeated; explicit off immediately clears once");}
