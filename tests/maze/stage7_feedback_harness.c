#include "maze_feedback.h"
#include <assert.h>
#include <stdio.h>
static maze_navigation_t n;static maze_feedback_t f;
int main(void){n.tick_hz=1000;n.page=MAZE_NAV_SELECT;maze_feedback_observe(&f,&n,0);maze_feedback_frame_t v=maze_feedback_frame(&f,75,1000);assert(v.menu_width==175);
 n.page=MAZE_NAV_GAME;n.app.state=MAZE_APP_PLAYING;n.app.generation=1;maze_feedback_observe(&f,&n,100);n.app.state=MAZE_APP_WON;maze_feedback_observe(&f,&n,200);v=maze_feedback_frame(&f,200,1000);assert(v.border_alpha==255&&v.color==0x40c98d);
 n.page_version++;maze_feedback_observe(&f,&n,300);assert(maze_feedback_frame(&f,600,1000).border_alpha==0);
 n.app.generation++;n.app.state=MAZE_APP_PLAYING;maze_feedback_observe(&f,&n,700);n.app.state=MAZE_APP_LOST;maze_feedback_observe(&f,&n,800);assert(maze_feedback_frame(&f,800,1000).border_alpha==255&&maze_feedback_frame(&f,920,1000).border_alpha==0&&maze_feedback_frame(&f,1040,1000).border_alpha==255&&maze_feedback_frame(&f,1280,1000).border_alpha==0);
 n.page=MAZE_NAV_SELECT;maze_feedback_observe(&f,&n,850);assert(!maze_feedback_frame(&f,860,1000).border_alpha);
 n.page=MAZE_NAV_GAME;n.app.state=MAZE_APP_PLAYING;n.app.generation++;maze_feedback_observe(&f,&n,0xfffffff0u);n.app.state=MAZE_APP_WON;maze_feedback_observe(&f,&n,0xfffffff8u);assert(maze_feedback_frame(&f,20,1000).border_alpha>0&&maze_feedback_frame(&f,400,1000).border_alpha==0);
 uint32_t color;assert(!maze_feedback_rgb(&f,500,1000,false,&color));assert(!maze_feedback_rgb(&f,510,1000,true,&color));assert(!maze_feedback_rgb(&f,520,1000,true,&color));assert(!maze_feedback_rgb(&f,521,1000,false,&color));assert(!maze_feedback_rgb(&f,522,1000,false,&color));
 n.app.generation++;n.app.state=MAZE_APP_PLAYING;maze_feedback_observe(&f,&n,600);n.app.model.checkpoint=1;maze_feedback_observe(&f,&n,610);assert(maze_feedback_rgb(&f,620,1000,true,&color)&&color);assert(!maze_feedback_rgb(&f,630,1000,true,&color));assert(maze_feedback_rgb(&f,631,1000,false,&color)&&color==0);
 n.app.generation++;maze_feedback_observe(&f,&n,640);assert(!maze_feedback_frame(&f,650,1000).border_alpha);
 puts("PASS: bounded win/loss/menu/action, same-page save does not replay, page/generation cancel, tick wrap, RGB50ms throttle and immediate off");}
