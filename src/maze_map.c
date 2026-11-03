/* 单一校验状态机：几何对象及BFS邻边均可让出，同步入口复用相同路径。 */
#include "maze_map.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#define CHECK_COLUMNS (MAZE_MAP_MAX_COLUMNS*MAZE_MAP_CELL_PX/MAZE_MAP_CHECK_STEP_PX+1)
#define CHECK_ROWS (MAZE_MAP_MAX_ROWS*MAZE_MAP_CELL_PX/MAZE_MAP_CHECK_STEP_PX+1)
#define CHECK_NODES (CHECK_COLUMNS*CHECK_ROWS)
static struct {uint16_t queue[CHECK_NODES];uint8_t seen[(CHECK_NODES+7)/8];uint8_t edges[(2*CHECK_NODES+7)/8];} work;
/* 所有跨让出点的游标/值都在静态任务中，不能依赖线程局部变量继续存活。 */
static struct {
 const maze_map_definition_t *d;maze_map_t *out;
 unsigned pc,i,j,k,x,y,starts,goals,head,tail,nx,ny,node,ix,iy,edge,pass,ops,nodes;
 int xx,yy,result,xmin,xmax,ymin,ymax;
 uint32_t keys,switches,next_keys,next_switches,reached;
 float ax,ay,bx,by;bool clear,goal,found;
 maze_circle_t start,c;maze_rect_t area;
} job;
size_t maze_map_workspace_size(void){return sizeof(work)+sizeof(job);}
static float distance_sq(float ax,float ay,float bx,float by){float x=ax-bx,y=ay-by;return x*x+y*y;}
static float wall_distance_sq(float ax,float ay,float bx,float by,maze_rect_t w){
 float x=fmaxf(fmaxf(w.x-fmaxf(ax,bx),fminf(ax,bx)-w.x-w.width),0);
 float y=fmaxf(fmaxf(w.y-fmaxf(ay,by),fminf(ay,by)-w.y-w.height),0);return x*x+y*y;}
static bool circle_bounds(const maze_level_t*l,maze_circle_t c){return isfinite(c.x)&&isfinite(c.y)&&isfinite(c.radius)&&c.radius>0&&
 c.x>=c.radius&&c.y>=c.radius&&c.x+c.radius<=l->width&&c.y+c.radius<=l->height;}
static bool circles_overlap(maze_circle_t a,maze_circle_t b){float r=a.radius+b.radius;return distance_sq(a.x,a.y,b.x,b.y)<=r*r;}
static bool centered(maze_circle_t c){return fmodf(c.x,MAZE_MAP_CELL_PX)==MAZE_MAP_CELL_PX/2&&fmodf(c.y,MAZE_MAP_CELL_PX)==MAZE_MAP_CELL_PX/2;}
static maze_rect_t sweep(maze_dynamic_t d){maze_rect_t a=d.area;if(d.kind==MAZE_DYNAMIC_PATROL){a.x=fminf(a.x,d.end.x);a.y=fminf(a.y,d.end.y);a.width+=fabsf(d.end.x-d.area.x);a.height+=fabsf(d.end.y-d.area.y);}return a;}
static bool rect_overlap(maze_rect_t a,maze_rect_t b){return a.x<b.x+b.width&&b.x<a.x+a.width&&a.y<b.y+b.height&&b.y<a.y+a.height;}
static bool rect_bounds(const maze_level_t*l,maze_rect_t a){return isfinite(a.x)&&isfinite(a.y)&&isfinite(a.width)&&isfinite(a.height)&&a.width>0&&a.height>0&&a.x>=0&&a.y>=0&&a.x+a.width<=l->width&&a.y+a.height<=l->height;}
void maze_map_begin(const maze_map_definition_t*d,maze_map_t*out){memset(&job,0,sizeof(job));job.d=d;job.out=out;job.result=MAZE_MAP_PENDING;}
void maze_map_cancel(void){job.result=MAZE_MAP_FORMAT;job.pc=0;}
#define DONE(code) do{job.result=(code);job.pc=0;return job.result;}while(0)
/* case恢复到检查前，因此新片的时间/操作预算同样生效。 */
#define CHECK() do{case __LINE__: if(job.ops>=check_budget||(clock_now&&(int32_t)(clock_now()-deadline_tick)>=0)){job.pc=__LINE__;return MAZE_MAP_PENDING;}job.ops++;}while(0)
#define NODE() do{case __LINE__: if(job.nodes>=node_budget||(clock_now&&(int32_t)(clock_now()-deadline_tick)>=0)){job.pc=__LINE__;return MAZE_MAP_PENDING;}job.nodes++;}while(0)
#define O (l->mechanisms[job.i])
#define D (l->dynamics[job.i])
int maze_map_step(unsigned node_budget,unsigned check_budget,uint32_t deadline_tick,uint32_t(*clock_now)(void)){
 const maze_map_definition_t*d=job.d;maze_level_t*l=&job.out->level;
 if(job.result!=MAZE_MAP_PENDING)return job.result;
 job.ops=job.nodes=0;
 switch(job.pc){case 0:
 CHECK();
 if(!d->id[0]||!d->columns||!d->rows||d->columns>MAZE_MAP_MAX_COLUMNS||d->rows>MAZE_MAP_MAX_ROWS||d->line_count!=2*d->rows+1)DONE(MAZE_MAP_FORMAT);
 for(job.y=0;job.y<d->line_count;job.y++){
  for(job.x=0;job.x<2*d->columns+1;job.x++){
   CHECK();
   char c=d->lines[job.y][job.x];if(!c)DONE(MAZE_MAP_FORMAT);
   if(!(job.x%2)&&!(job.y%2)){if(c!='+')DONE(MAZE_MAP_FORMAT);}
   else if(!(job.y%2)){if(c!='-'&&c!=' ')DONE(MAZE_MAP_FORMAT);if((job.y==0||job.y+1==d->line_count)&&c!='-')DONE(MAZE_MAP_FORMAT);}
   else if(!(job.x%2)){if(c!='|'&&c!=' ')DONE(MAZE_MAP_FORMAT);if((job.x==0||job.x==2*d->columns)&&c!='|')DONE(MAZE_MAP_FORMAT);}
   else if(c!='.'&&c!='~'&&c!=':')DONE(MAZE_MAP_FORMAT);
  }
  if(d->lines[job.y][2*d->columns+1])DONE(MAZE_MAP_FORMAT);
 }
 if(d->object_count>MAZE_MAP_MAX_OBJECTS||d->mechanism_count>MAZE_MAP_MAX_MECHANISMS||(d->mechanism_count&&!d->mechanisms)||d->dynamic_count>MAZE_MAP_MAX_DYNAMICS||(d->dynamic_count&&!d->dynamics))DONE(MAZE_MAP_OBJECT);
 memset(job.out,0,sizeof(*job.out));l->id=d->id;l->width=d->columns*MAZE_MAP_CELL_PX;l->height=d->rows*MAZE_MAP_CELL_PX;
 l->mechanisms=d->mechanisms;l->mechanism_count=d->mechanism_count;l->dynamics=d->dynamics;l->dynamic_count=d->dynamic_count;
 l->walls=job.out->walls;l->surfaces=job.out->surfaces;l->traps=job.out->traps;
 for(job.y=0;job.y<d->rows;job.y++)for(job.x=0;job.x<d->columns;job.x++){
  CHECK();
  float px=job.x*MAZE_MAP_CELL_PX,py=job.y*MAZE_MAP_CELL_PX,half=MAZE_MAP_WALL_PX/2;
  char c=d->lines[2*job.y+1][2*job.x+1];
  if(c=='~'||c==':')job.out->surfaces[l->surface_count++]=(maze_surface_area_t){{px,py,MAZE_MAP_CELL_PX,MAZE_MAP_CELL_PX},c=='~'?MAZE_SURFACE_ICE:MAZE_SURFACE_ROUGH};
  if(job.x+1<d->columns&&d->lines[2*job.y+1][2*job.x+2]=='|'){float lo=fmaxf(0,py-half),hi=fminf(l->height,py+MAZE_MAP_CELL_PX+half);job.out->walls[l->wall_count++]=(maze_rect_t){px+MAZE_MAP_CELL_PX-half,lo,MAZE_MAP_WALL_PX,hi-lo};}
  if(job.y+1<d->rows&&d->lines[2*job.y+2][2*job.x+1]=='-'){float lo=fmaxf(0,px-half),hi=fminf(l->width,px+MAZE_MAP_CELL_PX+half);job.out->walls[l->wall_count++]=(maze_rect_t){lo,py+MAZE_MAP_CELL_PX-half,hi-lo,MAZE_MAP_WALL_PX};}
 }
 for(job.i=0;job.i<d->object_count;job.i++){
  CHECK();
  maze_map_object_t o=d->objects[job.i];
  if(o.kind==MAZE_MAP_START){job.start=o.circle;job.starts++;}
  else if(o.kind==MAZE_MAP_GOAL){l->goal=o.circle;job.goals++;}
  else if(o.kind==MAZE_MAP_TRAP){if(l->trap_count==MAZE_MAP_MAX_CELLS)DONE(MAZE_MAP_OBJECT);job.out->traps[l->trap_count++]=o.circle;}
  else DONE(MAZE_MAP_OBJECT);
 }
 if(job.starts!=1||job.goals!=1)DONE(MAZE_MAP_OBJECT);
 l->start=(maze_point_t){job.start.x,job.start.y};l->ball_radius=job.start.radius;
 if(!centered(job.start)||!centered(l->goal)||2*job.start.radius>MAZE_MAP_CELL_PX-MAZE_MAP_WALL_PX||circles_overlap(job.start,l->goal))DONE(MAZE_MAP_GEOMETRY);
 for(job.i=0;job.i<l->trap_count+2;job.i++){
  job.c=job.i==0?job.start:job.i==1?l->goal:l->traps[job.i-2];
  if(!circle_bounds(l,job.c))DONE(MAZE_MAP_GEOMETRY);
  for(job.j=0;job.j<l->wall_count;job.j++){
   CHECK();
   if(wall_distance_sq(job.c.x,job.c.y,job.c.x,job.c.y,l->walls[job.j])<job.c.radius*job.c.radius)DONE(MAZE_MAP_GEOMETRY);
  }
  if(job.i>=2){
   if(circles_overlap(job.c,job.start)||circles_overlap(job.c,l->goal))DONE(MAZE_MAP_GEOMETRY);
   for(job.j=0;job.j<job.i-2;job.j++){
    CHECK();
    if(circles_overlap(job.c,l->traps[job.j]))DONE(MAZE_MAP_GEOMETRY);
   }
  }
 }
 for(job.i=0;job.i<l->mechanism_count;job.i++){
  CHECK();
  if(O.kind<MAZE_MECH_KEY||O.kind>MAZE_MECH_CHECKPOINT||!rect_bounds(l,O.area))DONE(MAZE_MAP_OBJECT);
  if(O.kind!=MAZE_MECH_BREAKABLE&&O.kind!=MAZE_MECH_CHECKPOINT&&O.link>7)DONE(MAZE_MAP_OBJECT);
  if(O.kind==MAZE_MECH_KEY_DOOR||O.kind==MAZE_MECH_SWITCH_DOOR){
   job.found=false;
   for(job.j=0;job.j<l->mechanism_count;job.j++){
    CHECK();
    if(l->mechanisms[job.j].kind==(O.kind==MAZE_MECH_KEY_DOOR?MAZE_MECH_KEY:MAZE_MECH_SWITCH)&&l->mechanisms[job.j].link==O.link)job.found=true;
   }
   if(!job.found)DONE(MAZE_MAP_OBJECT);
  }
  if(O.kind==MAZE_MECH_KEY_DOOR||O.kind==MAZE_MECH_SWITCH_DOOR||O.kind==MAZE_MECH_BREAKABLE){
   if(wall_distance_sq(l->start.x,l->start.y,l->start.x,l->start.y,O.area)<l->ball_radius*l->ball_radius||wall_distance_sq(l->goal.x,l->goal.y,l->goal.x,l->goal.y,O.area)<l->goal.radius*l->goal.radius)DONE(MAZE_MAP_OBJECT);
  }else{
   job.c=(maze_circle_t){O.area.x+O.area.width/2,O.area.y+O.area.height/2,fminf(O.area.width,O.area.height)/2};
   if(O.kind==MAZE_MECH_CHECKPOINT)job.c.radius=fmaxf(job.c.radius,l->ball_radius);
   if(!circle_bounds(l,job.c))DONE(MAZE_MAP_OBJECT);
   for(job.j=0;job.j<l->wall_count;job.j++){
    CHECK();
    if(wall_distance_sq(job.c.x,job.c.y,job.c.x,job.c.y,l->walls[job.j])<job.c.radius*job.c.radius)DONE(MAZE_MAP_OBJECT);
   }
   for(job.j=0;job.j<l->trap_count;job.j++){
    CHECK();
    if(circles_overlap(job.c,l->traps[job.j]))DONE(MAZE_MAP_OBJECT);
   }
   for(job.j=0;job.j<l->mechanism_count;job.j++){
    CHECK();
    maze_mechanism_t b=l->mechanisms[job.j];
    if((b.kind==MAZE_MECH_KEY_DOOR||b.kind==MAZE_MECH_SWITCH_DOOR||b.kind==MAZE_MECH_BREAKABLE)&&wall_distance_sq(job.c.x,job.c.y,job.c.x,job.c.y,b.area)<job.c.radius*job.c.radius)DONE(MAZE_MAP_OBJECT);
   }
  }
 }
 for(job.i=0;job.i<l->dynamic_count;job.i++){
  CHECK();
  if(D.kind<MAZE_DYNAMIC_MAGNET||D.kind>MAZE_DYNAMIC_PATROL||!rect_bounds(l,D.area))DONE(MAZE_MAP_OBJECT);
  if(D.kind==MAZE_DYNAMIC_MAGNET){if(!isfinite(D.force)||!isfinite(D.inner_radius)||!isfinite(D.outer_radius)||D.force<=0||D.force>MAZE_MAGNET_FORCE_MAX||D.inner_radius<=0||D.outer_radius<=D.inner_radius)DONE(MAZE_MAP_OBJECT);}
  else if(D.kind==MAZE_DYNAMIC_PATROL){if(!isfinite(D.speed)||D.speed<=0||D.speed>MAZE_SPEED_MAX||!isfinite(D.end.x)||!isfinite(D.end.y)||((D.end.x==D.area.x)==(D.end.y==D.area.y)))DONE(MAZE_MAP_OBJECT);
   double period=2*(hypot(D.end.x-D.area.x,D.end.y-D.area.y)*1000/D.speed+D.dwell_ms);if(D.phase_ms>=period)DONE(MAZE_MAP_OBJECT);}
  else if(!D.off_ms||!D.warning_ms||!D.on_ms||(uint64_t)D.off_ms+D.warning_ms+D.on_ms>UINT32_MAX||D.phase_ms>=(uint64_t)D.off_ms+D.warning_ms+D.on_ms)DONE(MAZE_MAP_OBJECT);
  job.area=sweep(D);if(!rect_bounds(l,job.area))DONE(MAZE_MAP_OBJECT);
  if(D.kind==MAZE_DYNAMIC_MAGNET)continue;
  for(job.j=0;job.j<l->wall_count;job.j++){
   CHECK();
   if(rect_overlap(job.area,l->walls[job.j]))DONE(MAZE_MAP_OBJECT);
  }
  if(wall_distance_sq(l->start.x,l->start.y,l->start.x,l->start.y,job.area)<=l->ball_radius*l->ball_radius||wall_distance_sq(l->goal.x,l->goal.y,l->goal.x,l->goal.y,job.area)<=l->goal.radius*l->goal.radius)DONE(MAZE_MAP_OBJECT);
  for(job.j=0;job.j<l->mechanism_count;job.j++){
   CHECK();
   maze_mechanism_t b=l->mechanisms[job.j];
   if(b.kind==MAZE_MECH_CHECKPOINT){float x=b.area.x+b.area.width/2,y=b.area.y+b.area.height/2;if(wall_distance_sq(x,y,x,y,job.area)<=l->ball_radius*l->ball_radius)DONE(MAZE_MAP_OBJECT);}
   else if((b.kind==MAZE_MECH_KEY_DOOR||b.kind==MAZE_MECH_SWITCH_DOOR||b.kind==MAZE_MECH_BREAKABLE)&&rect_overlap(job.area,b.area))DONE(MAZE_MAP_OBJECT);
  }
  for(job.j=0;job.j<job.i;job.j++){
   CHECK();
   if(l->dynamics[job.j].kind!=MAZE_DYNAMIC_MAGNET&&rect_overlap(job.area,sweep(l->dynamics[job.j])))DONE(MAZE_MAP_OBJECT);
  }
 }
 job.nx=(unsigned)l->width/MAZE_MAP_CHECK_STEP_PX+1;job.ny=(unsigned)l->height/MAZE_MAP_CHECK_STEP_PX+1;
 /* 静态边只在墙/坑附近栅格化一次；BFS各机关状态复用精确边掩码。 */
 memset(work.edges,0,sizeof(work.edges));
 for(job.i=0;job.i<l->wall_count+l->trap_count;job.i++){
  if(job.i<l->wall_count){job.area=l->walls[job.i];float r=l->ball_radius+MAZE_MAP_CHECK_STEP_PX;
   job.xmin=(int)floorf((job.area.x-r)/MAZE_MAP_CHECK_STEP_PX);job.xmax=(int)ceilf((job.area.x+job.area.width+r)/MAZE_MAP_CHECK_STEP_PX);
   job.ymin=(int)floorf((job.area.y-r)/MAZE_MAP_CHECK_STEP_PX);job.ymax=(int)ceilf((job.area.y+job.area.height+r)/MAZE_MAP_CHECK_STEP_PX);}
  else{job.c=l->traps[job.i-l->wall_count];float r=job.c.radius+MAZE_MAP_CHECK_STEP_PX;
   job.xmin=(int)floorf((job.c.x-r)/MAZE_MAP_CHECK_STEP_PX);job.xmax=(int)ceilf((job.c.x+r)/MAZE_MAP_CHECK_STEP_PX);
   job.ymin=(int)floorf((job.c.y-r)/MAZE_MAP_CHECK_STEP_PX);job.ymax=(int)ceilf((job.c.y+r)/MAZE_MAP_CHECK_STEP_PX);}
  if(job.xmin<0)job.xmin=0;
  if(job.ymin<0)job.ymin=0;
  if(job.xmax>=(int)job.nx)job.xmax=(int)job.nx-1;
  if(job.ymax>=(int)job.ny)job.ymax=(int)job.ny-1;
  for(job.yy=job.ymin;job.yy<=job.ymax;job.yy++)for(job.xx=job.xmin;job.xx<=job.xmax;job.xx++)for(job.edge=0;job.edge<2;job.edge++){
   CHECK();
   if((job.edge==0&&job.xx+1>=(int)job.nx)||(job.edge==1&&job.yy+1>=(int)job.ny))continue;
   float ax=job.xx*MAZE_MAP_CHECK_STEP_PX,ay=job.yy*MAZE_MAP_CHECK_STEP_PX;
   float bx=ax+(job.edge==0?MAZE_MAP_CHECK_STEP_PX:0),by=ay+(job.edge==1?MAZE_MAP_CHECK_STEP_PX:0);
   bool blocked;
   if(job.i<l->wall_count)blocked=wall_distance_sq(ax,ay,bx,by,job.area)<l->ball_radius*l->ball_radius;
   else{float x=fminf(fmaxf(job.c.x,ax),bx),y=fminf(fmaxf(job.c.y,ay),by);blocked=distance_sq(x,y,job.c.x,job.c.y)<=job.c.radius*job.c.radius;}
   if(blocked){unsigned at=((unsigned)job.yy*job.nx+(unsigned)job.xx)*2+job.edge;work.edges[at/8]|=(uint8_t)(1u<<(at%8));}
  }
 }

 for(job.pass=0;job.pass<=l->mechanism_count;job.pass++){
  job.next_keys=job.keys;job.next_switches=job.switches;job.goal=false;job.head=0;job.tail=1;
  memset(work.seen,0,sizeof(work.seen));
  job.node=(unsigned)(l->start.y/MAZE_MAP_CHECK_STEP_PX)*job.nx+(unsigned)(l->start.x/MAZE_MAP_CHECK_STEP_PX);
  work.queue[0]=(uint16_t)job.node;work.seen[job.node/8]|=(uint8_t)(1u<<(job.node%8));
  while(job.head<job.tail){
   NODE();
   job.node=work.queue[job.head++];job.ix=job.node%job.nx;job.iy=job.node/job.nx;
   job.ax=job.ix*MAZE_MAP_CHECK_STEP_PX;job.ay=job.iy*MAZE_MAP_CHECK_STEP_PX;job.out->checked_nodes++;
   if(distance_sq(job.ax,job.ay,l->goal.x,l->goal.y)<=l->goal.radius*l->goal.radius){job.goal=true;if(!l->mechanism_count)DONE(MAZE_MAP_OK);}
   for(job.i=0;job.i<l->mechanism_count;job.i++){
    CHECK();
    float r=fminf(O.area.width,O.area.height)/2+l->ball_radius;
    if(distance_sq(job.ax,job.ay,O.area.x+O.area.width/2,O.area.y+O.area.height/2)>r*r)continue;
    if(O.kind==MAZE_MECH_KEY){job.next_keys|=1u<<O.link;job.reached|=1u<<job.i;}
    else if(O.kind==MAZE_MECH_SWITCH){job.next_switches|=1u<<O.link;job.reached|=1u<<job.i;}
    else if(O.kind==MAZE_MECH_CHECKPOINT)job.reached|=1u<<job.i;
   }
   for(job.edge=0;job.edge<4;job.edge++){
    job.xx=(int)job.ix+(job.edge==0?1:job.edge==1?-1:0);job.yy=(int)job.iy+(job.edge==2?1:job.edge==3?-1:0);
    if(job.xx<0||job.yy<0||(unsigned)job.xx>=job.nx||(unsigned)job.yy>=job.ny)continue;
    job.node=(unsigned)job.yy*job.nx+(unsigned)job.xx;if(work.seen[job.node/8]&(1u<<(job.node%8)))continue;
    job.bx=job.xx*MAZE_MAP_CHECK_STEP_PX;job.by=job.yy*MAZE_MAP_CHECK_STEP_PX;
    job.clear=fminf(job.ax,job.bx)>=l->ball_radius&&fminf(job.ay,job.by)>=l->ball_radius&&fmaxf(job.ax,job.bx)<=l->width-l->ball_radius&&fmaxf(job.ay,job.by)<=l->height-l->ball_radius;
    if(!job.clear)continue;
    CHECK();
    unsigned at=job.edge==0?(job.iy*job.nx+job.ix)*2:job.edge==1?job.node*2:
                job.edge==2?(job.iy*job.nx+job.ix)*2+1:job.node*2+1;
    if(work.edges[at/8]&(1u<<(at%8)))continue;
    for(job.j=0;job.j<l->mechanism_count;job.j++){
     CHECK();
     maze_mechanism_t m=l->mechanisms[job.j];bool blocked=m.kind==MAZE_MECH_KEY_DOOR?!(job.keys&(1u<<m.link)):m.kind==MAZE_MECH_SWITCH_DOOR?!(job.switches&(1u<<m.link)):false;
     if(blocked&&wall_distance_sq(job.ax,job.ay,job.bx,job.by,m.area)<l->ball_radius*l->ball_radius){job.clear=false;break;}
    }
    if(!job.clear)continue;
    work.seen[job.node/8]|=(uint8_t)(1u<<(job.node%8));work.queue[job.tail++]=(uint16_t)job.node;
   }
  }
  if(job.keys==job.next_keys&&job.switches==job.next_switches){
   if(!job.goal)DONE(MAZE_MAP_NO_ROUTE);
   for(job.i=0;job.i<l->mechanism_count;job.i++){
    CHECK();
    if((O.kind==MAZE_MECH_KEY||O.kind==MAZE_MECH_SWITCH||O.kind==MAZE_MECH_CHECKPOINT)&&!(job.reached&(1u<<job.i)))DONE(MAZE_MAP_NO_ROUTE);
   }
   DONE(MAZE_MAP_OK);
  }
  job.keys=job.next_keys;job.switches=job.next_switches;
 }
 DONE(MAZE_MAP_NO_ROUTE);
 }
 return job.result;
}
int maze_map_load(const maze_map_definition_t*d,maze_map_t*out){maze_map_begin(d,out);int result;do{result=maze_map_step(UINT_MAX,UINT_MAX,0,NULL);}while(result==MAZE_MAP_PENDING);return result;}
