/* 固定布局草案的临时验证输入，不进入产品构建。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_map.h"
#include "maze_model.h"
static const char *const lines[]={
    "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+",
    "|.|.|. . . . .|. . .|. . . .|",
    "+ + + + +-+-+ + +-+ + +-+-+-+",
    "|.|. .|.|. .|. .|. .|. . . .|",
    "+ + +-+ + +-+-+-+ +-+ +-+-+ +",
    "|.|. .|.|. .|. . .|. .|. . .|",
    "+ +-+ + + + + +-+-+-+-+ +-+ +",
    "|. .|.|.|.|.|.|. . . .|. .|.|",
    "+-+ + + +-+ + +-+ +-+ +-+ +-+",
    "|. .|.|. .|. . . . ~|. .|. .|",
    "+ +-+ +-+ +-+-+-+-+ +-+ + + +",
    "|. .|. .|. . . .|. ~ .|.|.|.|",
    "+-+ +-+ +-+-+ + +-+ + + +-+ +",
    "|. .|. .|. .|.|.|~ ~|.|. . .|",
    "+ +-+-+-+ + + + + +-+-+-+-+ +",
    "|. . . . .|.|.|.|. . . .|. .|",
    "+ +-+-+-+-+ + + + +-+ +-+ +-+",
    "|~ .|.|. .|.|.|.|.|.|. . .|.|",
    "+ + + + + + + + + + +-+-+-+ +",
    "|~|. .|.|. .|.|.|. .|. . . .|",
    "+ +-+-+ +-+-+ + +-+ +-+ +-+ +",
    "|. . . .|. . .|.|.|. .|. .|.|",
    "+ +-+-+-+-+-+-+ + +-+ +-+ + +",
    "|. . . . . . . .|. . . . .|.|",
    "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+",
};
static const maze_map_object_t objects[]={{MAZE_MAP_START,{25,25,7}},{MAZE_MAP_GOAL,{675,575,14}},{MAZE_MAP_TRAP,{75,583,8}},{MAZE_MAP_TRAP,{367,425,8}},{MAZE_MAP_TRAP,{183,75,8}},{MAZE_MAP_TRAP,{375,117,8}},{MAZE_MAP_TRAP,{433,425,8}}};
static const maze_map_definition_t definition={"ice_loops_draft",14,12,lines,25,objects,7};
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
static const maze_point_t normal[]={{25,25},{25,75},{25,125},{25,175},{75,175},{75,225},{25,225},{25,275},{75,275},{75,325},{25,325},{25,375},{75,375},{125,375},{175,375},{225,375},{225,325},{275,325},{275,375},{275,425},{275,475},{225,475},{225,425},{175,425},{175,475},{175,525},{125,525},{75,525},{25,525},{25,575},{55.0,565.0},{75,565},{95.0,565.0},{125,575},{175,575},{225,575},{275,575},{325,575},{375,575},{375,525},{375,475},{385.0,445.0},{385,425},{385.0,405.0},{375,375},{375,325},{375,275},{325,275},{275,275},{225,275},{225,225},{175,225},{175,175},{175,125},{165.0,95.0},{165,75},{165.0,55.0},{175,25},{225,25},{275,25},{325,25},{325,75},{375,75},{375,25},{425,25},{475,25},{475,75},{425,75},{425,125},{395.0,135.0},{375,135},{355.0,135.0},{325,125},{325,175},{325,225},{375,225},{425,225},{425,175},{475,175},{525,175},{525,225},{575,225},{575,275},{575,325},{625,325},{675,325},{675,375},{625,375},{625,425},{575,425},{525,425},{525,375},{475,375},{425,375},{415.0,405.0},{415,425},{415.0,445.0},{425,475},{475,475},{475,525},{525,525},{525,575},{575,575},{625,575},{625,525},{575,525},{575,475},{625,475},{675,475},{675,525},{675,575}};
static const maze_point_t shortcut[]={{25,25},{25,75},{25,125},{25,175},{75,175},{75,225},{25,225},{25,275},{75,275},{75,325},{25,325},{25,375},{25,425},{25,475},{25,525},{25,575},{55.0,565.0},{75,565},{95.0,565.0},{125,575},{175,575},{225,575},{275,575},{325,575},{375,575},{375,525},{375,475},{385.0,445.0},{385,425},{385.0,405.0},{375,375},{375,325},{375,275},{325,275},{275,275},{225,275},{225,225},{175,225},{175,175},{175,125},{165.0,95.0},{165,75},{165.0,55.0},{175,25},{225,25},{275,25},{325,25},{325,75},{375,75},{375,25},{425,25},{475,25},{475,75},{425,75},{425,125},{395.0,135.0},{375,135},{355.0,135.0},{325,125},{325,175},{325,225},{375,225},{425,225},{475,225},{475,275},{475,325},{425,325},{425,375},{415.0,405.0},{415,425},{415.0,445.0},{425,475},{475,475},{475,525},{525,525},{525,575},{575,575},{625,575},{625,525},{575,525},{575,475},{625,475},{675,475},{675,525},{675,575}};
int main(void)
{
    int error=maze_map_load(&definition,&map);
    printf("draft load=%d walls=%u traps=%u ice=%u nodes=%u work=%zu\n",error,map.level.wall_count,map.level.trap_count,map.level.surface_count,map.checked_nodes,maze_map_workspace_size());
    assert(error==0 && map.level.wall_count==141 && map.level.trap_count==5 && map.level.surface_count==6);
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
