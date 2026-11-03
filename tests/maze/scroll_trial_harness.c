/* 试验地图实际装载与惯性路线，不移动途中坐标。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_map.h"
#include "maze_model.h"
extern const maze_map_definition_t maze_map_scroll_trial;
static maze_map_t map;
int main(void)
{
    int result=maze_map_load(&maze_map_scroll_trial,&map);
    fprintf(stderr,"trial load result=%d walls=%u nodes=%u\n",result,map.level.wall_count,map.checked_nodes);
    assert(result==0);
    assert(map.level.width==700 && map.level.height==600);
    assert(map.level.trap_count==3 && map.level.surface_count>0);
    assert(maze_map_workspace_size()>=40520 && maze_map_workspace_size()<42*1024);
    maze_map_definition_t bad=maze_map_scroll_trial;bad.columns=15;
    static maze_map_t invalid;assert(maze_map_load(&bad,&invalid)==MAZE_MAP_FORMAT);
    const maze_point_t route[]={{25,125},{675,125},{675,325},{25,325},{25,525},{675,525},{675,575}};
    maze_model_t m;maze_model_reset(&m,&map.level);unsigned steps=0;int ice=0;
    for(unsigned i=0;i<sizeof(route)/sizeof(route[0]) && m.result==MAZE_MODEL_RUNNING;i++)
    {
        unsigned n;
        for(n=0;n<6000 && m.result==MAZE_MODEL_RUNNING;n++)
        {
            float dx=route[i].x-m.x,dy=route[i].y-m.y;
            if(hypotf(dx,dy)<0.3f && hypotf(m.vx,m.vy)<1) break;
            maze_model_step(&m,(20*dx-8*m.vx)*8192/600,(20*dy-8*m.vy)*8192/600,10);steps++;
            ice|=maze_level_surface_at(&map.level,m.x,m.y)==MAZE_SURFACE_ICE;
            for(unsigned j=0;j<map.level.trap_count;j++)
                assert(hypotf(m.x-map.level.traps[j].x,m.y-map.level.traps[j].y)>=map.level.traps[j].radius+map.level.ball_radius);
        }
        assert(n<6000);
    }
    assert(m.result==MAZE_MODEL_WON && ice);
    printf("PASS: 700x600 trial, workspace=%zu, nodes=%u, inertia route=%u model-ms\n",maze_map_workspace_size(),map.checked_nodes,steps*10);
    for(unsigned i=0;i<3;i++)
    {
        maze_model_reset(&m,&map.level);m.x=map.level.traps[i].x-25;m.y=map.level.traps[i].y;
        for(unsigned j=0;j<100 && m.result==MAZE_MODEL_RUNNING;j++) maze_model_step(&m,8192,0,10);
        assert(m.result==MAZE_MODEL_LOST && m.vx==0 && m.vy==0);
    }
    puts("PASS: three trial traps lose and freeze");
}
