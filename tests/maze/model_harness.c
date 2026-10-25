/* 真实纯C运动模型测试，几何输入不依赖显示或传感器桩。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_model.h"

static void near(float actual, float expected)
{
    if (fabsf(actual-expected)>=0.002f)
        fprintf(stderr,"coordinate mismatch actual=%.9g expected=%.9g\n",actual,expected);
    assert(fabsf(actual-expected)<0.002f);
}

static maze_level_t empty(void)
{
    return (maze_level_t){.width=4000,.height=4000,.ball_radius=5,
        .start={10,50},.goal={3900,3900,3}};
}

static void movement(void)
{
    maze_level_t l=empty();
    maze_model_t m;
    maze_model_reset(&m,&l);
    near(m.vx,0); near(m.vy,0);
    maze_model_step(&m,8192,0,1000);
    near(m.x,130); near(m.y,50); near(m.step_elapsed_ms,1000);
    maze_model_reset(&m,&l);
    maze_model_step(&m,4096,0,1000); near(m.x,70);
    maze_model_reset(&m,&l);
    maze_model_step(&m,65535,0,1000); near(m.x,130);
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,8192,1000);
    near(hypotf(m.vx,m.vy),120);
    near(hypotf(m.x-10,m.y-50),120);
    maze_model_reset(&m,&l);
    maze_model_step(&m,0,0,1000); near(m.x,10); near(m.y,50);
    puts("PASS: proportional speed, saturation, diagonal limit and neutral");
}

static void walls(void)
{
    maze_rect_t wall={50,20,1,150};
    maze_level_t l=empty(); l.walls=&wall; l.wall_count=1;
    maze_model_t m;
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,0,1000);
    near(m.x,45); near(m.vx,0); assert(m.result==MAZE_MODEL_RUNNING);
    maze_model_step(&m,8192,4096,500);
    near(m.x,45); assert(m.y>70 && m.vy>0);
    maze_model_step(&m,-8192,0,100); near(m.x,33);
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,8192,1000);
    near(m.x,45); near(m.y,50+120/sqrtf(2));
    /* 两个法线方向均被阻挡，不沿内角穿出。 */
    maze_rect_t corner[]={{50,0,1,100},{0,70,100,1}};
    l.walls=corner; l.wall_count=2;
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,8192,1000);
    near(m.x,45); near(m.y,65);
    l=empty(); l.width=100; l.height=100;
    maze_model_reset(&m,&l);
    maze_model_step(&m,-8192,-8192,10000);
    near(m.x,5); near(m.y,5);
    maze_model_step(&m,8192,8192,10000);
    near(m.x,95); near(m.y,95);
    puts("PASS: thin wall, large sweep, contact sliding, inside corner and all borders");
}

static void rounded_corner(void)
{
    maze_rect_t wall={50,50,10,10};
    maze_level_t l=empty(); l.walls=&wall; l.wall_count=1; l.start=(maze_point_t){40,46};
    maze_model_t m;
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,0,55);
    /* (46.6,46)在方形膨胀内、圆角外，必须可达。 */
    near(m.x,46.6f); near(m.y,46);
    l.start=(maze_point_t){40,45};
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,0,250); near(m.x,70); near(m.y,45);
    l.start=(maze_point_t){40,46};
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,0,100);
    assert(m.y<46 && m.x>47);
    assert(hypotf(m.x-50,m.y-50)>=5-0.001f);
    puts("PASS: rounded expansion, exact tangent and oblique corner sliding");
}

static void regions(void)
{
    maze_circle_t trap={30,50,3};
    maze_level_t l=empty(); l.traps=&trap; l.trap_count=1; l.goal=(maze_circle_t){60,50,3};
    maze_model_t m;
    maze_model_reset(&m,&l);
    maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_LOST); near(m.x,27);
    near(m.step_elapsed_ms,1000.0f*17/120); near(m.vx,0);
    maze_model_step(&m,-8192,8192,1000);
    near(m.x,27); near(m.y,50); near(m.step_elapsed_ms,0);
    l.goal=(maze_circle_t){20,50,3};
    maze_model_reset(&m,&l); maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_WON); near(m.x,17);
    maze_model_step(&m,8192,0,1000); near(m.x,17);
    l.goal=(maze_circle_t){60,50,3}; l.start=(maze_point_t){27,50};
    maze_model_reset(&m,&l); maze_model_step(&m,0,0,0);
    assert(m.result==MAZE_MODEL_LOST); near(m.step_elapsed_ms,0);
    l.start=(maze_point_t){10,53};
    maze_model_reset(&m,&l); maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_LOST); near(m.x,30);
    l.start=(maze_point_t){10,53.01f};
    maze_model_reset(&m,&l); maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_RUNNING);
    /* 墙后的区域不得隔墙结算；滑动后的实际路径仍需检测。 */
    maze_rect_t wall={20,0,1,100}; l.walls=&wall; l.wall_count=1; l.start=(maze_point_t){10,50};
    maze_model_reset(&m,&l); maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_RUNNING); near(m.x,15);
    trap=(maze_circle_t){15,80,3};
    maze_model_reset(&m,&l); maze_model_step(&m,8192,8192,1000);
    assert(m.result==MAZE_MODEL_LOST); near(m.x,15); near(m.y,77);
    puts("PASS: earliest region, inclusive boundary, tangent, wall occlusion and terminal freeze");
}

static void clear_point(const maze_level_t *l, float x, float y)
{
    assert(x>=l->ball_radius && x<=l->width-l->ball_radius);
    assert(y>=l->ball_radius && y<=l->height-l->ball_radius);
    for (unsigned i=0;i<l->wall_count;i++)
    {
        maze_rect_t w=l->walls[i];
        float cx=fmaxf(w.x,fminf(x,w.x+w.width));
        float cy=fmaxf(w.y,fminf(y,w.y+w.height));
        assert(hypotf(x-cx,y-cy)>=l->ball_radius-0.001f);
    }
}

static void map_path(void)
{
    const maze_level_t *l=&maze_level_one;
    clear_point(l,l->start.x,l->start.y);
    for (unsigned i=0;i<l->wall_count;i++)
    {
        maze_rect_t w=l->walls[i];
        assert(w.width>0 && w.height>0 && w.x>=0 && w.y>=0);
        assert(w.x+w.width<=l->width && w.y+w.height<=l->height);
    }
    for (unsigned i=0;i<=l->trap_count;i++)
    {
        maze_circle_t c=i==l->trap_count?l->goal:l->traps[i];
        assert(c.radius>0 && c.x-c.radius>=0 && c.y-c.radius>=0);
        assert(c.x+c.radius<=l->width && c.y+c.radius<=l->height);
        assert(hypotf(c.x-l->start.x,c.y-l->start.y)>c.radius+l->ball_radius);
        for (unsigned j=i+1;j<=l->trap_count;j++)
        {
            maze_circle_t d=j==l->trap_count?l->goal:l->traps[j];
            assert(hypotf(c.x-d.x,c.y-d.y)>c.radius+d.radius);
        }
    }
    maze_point_t path[]={{24,24},{24,246},{145,246},{145,42},{270,42},{270,276},{326,276}};
    maze_model_t m; maze_model_reset(&m,l);
    for (unsigned i=1;i<sizeof(path)/sizeof(path[0]);i++)
    {
        float dx=path[i].x-path[i-1].x, dy=path[i].y-path[i-1].y;
        unsigned steps=(unsigned)lroundf(hypotf(dx,dy)*10);
        for (unsigned n=0;n<steps && m.result==MAZE_MODEL_RUNNING;n++)
        {
            maze_model_step(&m,dx==0?0:(dx>0?1:-1)*8192.0f*5/6,
                            dy==0?0:(dy>0?1:-1)*8192.0f*5/6,1);
            clear_point(l,m.x,m.y);
        }
        if (i<6) assert(m.result==MAZE_MODEL_RUNNING);
    }
    assert(m.result==MAZE_MODEL_WON);
    puts("PASS: one-level geometry, disjoint regions and ball-radius-safe route witness");
}

static void repeated_contacts(void)
{
    maze_rect_t wall={50,50,20,20};
    maze_level_t l=empty(); l.walls=&wall; l.wall_count=1;
    /* 从周围64个方向持续压向墙，逐步核对，不只检查最终停点。 */
    for (unsigned k=0;k<64;k++)
    {
        float angle=k*6.283185307f/64;
        l.start=(maze_point_t){60+38*cosf(angle),60+38*sinf(angle)};
        maze_model_t m; maze_model_reset(&m,&l);
        for (unsigned n=0;n<150;n++)
        {
            maze_model_step(&m,-8192*cosf(angle),-8192*sinf(angle),10);
            clear_point(&l,m.x,m.y);
        }
    }
    puts("PASS: 64 approach directions with 150 repeated contact steps each");
}

static void render_independence(void)
{
    maze_level_t l=empty(); maze_model_t a,b;
    maze_model_reset(&a,&l); maze_model_reset(&b,&l);
    /* 验证模型依据dt而非调用次数；固定步及真正显示对照留到app/UI接入。 */
    for (unsigned ms=0;ms<10000;ms+=50) maze_model_step(&a,8192,4096,50);
    for (unsigned ms=0;ms<10000;ms+=25) maze_model_step(&b,8192,4096,25);
    assert(fabsf(a.x-b.x)<0.1f && fabsf(a.y-b.y)<0.1f);
    assert(fabsf(a.x-(10+1200*2/sqrtf(5)))<0.1f);
    assert(fabsf(a.y-(50+1200/sqrtf(5)))<0.1f);
    puts("PASS: 10s elapsed-time motion at 20/40 caller Hz (not display acceptance)");
}

int main(void)
{
    setbuf(stdout,NULL);
    movement(); walls(); rounded_corner(); regions(); map_path(); repeated_contacts(); render_independence();
    return 0;
}
