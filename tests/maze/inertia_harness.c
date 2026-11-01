/* 使用真实模型验证惯性；不连接显示、传感器或系统时钟。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_model.h"
#include "legacy_levels.h"

static const maze_level_t open_level={.width=4000,.height=4000,.ball_radius=7,
    .start={1000,1000},.goal={3900,3900,10}};

static void rolling(void)
{
    maze_model_t m;
    maze_model_reset(&m,&open_level);
    maze_model_step(&m,8192,0,10);
    assert(m.vx>0 && m.vx<120);
    for (unsigned i=0;i<99;i++) maze_model_step(&m,8192,0,10);
    assert(m.vx>150 && m.vx<=MAZE_SPEED_MAX);
    float before=m.vx, x=m.x;
    maze_model_step(&m,0,0,10);
    assert(m.vx>0 && m.vx<before && m.x>x);
    for (unsigned i=0;i<200;i++) maze_model_step(&m,0,0,10);
    assert(m.vx==0 && m.vy==0);
    puts("PASS: acceleration builds speed, neutral coasts and eventually stops");
}

static void faster_and_longer(void)
{
    maze_model_t normal, slippery;
    maze_model_reset(&normal,&open_level);
    for (unsigned i=0;i<200;i++) maze_model_step(&normal,8192,0,10);
    assert(normal.vx>170 && normal.vx<=180.001f);
    maze_surface_area_t ice={.area={0,0,4000,4000},.surface=MAZE_SURFACE_ICE};
    maze_level_t ice_level=open_level; ice_level.surfaces=&ice; ice_level.surface_count=1;
    maze_model_reset(&normal,&open_level); maze_model_reset(&slippery,&ice_level);
    normal.vx=slippery.vx=80;
    for (unsigned i=0;i<100;i++)
    {
        maze_model_step(&normal,0,0,10); maze_model_step(&slippery,0,0,10);
    }
    assert(normal.x-1000>24 && slippery.x-1000>58);
    printf("PASS: tuned speed >170; 80px/s neutral for 1s normal=%.2fpx ice=%.2fpx\n",
           normal.x-1000,slippery.x-1000);
}

static void reverse_and_turn(void)
{
    maze_model_t m;
    maze_model_reset(&m,&open_level); m.vx=100;
    maze_model_step(&m,-8192,0,10);
    assert(m.vx>0 && m.vx<100);
    for (unsigned i=0;i<50;i++) maze_model_step(&m,-8192,0,10);
    assert(m.vx<0);
    maze_model_reset(&m,&open_level); m.vx=80;
    maze_model_step(&m,0,8192,10);
    assert(m.vx>0 && m.vx<80 && m.vy>0);
    for (unsigned i=0;i<200;i++) maze_model_step(&m,65535,65535,10);
    assert(hypotf(m.vx,m.vy)<=MAZE_SPEED_MAX+0.001f);
    puts("PASS: reverse brakes before turning; lateral acceleration retains momentum; vector speed limited");
}

static void surfaces(void)
{
    maze_surface_area_t ice={.area={0,0,4000,4000},.surface=MAZE_SURFACE_ICE};
    maze_level_t ice_level=open_level; ice_level.surfaces=&ice; ice_level.surface_count=1;
    maze_model_t normal, slippery;
    maze_model_reset(&normal,&open_level); maze_model_reset(&slippery,&ice_level);
    normal.vx=slippery.vx=80;
    for (unsigned i=0;i<100;i++)
    {
        maze_model_step(&normal,0,0,10); maze_model_step(&slippery,0,0,10);
    }
    assert(slippery.x>normal.x+20 && slippery.vx>normal.vx);
    for (unsigned i=0;i<600;i++) maze_model_step(&slippery,0,0,10);
    assert(slippery.vx==0);
    const maze_level_t *l=&maze_level_surface_lab;
    assert(maze_level_surface_at(l,174.99f,50)==MAZE_SURFACE_NORMAL);
    assert(maze_level_surface_at(l,175,50)==MAZE_SURFACE_ICE);
    assert(maze_level_surface_at(l,350,50)==MAZE_SURFACE_NORMAL);
    assert(maze_level_surface_at(l,175,300)==MAZE_SURFACE_NORMAL);
    maze_model_t crossing; maze_model_reset(&crossing,l);
    crossing.x=174.5f; crossing.vx=100;
    maze_model_step(&crossing,0,0,10);
    assert(crossing.x>175 && crossing.vx>90 && crossing.vx<100);
    float old=crossing.vx;
    maze_model_step(&crossing,0,0,10);
    assert(crossing.vx>old*0.99f && crossing.vx<old);
    crossing.x=175.5f; crossing.vx=-100;
    maze_model_step(&crossing,0,0,10);
    assert(crossing.x<175 && crossing.vx<-90);
    old=crossing.vx; maze_model_step(&crossing,0,0,10);
    assert(crossing.vx<0 && crossing.vx>old);
    puts("PASS: ice coasts farther, both floors stop, half-open regions and bidirectional crossings stay continuous");
}

int main(void)
{
    setbuf(stdout,NULL);
    faster_and_longer(); rolling(); reverse_and_turn(); surfaces();
    return 0;
}
