/* 固定布局草案的临时验证输入，不进入产品构建。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "maze_map.h"
#include "maze_model.h"
#include "third_fixture.h"
static maze_map_t map;
static void clear(maze_model_t *m)
{
    const maze_level_t *l=&map.level;
    assert(m->x>=7 && m->x<=l->width-7 && m->y>=7 && m->y<=l->height-7);
    for(unsigned i=0;i<l->wall_count;i++)
    {
        maze_rect_t w=l->walls[i];
        float x=fminf(fmaxf(m->x,w.x),w.x+w.width),y=fminf(fmaxf(m->y,w.y),w.y+w.height);
        assert(hypotf(m->x-x,m->y-y)>=7-0.001f);
    }
    for(unsigned i=0;i<l->trap_count;i++)
        assert(hypotf(m->x-l->traps[i].x,m->y-l->traps[i].y)>=7+l->traps[i].radius);
}
static void run(const maze_point_t *points,unsigned count,const char *name)
{
    maze_model_t m;maze_model_reset(&m,&map.level);unsigned steps=0;
    for(unsigned i=1;i<count && m.result==MAZE_MODEL_RUNNING;i++)
    {
        unsigned n;
        for(n=0;n<6000 && m.result==MAZE_MODEL_RUNNING;n++)
        {
            float dx=points[i].x-m.x,dy=points[i].y-m.y;
            if(hypotf(dx,dy)<0.3f && hypotf(m.vx,m.vy)<1) break;
            maze_model_step(&m,(20*dx-8*m.vx)*8192/600,(20*dy-8*m.vy)*8192/600,10);steps++;clear(&m);
        }
        if(n==6000) { fprintf(stderr,"stuck %s point %u pos=%f,%f target=%f,%f result=%d\n",name,i,m.x,m.y,points[i].x,points[i].y,m.result);assert(0); }
    }
    assert(m.result==MAZE_MODEL_WON);
    printf("PASS: actual inertia model %s route WON, %u model-ms\n",name,steps*10);
}
int main(void)
{
    int error=maze_map_load(&maze_map_ice_loops,&map);
    printf("draft load=%d walls=%u traps=%u ice=%u nodes=%u work=%zu\n",error,map.level.wall_count,map.level.trap_count,map.level.surface_count,map.checked_nodes,maze_map_workspace_size());
    assert(error==0 && map.level.wall_count==141 && map.level.trap_count==5 && map.level.surface_count==6);
    assert(MAZE_MAP_MAX_COLUMNS==14 && MAZE_MAP_MAX_ROWS==12);
    for(unsigned i=0;i<141;i++) assert(!memcmp(&map.level.walls[i],&third_walls[i],sizeof(maze_rect_t)));
    for(unsigned i=0;i<5;i++) assert(!memcmp(&map.level.traps[i],&third_pits[i],sizeof(maze_circle_t)));
    for(unsigned i=0;i<6;i++) assert(map.level.surfaces[i].surface==MAZE_SURFACE_ICE && !memcmp(&map.level.surfaces[i].area,&third_ice[i],sizeof(maze_rect_t)));
    assert(!strcmp(map.level.id,"ice_loops") && map.level.width==700 && map.level.height==600);
    run(normal,sizeof(normal)/sizeof(normal[0]),"ordinary");
    run(shortcut,sizeof(shortcut)/sizeof(shortcut[0]),"ice-shortcuts");
    for(unsigned i=0;i<map.level.trap_count;i++)
    {
        maze_model_t m;maze_model_reset(&m,&map.level);
        maze_circle_t pit=map.level.traps[i];
        /* 候选陷阱均在直通格：偏置y代表水平通道，偏置x代表竖直通道。 */
        int horizontal=fmodf(pit.y,50)!=25;
        m.x=pit.x-(horizontal?20:0);m.y=pit.y-(horizontal?0:20);
        clear(&m);
        for(unsigned step=0;step<100 && m.result==MAZE_MODEL_RUNNING;step++)
            maze_model_step(&m,horizontal?8192:0,horizontal?0:8192,10);
        assert(m.result==MAZE_MODEL_LOST && m.vx==0 && m.vy==0);
        float x=m.x,y=m.y;maze_model_step(&m,8192,8192,1000);
        assert(m.x==x && m.y==y);
    }
    puts("PASS: all five pits approached by real model, LOST then frozen");
}
