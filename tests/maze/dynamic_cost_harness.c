/* 用真实第五图和函数调用预算约束精确碰撞开销，不代表板端耗时。 */
#include <assert.h>
#include <stdio.h>
#include "maze_model.c"
#include "maze_map.h"
static unsigned expensive_entries;
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *fn,void *site)
{(void)site;if(fn==(void*)circle_entry) expensive_entries++;}
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *fn,void *site)
{(void)fn;(void)site;}
static maze_map_t map;
int main(void){assert(!maze_map_load(&maze_map_dynamic_trial,&map));maze_model_t m;maze_model_reset(&m,&map.level);
 expensive_entries=0;maze_model_step(&m,0,0,10);
 printf("precise_circle_entries_per_10ms=%u\n",expensive_entries);fflush(stdout);
 assert(expensive_entries<=100);assert(m.x==25&&m.y==25&&m.dynamic_us==10000);
 puts("PASS: distant walls do not invoke expensive corner sweep; original state/time retained");return 0;}
