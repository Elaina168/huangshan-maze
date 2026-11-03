/* 真实产品地图与惯性模型，按目标轨迹等待机关后通关。 */
#include "maze_model.h"
#include "maze_map.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
static maze_map_t map;
static float control(float error,float speed){return fmaxf(-8192,fminf(8192,(error*10-speed*7)*8192/600));}
int main(void){
 assert(!maze_map_load(&maze_map_dynamic_trial,&map));
 maze_model_t m;maze_model_reset(&m,&map.level);
 maze_point_t path[]={{180,50},{200,50},{280,50},{550,50},{550,200},{440,200},{350,200},
 {150,200},{150,350},{250,350},{440,370},{500,370},{500,400},{500,450},{525,475}};
 unsigned point=0,steps=0;bool crossing=false;
 while(m.result==MAZE_MODEL_RUNNING && steps++<120000){
    if((point==2||point==13)&&!crossing){
      unsigned phase=(unsigned)((m.dynamic_us/1000+(point==13?1000:0))%3600);
      if(phase<150 && hypotf(m.vx,m.vy)<5) crossing=true;
      else {maze_point_t wait=path[point-1];maze_model_step(&m,control(wait.x-m.x,m.vx),control(wait.y-m.y,m.vy),10);continue;}
    }
    maze_point_t p=path[point];
    maze_model_step(&m,control(p.x-m.x,m.vx),control(p.y-m.y,m.vy),10);
    if(hypotf(m.x-p.x,m.y-p.y)<(point<2?25.f:1.5f) && hypotf(m.vx,m.vy)<5 && point+1<sizeof(path)/sizeof(path[0])){point++;crossing=false;}
 }
 fprintf(stderr,"route result=%d reason=%d point=%u x=%.2f y=%.2f ms=%llu checkpoint=%u steps=%u\n",m.result,m.loss_reason,point,m.x,m.y,(unsigned long long)(m.dynamic_us/1000),m.checkpoint,steps);
 assert(m.result==MAZE_MODEL_WON && m.checkpoint);
 puts("PASS: actual fifth-map waiting/gate/magnet/patrol corridor route WON");
 maze_dynamic_t invalid=maze_map_dynamic_trial.dynamics[0];maze_map_definition_t d=maze_map_dynamic_trial;
 d.dynamics=&invalid;d.dynamic_count=1;invalid.outer_radius=0;
 assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
 d=maze_map_dynamic_trial;invalid=d.dynamics[3];invalid.end.x=500;invalid.end.y=450;d.dynamics=&invalid;d.dynamic_count=1;
 assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
 puts("PASS: invalid magnet and non-axis patrol rejected");
 char rows[21][26];const char *lines[21];
 for(unsigned i=0;i<21;i++){strcpy(rows[i],maze_map_dynamic_trial.lines[i]);lines[i]=rows[i];}
 rows[7][16]='|';rows[9][16]='|';d=maze_map_dynamic_trial;d.lines=lines;d.dynamic_count=0;
 assert(maze_map_load(&d,&map)==MAZE_MAP_NO_ROUTE);
 puts("PASS: closing timed-gate corridor disconnects goal; no branch bypass");
 return 0;
}
