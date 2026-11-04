/* 只计算短反馈，不操作LVGL/外设，不修改游戏或结算。 */
#include "maze_feedback.h"
static uint64_t elapsed(uint32_t now,uint32_t start,uint32_t hz){return (uint64_t)(uint32_t)(now-start)*1000/hz;}
void maze_feedback_observe(maze_feedback_t*f,const maze_navigation_t*n,uint32_t now)
{
 bool reset=!f->known||f->page!=n->page||f->generation!=n->app.generation;
 if(reset){f->kind=n->page==MAZE_NAV_GAME?MAZE_FEEDBACK_NONE:MAZE_FEEDBACK_MENU;f->started_tick=now;}
 else if(n->page==MAZE_NAV_GAME){
  if(f->state!=n->app.state){
   f->kind=n->app.state==MAZE_APP_WON?MAZE_FEEDBACK_WIN:n->app.state==MAZE_APP_LOST?MAZE_FEEDBACK_LOSS:MAZE_FEEDBACK_NONE;
   f->started_tick=now;
  }else if(n->app.state==MAZE_APP_PLAYING&&(f->keys!=n->app.model.keys||f->switches!=n->app.model.switches||f->checkpoint!=n->app.model.checkpoint)){
   f->kind=MAZE_FEEDBACK_ACTION;f->started_tick=now;
  }
 }
 f->known=true;f->page=n->page;f->generation=n->app.generation;f->state=n->app.state;
 f->keys=n->app.model.keys;f->switches=n->app.model.switches;f->checkpoint=n->app.model.checkpoint;
}
maze_feedback_frame_t maze_feedback_frame(const maze_feedback_t*f,uint32_t now,uint32_t hz)
{
 maze_feedback_frame_t v={0};uint64_t age=elapsed(now,f->started_tick,hz);
 if(f->kind==MAZE_FEEDBACK_MENU)v.menu_width=age<MAZE_FEEDBACK_MENU_MS?(unsigned)(age*350/MAZE_FEEDBACK_MENU_MS):350;
 else if(f->kind==MAZE_FEEDBACK_WIN&&age<MAZE_FEEDBACK_WIN_MS){v.color=0x40c98d;v.border_alpha=(unsigned)((MAZE_FEEDBACK_WIN_MS-age)*255/MAZE_FEEDBACK_WIN_MS);}
 else if(f->kind==MAZE_FEEDBACK_LOSS&&age<MAZE_FEEDBACK_LOSS_MS){v.color=0xef4b53;v.border_alpha=(age/120)%2?0:255;}
 else if(f->kind==MAZE_FEEDBACK_ACTION&&age<200)v.color=0x35d6c8;
 return v;
}
bool maze_feedback_rgb(maze_feedback_t*f,uint32_t now,uint32_t hz,bool enabled,uint32_t *out)
{
 bool first=!f->rgb_known;
 if(first){f->rgb_known=true;f->last_color=0;f->last_rgb_tick=now;}
 if(!enabled){
  /* 主动关灯可立即熄灭；已经为黑色时不重复设备调用。 */
  if(!f->last_color)return false;
  f->last_color=0;f->last_rgb_tick=now;*out=0;return true;
 }
 if(!first&&elapsed(now,f->last_rgb_tick,hz)<MAZE_FEEDBACK_RGB_MS)return false;
 maze_feedback_frame_t v=maze_feedback_frame(f,now,hz);uint32_t color=0;
 if(f->kind==MAZE_FEEDBACK_MENU){unsigned phase=(unsigned)(elapsed(now,f->started_tick,hz)%2000);color=(phase<1000?phase:2000-phase)*8/1000;}
 else if(v.border_alpha||f->kind==MAZE_FEEDBACK_ACTION)color=((((v.color>>16)&255)*24/255)<<16)|((((v.color>>8)&255)*24/255)<<8)|((v.color&255)*24/255);
 if(color==f->last_color)return false;
 f->last_rgb_tick=now;f->last_color=color;*out=color;return true;
}
