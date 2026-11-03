/* 同步与逐检查校验相同，且截止/取消不发布活动图。 */
#include "maze_map.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static maze_map_t a,b;
static uint32_t tick;
static uint32_t now(void){return tick++;}
int main(void){const maze_map_definition_t* defs[]={&maze_map_ice_loops,&maze_map_mechanism_trial,&maze_map_dynamic_trial};
 for(unsigned i=0;i<3;i++){
 assert(!maze_map_load(defs[i],&a));maze_map_begin(defs[i],&b);int result=MAZE_MAP_PENDING;unsigned calls=0;
 while(result==MAZE_MAP_PENDING){result=maze_map_step(1,1,0,NULL);assert(++calls<10000000);}
 assert(result==0 && a.checked_nodes==b.checked_nodes);
 assert(a.level.wall_count==b.level.wall_count && !memcmp(a.walls,b.walls,a.level.wall_count*sizeof(a.walls[0])));
 assert(a.level.trap_count==b.level.trap_count && !memcmp(a.traps,b.traps,a.level.trap_count*sizeof(a.traps[0])));
 }
 maze_map_begin(defs[2],&b);tick=0xfffffff0;assert(maze_map_step(128,1024,0xfffffff5,now)==MAZE_MAP_PENDING);
 assert(tick<=0xfffffff6);maze_map_cancel();
 maze_map_begin(defs[0],&b);tick=0xfffffffd;assert(maze_map_step(128,1024,3,now)==MAZE_MAP_PENDING);assert(tick<=4);
 maze_map_cancel();assert(!maze_map_load(defs[0],&b));
 puts("PASS: three maps sync/incremental identical; per-check yield, deadline wrap and cancel/reuse");return 0;}
