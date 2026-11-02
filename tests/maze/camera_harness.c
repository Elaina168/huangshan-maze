/* 镜头只控制显示，不改变世界运动。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_camera.h"
#include "maze_model.h"

int main(void)
{
    maze_camera_t c;
    maze_camera_reset(&c,700,600,175,150,0);
    assert(c.x==0 && c.y==0);
    maze_camera_update(&c,220,180,50,1000,true);
    assert(c.x==0 && c.y==0);
    maze_camera_update(&c,400,350,100,1000,true);
    assert(c.x>0 && c.y>0 && c.x<350 && c.y<300);
    float x=c.x,y=c.y;
    maze_camera_update(&c,600,500,100000,1000,false);
    assert(c.x==x && c.y==y);
    maze_camera_update(&c,600,500,100050,1000,true);
    maze_camera_t resumed=c;
    resumed.x=x;resumed.y=y;resumed.last_tick=100000;
    maze_camera_update(&resumed,600,500,100050,1000,true);
    assert(c.x==resumed.x && c.y==resumed.y);
    maze_camera_reset(&c,700,600,693,593,0);
    assert(c.x==350 && c.y==300);
    maze_camera_reset(&c,700,600,7,7,0);
    assert(c.x==0 && c.y==0);
    maze_camera_reset(&c,200,100,150,70,0);
    maze_camera_update(&c,150,70,100,1000,true);
    assert(c.x==0 && c.y==0);
    maze_camera_t a,b;
    maze_camera_reset(&a,700,600,175,150,0);b=a;
    for(unsigned t=50;t<=1000;t+=50) maze_camera_update(&a,500,450,t,1000,true);
    for(unsigned t=25;t<=1000;t+=25) maze_camera_update(&b,500,450,t,1000,true);
    assert(fabsf(a.x-b.x)<0.001f && fabsf(a.y-b.y)<0.001f);
    maze_camera_reset(&c,700,600,25,25,UINT32_MAX-20);
    for(unsigned i=0;i<4000;i++)
    {
        float px=7+fabsf(686-fmodf(i*9.0f,1372));
        float py=7+fabsf(586-fmodf(i*7.0f,1172));
        maze_camera_update(&c,px,py,(uint32_t)(UINT32_MAX-20+i*50),1000,true);
        assert(c.x>=0 && c.x<=350 && c.y>=0 && c.y<=300);
        assert(px-c.x>=7-0.001f && px-c.x<=343+0.001f);
        assert(py-c.y>=7-0.001f && py-c.y<=293+0.001f);
    }
    /* 同一模型输入下，镜头调用不能改变任何物理状态。 */
    const maze_level_t level={.width=700,.height=600,.ball_radius=7,.start={25,25},.goal={675,575,14}};
    maze_model_t m,n;maze_model_reset(&m,&level);n=m;
    maze_camera_reset(&c,700,600,25,25,0);
    for(unsigned t=10;t<5000;t+=10)
    {
        maze_model_step(&m,3000,2000,10);maze_model_step(&n,3000,2000,10);
        maze_camera_update(&c,m.x,m.y,t,1000,true);
        assert(m.x==n.x && m.y==n.y && m.result==n.result && m.vx==n.vx && m.vy==n.vy);
    }
    puts("PASS: camera dead zone, clamp, pause, reset, wrap, cadence and physics isolation");
}
