/* 版本1确定性生成：树形骨架、受控模板和有界候选，不依赖RTC或libc rand。 */
#include "maze_random_templates.h"
#include <string.h>
static uint32_t next(maze_generator_t*g){g->rng=g->rng*1664525u+1013904223u;return g->rng>>16;}
static unsigned degree(unsigned links){unsigned n=0;while(links){n+=links&1u;links>>=1;}return n;}
static unsigned rotation(unsigned from,unsigned to){int d=(int)to-(int)from;return d==1?0:d==6?1:d==-1?2:3;}
static void reset(maze_generator_t*g){maze_generated_t*out=g->out;memset(out,0,sizeof(*out));out->seed=g->seed;memset(out->dynamic_index,255,sizeof(out->dynamic_index));memset(out->terrain,'.',sizeof(out->terrain));
 out->definition=(maze_map_definition_t){.id=MAZE_RANDOM_ID,.columns=12,.rows=10,.lines=out->line_ptrs,.line_count=21,.objects=out->objects,.mechanisms=out->mechanisms,.dynamics=out->dynamics};
 for(unsigned i=0;i<21;i++)out->line_ptrs[i]=out->lines[i];
 g->rng=g->seed^0x9e3779b9u;unsigned corners[]={0,5,24,29};g->start=(uint8_t)corners[next(g)%4];g->stack[0]=g->start;g->depth=1;g->visited=1u<<g->start;
 memset(g->parent,255,sizeof(g->parent));memset(g->distance,0,sizeof(g->distance));g->phase=1;g->cursor=0;}
void maze_generator_begin(maze_generator_t*g,maze_generated_t*out,uint32_t seed,bool exact){memset(g,0,sizeof(*g));g->out=out;g->seed=seed;g->exact=exact;g->result=MAZE_RANDOM_PENDING;reset(g);}
void maze_generator_cancel(maze_generator_t*g){g->result=MAZE_RANDOM_EXHAUSTED;}
int maze_generator_reject(maze_generator_t*g){if(g->exact){g->result=MAZE_RANDOM_REBUILD;return g->result;}if(++g->attempt>=8){g->result=MAZE_RANDOM_EXHAUSTED;return g->result;}g->seed=g->seed*1664525u+1013904223u;reset(g);return g->result=MAZE_RANDOM_PENDING;}
static void checkpoint(maze_generated_t*out,unsigned node){out->types[node]=MAZE_TEMPLATE_CHECKPOINT;out->mechanisms[out->definition.mechanism_count++]=(maze_mechanism_t){MAZE_MECH_CHECKPOINT,{(node%6)*100+40,(node/6)*100+40,20,20},0};}
static bool assemble(maze_generator_t*g){maze_generated_t*out=g->out;g->goal=g->start;
 for(unsigned i=0;i<30;i++){if(g->distance[i]>g->distance[g->goal])g->goal=(uint8_t)i;if(degree(out->links[i])>=3)out->branches++;}
 unsigned node=g->goal;while(node!=255){out->path[out->path_count++]=(uint8_t)node;node=g->parent[node];}
 for(unsigned i=0;i<out->path_count/2;i++){uint8_t a=out->path[i];out->path[i]=out->path[out->path_count-1-i];out->path[out->path_count-1-i]=a;}
 if(out->path_count<10||out->branches<3)return false;
 unsigned eligible[30],count=0;
 for(unsigned i=2;i+2<out->path_count;i++)if(degree(out->links[out->path[i]])==2&&(int)out->path[i]-(int)out->path[i-1]==(int)out->path[i+1]-(int)out->path[i])eligible[count++]=i;
 unsigned first=0,second=0,pairs=0;
 for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)if(eligible[j]>=eligible[i]+2){if(next(g)%++pairs==0){first=eligible[i];second=eligible[j];}}
 if(!pairs)return false;
 unsigned kind=next(g)%4+1,other=(kind-1+1+next(g)%3)%4+1;
 maze_random_dynamic(out,out->path[first],rotation(out->path[first-1],out->path[first]),(maze_template_kind_t)kind);
 maze_random_dynamic(out,out->path[second],rotation(out->path[second-1],out->path[second]),(maze_template_kind_t)other);
 checkpoint(out,out->path[first-1]);checkpoint(out,out->path[second-1]);
 /* 可选的一组单调触发门，触发物始终位于唯一主路的门前。 */
 if(next(g)&1u)for(unsigned j=0;j<count;j++){unsigned at=eligible[j],door=out->path[at];if(out->types[door])continue;
  unsigned key_at=1;while(key_at<at&&out->types[out->path[key_at]])key_at++;if(key_at==at)continue;
  unsigned key=out->path[key_at];bool sw=(next(g)&1u)!=0;out->types[key]=sw?MAZE_TEMPLATE_SWITCH:MAZE_TEMPLATE_KEY;out->types[door]=MAZE_TEMPLATE_DOOR;
  out->mechanisms[out->definition.mechanism_count++]=(maze_mechanism_t){sw?MAZE_MECH_SWITCH:MAZE_MECH_KEY,{(key%6)*100+40,(key/6)*100+40,20,20},0};
  maze_rect_t a=maze_random_rect((maze_rect_t){46,4,8,92},rotation(out->path[at-1],door));a.x+=(door%6)*100;a.y+=(door/6)*100;
  out->mechanisms[out->definition.mechanism_count++]=(maze_mechanism_t){sw?MAZE_MECH_SWITCH_DOOR:MAZE_MECH_KEY_DOOR,a,0};break;}
 out->objects[out->definition.object_count++]=(maze_map_object_t){MAZE_MAP_START,{(g->start%6)*100+25,(g->start/6)*100+25,7}};
 out->objects[out->definition.object_count++]=(maze_map_object_t){MAZE_MAP_GOAL,{(g->goal%6)*100+25,(g->goal/6)*100+25,14}};
 for(unsigned i=first+1;i+1<out->path_count;i++){unsigned at=out->path[i];if(out->types[at])continue;out->types[at]=MAZE_TEMPLATE_TRAP;
  out->objects[out->definition.object_count++]=(maze_map_object_t){MAZE_MAP_TRAP,{(at%6)*100+68,(at/6)*100+68,8}};break;}
 for(unsigned i=1;i+1<out->path_count;i++){unsigned at=out->path[i];if(!out->types[at]){out->terrain[at]=(next(g)&1u)?'~':':';break;}}
 return true;}
static char character(const maze_generated_t*out,unsigned x,unsigned y){
 if(!(x%2)&&!(y%2))return '+';
 if(!(y%2)){unsigned sy=y/2;if(!sy||sy==10)return '-';if(sy%2)return ' ';unsigned mx=x/4,my=sy/2;return(out->links[(my-1)*6+mx]&2u)?' ':'-';}
 if(!(x%2)){unsigned sx=x/2;if(!sx||sx==12)return '|';if(sx%2)return ' ';unsigned mx=sx/2,my=y/4;return(out->links[my*6+mx-1]&1u)?' ':'|';}
 return out->terrain[(y/4)*6+x/4];}
int maze_generator_step(maze_generator_t*g,unsigned operations){if(g->result!=MAZE_RANDOM_PENDING)return g->result;
 while(operations--){maze_generated_t*out=g->out;
  if(g->phase==1){if(!g->depth){if(!assemble(g)){int r=maze_generator_reject(g);if(r!=MAZE_RANDOM_PENDING)return r;continue;}g->phase=2;g->cursor=0;continue;}
   unsigned node=g->stack[g->depth-1],neighbors[4],dirs[4],n=0;int x=node%6,y=node/6;const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
   for(unsigned d=0;d<4;d++){int xx=x+dx[d],yy=y+dy[d];if(xx<0||xx>=6||yy<0||yy>=5)continue;unsigned next_node=(unsigned)yy*6+(unsigned)xx;if(!(g->visited&(1u<<next_node))){neighbors[n]=next_node;dirs[n++]=d;}}
   if(!n){g->depth--;continue;}unsigned chosen=next(g)%n,next_node=neighbors[chosen],d=dirs[chosen];out->links[node]|=(uint8_t)(1u<<d);out->links[next_node]|=(uint8_t)(1u<<((d+2)%4));
   g->visited|=1u<<next_node;g->parent[next_node]=(uint8_t)node;g->distance[next_node]=(uint8_t)(g->distance[node]+1);g->stack[g->depth++]=(uint8_t)next_node;
  }else if(g->phase==2){unsigned y=g->cursor/26,x=g->cursor%26;out->lines[y][x]=x==25?0:character(out,x,y);if(++g->cursor==21*26){g->phase=3;return g->result=0;}}
 }
 return MAZE_RANDOM_PENDING;}
