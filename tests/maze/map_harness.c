/* 用真实关卡与模型验证必经陷阱地图，不依赖显示或外设。 */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "maze_model.h"

static const maze_level_t *const l=&maze_level_frost_fork;

static const maze_point_t normal_points[]={{25,25},{75,25},{65,60},{65,90},{75,125},{25,125},{25,175},{75,175},{75,225},{75,275},{125,275},{175,275},{225,275},{275,275},{275,225},{275,175},{225,175},{175,175},{125,175},{125,125},{125,75},{125,25},{175,25},{165,60},{165,90},{175,125},{225,125},{225,75},{275,75},{325,75},{325,125},{335,160},{335,190},{325,225},{325,275}};
static const maze_point_t ice_points[]={{25,25},{75,25},{65,60},{65,90},{75,125},{25,125},{25,175},{75,175},{75,225},{75,275},{125,275},{175,275},{175,225},{125,225},{125,175},{125,125},{125,75},{125,25},{175,25},{165,60},{165,90},{175,125},{225,125},{225,75},{275,75},{325,75},{325,125},{335,160},{335,190},{325,225},{325,275}};

static bool wall_clear(float x, float y)
{
    float r=l->ball_radius;
    if (x<r || x>l->width-r || y<r || y>l->height-r) return false;
    for (unsigned i=0;i<l->wall_count;i++)
    {
        maze_rect_t w=l->walls[i];
        float cx=fminf(fmaxf(x,w.x),w.x+w.width),cy=fminf(fmaxf(y,w.y),w.y+w.height);
        if (hypotf(x-cx,y-cy)<r-0.001f) return false;
    }
    return true;
}

static void pit_clear(float x, float y)
{
    for (unsigned i=0;i<l->trap_count;i++)
    {
        maze_circle_t t=l->traps[i];
        /* 比球心落坑规则更保守：参考轨迹的完整球体也要绕过坑。 */
        assert(hypotf(x-t.x,y-t.y)>=t.radius+l->ball_radius);
    }
}

static void topology(void)
{
    assert(l->wall_count==29 && l->trap_count==3 && l->surface_count==2);
    bool graph[42][42]={{false}};
    for (unsigned a=0;a<42;a++)
        for (unsigned b=a+1;b<42;b++)
        {
            if (!((a/7==b/7 && b==a+1) || b==a+7)) continue;
            float ax=(a%7)*50+25,ay=(a/7)*50+25,bx=(b%7)*50+25,by=(b/7)*50+25;
            bool open=true;
            for (unsigned k=0;k<=50;k++)
                if (!wall_clear(ax+(bx-ax)*k/50,ay+(by-ay)*k/50)) open=false;
            graph[a][b]=graph[b][a]=open;
        }
    unsigned edges=0,junctions=0,dead=0;
    for (unsigned a=0;a<42;a++)
    {
        unsigned degree=0;
        for (unsigned b=0;b<42;b++) if (graph[a][b]) degree++;
        edges+=degree;
        if (degree>=3) junctions++;
        if (degree==1 && a!=0 && a!=41) dead++;
        if (a==41) assert(degree==1);
    }
    assert(edges/2==42 && junctions==7 && dead==5);
    for (unsigned trial=0;trial<=3;trial++)
    {
        int blocked=-1;
        if (trial<3)
        {
            maze_circle_t t=l->traps[trial];
            blocked=(int)(t.y/50)*7+(int)(t.x/50);
            assert(blocked==8 || blocked==10 || blocked==27);
        }
        unsigned queue[42],head=0,tail=1; bool seen[42]={false};
        queue[0]=0;seen[0]=true;
        while (head<tail)
        {
            unsigned a=queue[head++];
            for (unsigned b=0;b<42;b++)
                if (graph[a][b] && !seen[b] && (int)b!=blocked)
                { seen[b]=true;queue[tail++]=b; }
        }
        if (trial<3) assert(!seen[41]);
        else assert(tail==42 && seen[41]);
    }
    puts("PASS: actual C map has 7 forks/5 dead ends, all connected; each trap corridor is mandatory");
}

static void route(const maze_point_t *points, unsigned count, bool ice)
{
    for (unsigned i=1;i<count;i++)
    {
        maze_point_t a=points[i-1],b=points[i];
        unsigned samples=(unsigned)ceilf(hypotf(b.x-a.x,b.y-a.y));
        for (unsigned n=0;n<=samples;n++)
        {
            float x=a.x+(b.x-a.x)*n/samples,y=a.y+(b.y-a.y)*n/samples;
            assert(wall_clear(x,y));pit_clear(x,y);
        }
    }
    maze_model_t m;maze_model_reset(&m,l);
    bool touched_ice=false;unsigned steps=0;
    for (unsigned i=1;i<count;i++)
    {
        unsigned n;
        for (n=0;n<5000 && m.result==MAZE_MODEL_RUNNING;n++)
        {
            float dx=points[i].x-m.x,dy=points[i].y-m.y;
            if (hypotf(dx,dy)<0.3f && hypotf(m.vx,m.vy)<1) break;
            /* 仅提供倾斜输入；不传送小球、不重置途中速度。 */
            maze_model_step(&m,(20*dx-8*m.vx)*8192/600,(20*dy-8*m.vy)*8192/600,10);
            steps++;assert(wall_clear(m.x,m.y));pit_clear(m.x,m.y);
            if (maze_level_surface_at(l,m.x,m.y)==MAZE_SURFACE_ICE) touched_ice=true;
        }
        assert(n<5000);
        if (i+1<count) assert(m.result==MAZE_MODEL_RUNNING);
    }
    assert(m.result==MAZE_MODEL_WON && touched_ice==ice);
    printf("PASS: %s route won through actual inertia in %u model-ms, full-ball pit clearance retained\n",
           ice?"ice shortcut":"normal",steps*10);
}

static void hazards(void)
{
    for (unsigned i=0;i<l->trap_count;i++)
    {
        maze_model_t m;maze_model_reset(&m,l);
        m.x=l->traps[i].x;m.y=l->traps[i].y-20;
        assert(wall_clear(m.x,m.y));
        for (unsigned n=0;n<100 && m.result==MAZE_MODEL_RUNNING;n++) maze_model_step(&m,0,8192,10);
        assert(m.result==MAZE_MODEL_LOST && m.vx==0 && m.vy==0);
        float x=m.x,y=m.y;maze_model_step(&m,8192,8192,1000);
        assert(m.x==x && m.y==y && m.result==MAZE_MODEL_LOST);
    }
    puts("PASS: all three mandatory-corridor pits trigger loss and terminal freeze");
}

int main(void)
{
    setbuf(stdout,NULL);
    topology();
    route(normal_points,sizeof(normal_points)/sizeof(normal_points[0]),false);
    route(ice_points,sizeof(ice_points)/sizeof(ice_points[0]),true);
    hazards();
    return 0;
}
