/* 模板/整图以真实惯性输入通关，不传送或重置途中速度。 */
#include "maze_random_templates.h"
#include "maze_model.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static maze_generated_t generated;static maze_generator_t generator;static maze_map_t map;
static float control(float e,float v){return fmaxf(-8192,fminf(8192,(e*20-v*9)*8192/600));}
static void move(maze_model_t*m,maze_point_t p){for(unsigned i=0;i<3000&&m->result==MAZE_MODEL_RUNNING;i++){
 if(hypotf(m->x-p.x,m->y-p.y)<1.5f&&hypotf(m->vx,m->vy)<5)return;
 maze_model_step(m,control(p.x-m->x,m->vx),control(p.y-m->y,m->vy),10);
 }
 if(m->result!=MAZE_MODEL_WON && (m->result!=MAZE_MODEL_RUNNING||hypotf(m->x-p.x,m->y-p.y)>=1.5f)){
 fprintf(stderr,"move failed result=%d reason=%d pos=%.2f,%.2f target=%.2f,%.2f t=%llu\n",m->result,m->loss_reason,m->x,m->y,p.x,p.y,(unsigned long long)m->dynamic_us);assert(0);}}
static maze_point_t point(unsigned node,unsigned r,float x,float y){maze_point_t p=maze_random_point((maze_point_t){x,y},r);p.x+=(node%6)*100;p.y+=(node/6)*100;return p;}
static void laser(maze_model_t*m,const maze_dynamic_t*d,unsigned node,unsigned r){maze_point_t p=point(node,r,20,50);move(m,p);
 while(((m->dynamic_us/1000+d->phase_ms)%3600)>=100 && m->result==MAZE_MODEL_RUNNING)maze_model_step(m,control(p.x-m->x,m->vx),control(p.y-m->y,m->vy),10);
 move(m,point(node,r,80,50));}
int main(void){
 for(unsigned kind=1;kind<=4;kind++)for(unsigned r=0;r<4;r++)for(unsigned phase=0;phase<3;phase++){
  maze_generated_t out={0};out.definition.dynamics=out.dynamics;maze_random_dynamic(&out,0,r,(maze_template_kind_t)kind);
  maze_level_t l={.width=100,.height=100,.ball_radius=7,.start=maze_random_point((maze_point_t){10,50},r),.goal={0,0,1},.dynamics=out.dynamics,.dynamic_count=1};
  maze_point_t end=maze_random_point((maze_point_t){90,50},r);l.goal=(maze_circle_t){end.x,end.y,5};maze_model_t m;maze_model_reset(&m,&l);m.dynamic_us=phase==0?0:phase==1?2399000:3599000;
  if(kind==MAZE_TEMPLATE_LASER)laser(&m,&out.dynamics[0],0,r);
  move(&m,end);assert(m.result==MAZE_MODEL_WON);
 }
 puts("PASS: four dynamic templates x four rotations x three arrival phases real-model traversal");
 unsigned respawns=0;
 for(uint32_t seed=0;seed<4;seed++){
  maze_generator_begin(&generator,&generated,seed,false);int result;
  do{result=maze_generator_step(&generator,1024);if(!result&&maze_map_load(&generated.definition,&map))result=maze_generator_reject(&generator);}while(result==MAZE_RANDOM_PENDING);assert(!result);
  maze_model_t m;maze_model_reset(&m,&map.level);
  for(unsigned i=0;i<generated.path_count&&m.result==MAZE_MODEL_RUNNING;i++){
   unsigned node=generated.path[i];
   if(generated.types[node]==MAZE_TEMPLATE_LASER){unsigned prev=generated.path[i-1];int d=(int)node-(int)prev;unsigned r=d==1?0:d==6?1:d==-1?2:3;
    if(!respawns){
     maze_point_t wait=point(node,r,20,50);move(&m,wait);
     while((m.dynamic_us/1000)%3600<2400||(m.dynamic_us/1000)%3600>=2500)maze_model_step(&m,control(wait.x-m.x,m.vx),control(wait.y-m.y,m.vy),10);
     maze_point_t danger=point(node,r,50,50);
     for(unsigned t=0;t<1000&&m.result==MAZE_MODEL_RUNNING;t++)maze_model_step(&m,control(danger.x-m.x,m.vx),control(danger.y-m.y,m.vy),10);
     assert(m.result==MAZE_MODEL_LOST&&m.loss_reason==MAZE_LOSS_LASER&&m.checkpoint);
     assert(maze_model_respawn(&m));respawns++;
    }
    laser(&m,&generated.dynamics[generated.dynamic_index[node]],node,r);}
   else move(&m,point(node,0,50,50));
  }
  move(&m,(maze_point_t){map.level.goal.x,map.level.goal.y});assert(m.result==MAZE_MODEL_WON);
  printf("PASS: random seed=%u nodes=%u branches=%u model_time_ms=%llu WON checkpoint=%u\n",generated.seed,generated.path_count,generated.branches,(unsigned long long)(m.dynamic_us/1000),m.checkpoint);
 }
 assert(respawns==1);puts("PASS: actual random-map laser loss, checkpoint respawn and continued traversal");
 return 0;}
