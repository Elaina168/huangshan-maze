/* 实际惯性模型沿静态见证输入，不传送或重置途中速度。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "maze_map.h"
#include "maze_model.h"
#include "second_fixture.h"
extern const maze_map_definition_t maze_map_second;
static maze_map_t map;

static void clear(float x,float y,bool pits)
{
    const maze_level_t *l=&map.level;
    assert(x>=l->ball_radius && x<=l->width-l->ball_radius);
    assert(y>=l->ball_radius && y<=l->height-l->ball_radius);
    for(unsigned i=0;i<l->wall_count;i++)
    {
        maze_rect_t w=l->walls[i];
        float cx=fminf(fmaxf(x,w.x),w.x+w.width),cy=fminf(fmaxf(y,w.y),w.y+w.height);
        assert(hypotf(x-cx,y-cy)>=l->ball_radius-0.001f);
    }
    if(pits) for(unsigned i=0;i<l->trap_count;i++)
        assert(hypotf(x-l->traps[i].x,y-l->traps[i].y)>=l->traps[i].radius+l->ball_radius);
}

int main(void)
{
    setbuf(stdout,NULL);
    assert(maze_map_load(&maze_map_second,&map)==0);
    assert(map.level.wall_count==30 && map.level.trap_count==4 && map.level.surface_count==4);
    for(unsigned i=0;i<30;i++)
    {
        maze_rect_t a=map.level.walls[i],b=expected_walls[i];
        assert(a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height);
    }
    maze_model_t m; maze_model_reset(&m,&map.level);
    unsigned steps=0; bool ice=false;
    for(unsigned i=1;i<sizeof(points)/sizeof(points[0]) && m.result==MAZE_MODEL_RUNNING;i++)
    {
        unsigned n;
        for(n=0;n<5000 && m.result==MAZE_MODEL_RUNNING;n++)
        {
            float dx=points[i].x-m.x,dy=points[i].y-m.y;
            if(hypotf(dx,dy)<0.3f && hypotf(m.vx,m.vy)<1) break;
            maze_model_step(&m,(20*dx-8*m.vx)*8192/600,(20*dy-8*m.vy)*8192/600,10);
            steps++; clear(m.x,m.y,true);
            if(maze_level_surface_at(&map.level,m.x,m.y)==MAZE_SURFACE_ICE) ice=true;
        }
        assert(n<5000);
    }
    assert(m.result==MAZE_MODEL_WON && ice);
    printf("PASS: second map exact 30 walls, 4 ice cells; inertia route won in %u model-ms with full-ball clearance\n",steps*10);
    for(unsigned i=0;i<4;i++)
    {
        maze_model_reset(&m,&map.level);
        m.x=pit_approaches[i].x; m.y=pit_approaches[i].y;
        clear(m.x,m.y,false);
        float dx=map.level.traps[i].x-m.x,dy=map.level.traps[i].y-m.y;
        for(unsigned j=0;j<100 && m.result==MAZE_MODEL_RUNNING;j++) maze_model_step(&m,dx*8192,dy*8192,10);
        assert(m.result==MAZE_MODEL_LOST && m.vx==0 && m.vy==0);
        float x=m.x,y=m.y; maze_model_step(&m,8192,8192,1000);
        assert(m.x==x && m.y==y);
    }
    puts("PASS: all four second-map pits lose and freeze");
    return 0;
}
