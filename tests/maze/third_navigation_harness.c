/* 真实目录与业务，胜利为明确注入状态，不冒充实板通关。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "maze_navigation.h"
static maze_navigation_t nav;
static void won(double ms,uint32_t tick)
{
    maze_progress_before_event(&nav.progress,&nav.app,MAZE_EVENT_START);
    nav.app.state=MAZE_APP_WON;nav.app.elapsed_ms=ms;
    maze_navigation_update(&nav,tick);
    uint64_t revision=nav.progress.archive.revision;
    maze_navigation_update(&nav,tick);
    assert(nav.progress.archive.revision==revision);
}
int main(void)
{
    assert(maze_catalog_count==4);
    const char *ids[]={"frost_fork","ice_bends","ice_loops","mechanism_trial"};
    for(unsigned i=0;i<4;i++) assert(!strcmp(maze_catalog[i].definition->id,ids[i]));
    for(unsigned mode=0;mode<2;mode++)
    {
        maze_navigation_init(&nav,1000);
        assert(!maze_progress_unlocked(&nav.progress,"ice_loops"));
        assert(maze_navigation_select(&nav,2)==MAZE_NAV_LOCKED);
        nav.progress.archive.levels[1].unlocked=true;
        if(mode) maze_navigation_toggle_direction(&nav);
        assert(maze_navigation_select(&nav,1)==0);
        won(1230,1000);
        assert(nav.progress.archive.levels[1].completed==(1u<<mode));
        assert(nav.progress.archive.levels[2].unlocked && nav.progress.archive.levels[2].completed==0);
        uint32_t previous=nav.app.generation;
        assert(maze_navigation_next(&nav)==0);
        assert(nav.index==2 && !strcmp(nav.app.level->id,"ice_loops") && nav.app.reversed==(mode!=0));
        assert(nav.app.generation>previous && nav.app.state==MAZE_APP_READY);
        assert(nav.app.model.x==25 && nav.app.model.y==25 && nav.app.model.vx==0 && nav.app.elapsed_ms==0);
        maze_app_event(&nav.app,MAZE_EVENT_START,2000);
        maze_imu_sample_t old={.generation=previous,.sequence=1,.read_start_tick=2000};
        maze_navigation_sample(&nav,&old,2000);assert(nav.app.window_count==0);
        nav.app.state=MAZE_APP_READY;
        won(4560,3000);
        assert(nav.progress.level==2 && nav.progress.archive.levels[2].completed==(1u<<mode));
        assert(nav.progress.archive.levels[2].modes[mode].count==1 && nav.progress.archive.levels[2].modes[mode].scores[0].time_cs==456);
        assert(nav.progress.archive.levels[1].modes[mode].scores[0].time_cs==123);
        assert(nav.progress.archive.levels[3].unlocked && !nav.progress.archive.levels[3].completed);
        assert(maze_navigation_next(&nav)==0 && nav.index==3 && nav.app.state==MAZE_APP_READY);
        assert(!strcmp(nav.app.level->id,"mechanism_trial"));
        won(7890,4000);
        assert(nav.progress.archive.levels[3].modes[mode].scores[0].time_cs==789);
        uint32_t version=nav.page_version,generation=nav.app.generation;
        assert(maze_navigation_next(&nav)==0 && nav.index==3 && nav.page_version==version && nav.app.generation==generation);
        maze_navigation_back(&nav);assert(nav.page==MAZE_NAV_SELECT && nav.app.generation>generation);
        assert(maze_navigation_select(&nav,2)==0 && nav.app.state==MAZE_APP_READY);
    }
    puts("PASS: actual four-level catalog, shared unlock both modes, next/final, score isolation, stale input and reentry");
}
