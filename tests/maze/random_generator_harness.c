#include "maze_random.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static maze_generated_t a,b;static maze_generator_t g,h;static maze_map_t map;
static int generate(maze_generator_t*j,maze_generated_t*out,uint32_t seed,bool exact,unsigned budget){maze_generator_begin(j,out,seed,exact);int r;
 do{r=maze_generator_step(j,budget);if(!r){r=maze_map_load(&out->definition,&map);if(r)r=maze_generator_reject(j);}}while(r==MAZE_RANDOM_PENDING);return r;}
int main(void){unsigned accepted=0,different=0;char previous[sizeof(a.lines)]={0};
 for(unsigned i=0;i<32;i++){uint32_t seed=i==31?0xffffffffu:i;int r=generate(&g,&a,seed,false,1024);if(r){assert(r==MAZE_RANDOM_EXHAUSTED);continue;}
 assert(a.path_count>=10 && a.branches>=3 && a.definition.dynamic_count>=2 && a.definition.dynamic_count<=4);accepted++;
 unsigned cp=a.path_count,trap=a.path_count;
 for(unsigned j=0;j<a.path_count;j++){if(a.types[a.path[j]]==MAZE_TEMPLATE_CHECKPOINT&&cp==a.path_count)cp=j;if(a.types[a.path[j]]==MAZE_TEMPLATE_TRAP)trap=j;}
 assert(cp<trap&&trap<a.path_count);
 assert(!generate(&h,&b,a.seed,true,1));assert(a.seed==b.seed&&!memcmp(a.lines,b.lines,sizeof(a.lines)));
 assert(a.definition.object_count==b.definition.object_count&&a.definition.mechanism_count==b.definition.mechanism_count);
 if(i&&memcmp(previous,a.lines,sizeof(a.lines)))different++;
 memcpy(previous,a.lines,sizeof(a.lines));
 }
 assert(accepted>=28 && different>=20);printf("PASS: random generator accepted=%u/32 distinct-adjacent=%u; exact seed and budget independence\n",accepted,different);
 /* 几何验证器连续拒绝候选时，第八次必须停止；exact不推进种子。 */
 maze_generator_begin(&g,&a,0,false);
 for(unsigned i=1;i<=8;i++) assert(maze_generator_reject(&g)==(i<8?MAZE_RANDOM_PENDING:MAZE_RANDOM_EXHAUSTED));
 assert(g.attempt==8&&maze_generator_step(&g,1024)==MAZE_RANDOM_EXHAUSTED);
 maze_generator_begin(&g,&a,0,true);assert(maze_generator_reject(&g)==MAZE_RANDOM_REBUILD&&g.seed==0&&g.attempt==0);
 maze_generator_begin(&g,&a,0,false);maze_generator_cancel(&g);assert(maze_generator_step(&g,1)<0);
 puts("PASS: eight rejected candidates exhaust, exact rejection retains seed0, cancellation stops");return 0;}
