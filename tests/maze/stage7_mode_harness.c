#include "maze_navigation.h"
#include <assert.h>
#include <stdio.h>
static maze_navigation_t n;
int main(void){maze_navigation_init(&n,1000);maze_navigation_toggle_dark(&n);maze_navigation_toggle_direction(&n);assert(n.dark&&n.reversed);assert(!maze_navigation_select(&n,0));assert(maze_app_mode(&n.app)==3);
 uint32_t version=n.page_version;maze_navigation_toggle_dark(&n);maze_navigation_toggle_direction(&n);assert(n.page_version==version&&maze_app_mode(&n.app)==3);
 maze_navigation_back(&n);maze_navigation_toggle_dark(&n);assert(!maze_navigation_select(&n,0)&&maze_app_mode(&n.app)==2);
 maze_navigation_back(&n);maze_navigation_toggle_dark(&n);maze_navigation_toggle_direction(&n);assert(!maze_navigation_random_open(&n,0));int r;do{r=maze_navigation_random_step(&n,0,NULL);}while(r==MAZE_RANDOM_PENDING);assert(r==MAZE_RANDOM_READY);assert(!maze_navigation_random_accept(&n,n.random_job_id));assert(n.random&&maze_app_mode(&n.app)==1);
 puts("PASS: four independent settings combinations, active session frozen, fixed and random bind selected mode");}
