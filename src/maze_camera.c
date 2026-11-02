/* 世界坐标独立于显示镜头；只由UI线程使用。 */
#include "maze_camera.h"
#include "maze_config.h"
#include <math.h>

static float clamp(float x,float low,float high) { return fminf(fmaxf(x,low),high); }

void maze_camera_reset(maze_camera_t *c,float width,float height,float x,float y,uint32_t tick)
{
    *c=(maze_camera_t){.world_width=width,.world_height=height,.last_tick=tick};
    c->x=clamp(x-MAZE_VIEW_WIDTH/2,0,fmaxf(0,width-MAZE_VIEW_WIDTH));
    c->y=clamp(y-MAZE_VIEW_HEIGHT/2,0,fmaxf(0,height-MAZE_VIEW_HEIGHT));
}

static float follow(float offset,float ball,float extent,float view,float dead,float alpha)
{
    float low=(view-dead)/2,high=(view+dead)/2;
    float target=offset+ball-offset-clamp(ball-offset,low,high);
    offset+=(target-offset)*alpha;
    /* 急折返或大步位置更新时仍保留完整球的可见余量。 */
    offset=clamp(offset,ball-view+MAZE_CAMERA_MARGIN,ball-MAZE_CAMERA_MARGIN);
    return clamp(offset,0,fmaxf(0,extent-view));
}

void maze_camera_update(maze_camera_t *c,float x,float y,uint32_t tick,uint32_t hz,bool playing)
{
    uint32_t elapsed=tick-c->last_tick;c->last_tick=tick;
    if(!playing) return;
    float alpha=-expm1f(-MAZE_CAMERA_FOLLOW*(float)elapsed/hz);
    c->x=follow(c->x,x,c->world_width,MAZE_VIEW_WIDTH,MAZE_CAMERA_DEAD_WIDTH,alpha);
    c->y=follow(c->y,y,c->world_height,MAZE_VIEW_HEIGHT,MAZE_CAMERA_DEAD_HEIGHT,alpha);
}
