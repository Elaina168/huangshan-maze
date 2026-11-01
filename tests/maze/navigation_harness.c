/* 注入关卡目录测试导航，不把测试地图当作正式第二关。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "maze_navigation.h"

static const char *const lines[]={"+-+-+","|. .|","+-+-+"};
static const maze_map_object_t objects[]={
    {MAZE_MAP_START,{25,25,7}}, {MAZE_MAP_GOAL,{75,25,10}}
};
static const maze_map_definition_t first={"frost_fork",2,1,lines,3,objects,2};
static const maze_map_definition_t second={"ice_bends",2,1,lines,3,objects,2};
static const maze_map_definition_t broken={"frost_fork",0,1,lines,3,objects,2};
const maze_catalog_entry_t maze_catalog[]={
    {"测试一",&first},{"测试二",&second},{"非法图",&broken},{"末关",&second}
};
const unsigned maze_catalog_count=4;
static maze_navigation_t n;

static void sample(maze_navigation_t *nav, unsigned seq, unsigned tick, int x, int y)
{
    maze_imu_sample_t s={.generation=nav->app.generation,.sequence=seq,
        .read_start_tick=tick,.read_gap_tick=50,.raw={x,y,-16000}};
    maze_navigation_sample(nav,&s,tick);
    maze_navigation_update(nav,tick);
}

static void calibrate(maze_navigation_t *nav, unsigned start)
{
    maze_app_event(&nav->app,MAZE_EVENT_START,start);
    for(unsigned i=0;i<20;i++) sample(nav,i+1,start+i*50,70,-320);
    assert(nav->app.state==MAZE_APP_PLAYING && nav->app.has_reference);
}

int main(void)
{
    maze_navigation_init(&n,1000);
    assert(n.page==MAZE_NAV_SELECT && !n.reversed && !n.app_initialized);
    assert(maze_navigation_can_save(&n));
    assert(maze_navigation_select(&n,1)==MAZE_NAV_LOCKED && n.page==MAZE_NAV_SELECT);
    n.progress.archive.levels[1].unlocked=true; /* 后续测试沿用明确已解锁档案。 */
    uint32_t generation; bool enabled;
    maze_navigation_request(&n,&generation,&enabled);
    assert(!enabled && generation==0);
    assert(maze_navigation_select(&n,4)==MAZE_NAV_BAD_INDEX);
    assert(n.page==MAZE_NAV_SELECT);
    assert(maze_navigation_select(&n,0)==0);
    assert(n.page==MAZE_NAV_GAME && n.app.state==MAZE_APP_READY);
    assert(!maze_navigation_can_save(&n));
    for(int state=MAZE_APP_READY;state<=MAZE_APP_LOST;state++)
    {
        n.app.state=(maze_app_state_t)state;
        assert(maze_navigation_can_save(&n)==(state==MAZE_APP_WON || state==MAZE_APP_LOST));
    }
    n.page=MAZE_NAV_SCORES;assert(!maze_navigation_can_save(&n));
    n.page=MAZE_NAV_GAME;n.app.state=MAZE_APP_READY;
    assert(!strcmp(n.app.level->id,"frost_fork"));
    calibrate(&n,100);
    assert(!maze_navigation_can_save(&n));
    sample(&n,21,1100,-930,1680);
    assert(n.app.input.x==1700 && n.app.input.y==700);
    sample(&n,22,1150,370,-620);
    assert(n.app.input.x==0 && n.app.input.y==0);
    sample(&n,23,1200,371,-621);
    assert(n.app.input.x==-1 && n.app.input.y==-1);
    maze_app_event(&n.app,MAZE_EVENT_PAUSE,1200);
    sample(&n,24,1250,-930,1680);
    assert(n.app.input.x==1700 && n.app.input.y==700);
    uint32_t old_gen=n.app.generation,old_version=n.page_version;
    n.app.model.vx=25; n.app.model.vy=-30; n.app.elapsed_ms=500;
    maze_navigation_toggle_direction(&n);
    assert(!n.reversed);
    maze_navigation_back(&n);
    assert(n.page==MAZE_NAV_SELECT && n.page_version!=old_version);
    assert(n.app.generation>old_gen && !n.app.has_reference);
    assert(n.app.elapsed_ms==0 && n.app.model.vx==0 && n.app.model.vy==0);
    assert(n.app.reference[0]==0 && n.app.reference[1]==0 && n.app.reference[2]==0);
    maze_navigation_request(&n,&generation,&enabled); assert(!enabled);
    maze_imu_sample_t old={.generation=old_gen,.sequence=25,.read_start_tick=1300};
    maze_navigation_sample(&n,&old,1300); maze_navigation_update(&n,9000);
    assert(n.app.state==MAZE_APP_READY);
    maze_navigation_toggle_direction(&n); assert(n.reversed);
    assert(maze_navigation_select(&n,1)==0);
    assert(n.app.reversed && !strcmp(n.app.level->id,"ice_bends"));
    assert(n.app.reference[0]==0 && n.app.reference[1]==0 && n.app.reference[2]==0);
    assert(n.app.model.x==25 && n.app.model.y==25);
    maze_app_event(&n.app,MAZE_EVENT_START,2000);
    maze_navigation_sample(&n,&old,2000);
    assert(n.app.window_count==0);
    for(unsigned i=0;i<20;i++) sample(&n,i+1,2000+i*50,70,-320);
    assert(n.app.state==MAZE_APP_PLAYING);
    assert(n.app.reference[0]==70 && n.app.reference[1]==-320);
    sample(&n,21,3000,-930,1680);
    assert(n.app.input.x==-1700 && n.app.input.y==-700);
    sample(&n,22,3050,371,-621);
    assert(n.app.input.x==1 && n.app.input.y==1);
    maze_app_event(&n.app,MAZE_EVENT_PAUSE,3050);
    sample(&n,23,3100,-930,1680);
    assert(n.app.input.x==-1700 && n.app.input.y==-700);
    maze_app_event(&n.app,MAZE_EVENT_REPLAY,3100);
    assert(n.app.reversed && n.app.state==MAZE_APP_CALIBRATING);
    puts("PASS: normal/reverse, identical raw calibration, dead-zone edge, paused input and replay");

    maze_navigation_back(&n); assert(maze_navigation_select(&n,0)==0);
    old_gen=n.app.generation;
    n.app.state=MAZE_APP_WON;
    assert(maze_navigation_next(&n)==0 && n.index==1);
    assert(n.app.generation>old_gen && n.app.state==MAZE_APP_READY && n.app.reversed);
    old_gen=n.app.generation;
    maze_navigation_back(&n); assert(maze_navigation_select(&n,1)==0);
    assert(n.app.generation>old_gen);
    assert(maze_navigation_next(&n)==0 && n.index==1); /* 未胜利不切关。 */
    n.app.state=MAZE_APP_WON;
    assert(maze_navigation_next(&n)==MAZE_MAP_FORMAT && n.page==MAZE_NAV_MAP_ERROR);
    maze_navigation_back(&n); /* 旧图已被失败装载覆写，不得再对旧app执行重置。 */
    assert(maze_navigation_select(&n,1)==0);
    maze_navigation_back(&n); assert(maze_navigation_select(&n,2)==MAZE_MAP_FORMAT);
    assert(n.page==MAZE_NAV_MAP_ERROR && n.error==MAZE_MAP_FORMAT);
    maze_navigation_request(&n,&generation,&enabled); assert(!enabled);
    old_gen=n.app.generation;
    maze_navigation_sample(&n,&old,5000); maze_navigation_update(&n,10000);
    assert(n.app.generation==old_gen && n.app.state==MAZE_APP_READY);
    maze_navigation_back(&n); assert(maze_navigation_select(&n,0)==0);
    assert(n.error==0 && n.app.level==&n.map.level && n.app.reversed);
    puts("PASS: switch/reselect generation, reset, stale sample rejection and failed-load recovery");

    maze_navigation_back(&n); assert(maze_navigation_select(&n,3)==0);
    n.app.state=MAZE_APP_WON;
    old_gen=n.app.generation;
    assert(maze_navigation_next(&n)==0 && n.index==3 && n.app.generation==old_gen);
    puts("PASS: final level has no next transition");

    maze_navigation_init(&n,1000);
    assert(!n.reversed && n.page==MAZE_NAV_SELECT);
    assert(maze_navigation_select(&n,2)==MAZE_MAP_FORMAT && !n.app_initialized);
    maze_navigation_request(&n,&generation,&enabled); assert(!enabled && generation==0);
    maze_navigation_back(&n); assert(maze_navigation_select(&n,0)==0);
    assert(n.app_initialized && !n.app.reversed);
    puts("PASS: first-load failure, default mode and initialization recovery");
    return 0;
}
