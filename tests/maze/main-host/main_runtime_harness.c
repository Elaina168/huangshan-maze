/* 直接编译正式main，替换外部I/O，验证实际启动迁移/重试与菜单消费顺序。 */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#ifndef BOARD_MAIN_SOURCE
#define BOARD_MAIN_SOURCE "../../../src/main.c"
#endif
#define main board_main
#include BOARD_MAIN_SOURCE
#undef main
static jmp_buf stop;
static unsigned saves,takes,phase,opens,loads;
static bool fail_first,fail_open;
static unsigned boundary;
static uint32_t clock_tick;
static maze_navigation_t *shown;
static void (*flush_service)(void *);
static void *service_context;
static bool sample_available;
static maze_imu_sample_t current_sample;
int rt_kprintf(const char *format,...) { (void)format;return 0; }
rt_tick_t rt_tick_get(void) { return clock_tick; }
rt_tick_t rt_tick_from_millisecond(int ms) { return (uint32_t)ms; }
void rt_thread_mdelay(int ms) { clock_tick+=(unsigned)ms; }
struct rt_device *rt_device_find(const char *name) { static struct rt_device d;(void)name;return &d; }
int rt_device_control(struct rt_device *d,int cmd,void *p) { (void)d;(void)cmd;(void)p;return 0; }
int maze_imu_init(void) { return 0; }
void maze_imu_request(uint32_t generation,bool enabled) { (void)generation;(void)enabled; }
bool maze_imu_latest(maze_imu_sample_t *out) { if(sample_available) *out=current_sample;return sample_available; }
int maze_ui_init(void) { return 0; }
void maze_ui_set_service(void (*f)(void *),void *p) { flush_service=f;service_context=p; }
void maze_ui_render(const maze_navigation_t *n) { shown=(maze_navigation_t *)n; }
void maze_ui_clear(void) {}
void maze_ui_loading(void) { assert(shown->page==MAZE_NAV_SELECT);loads++; }
void maze_ui_process(void)
{
    clock_tick+=20;
    if(boundary && shown->store_error==MAZE_STORE_STALE)
    {
        assert(shown->progress.revision_exhausted && shown->progress.archive.revision==UINT64_MAX);
        assert(saves==(boundary==2?1u:0u));longjmp(stop,1);
    }
    if(boundary && shown->page==MAZE_NAV_GAME && shown->app.state==MAZE_APP_CALIBRATING)
        shown->app.state=MAZE_APP_LOST;
    flush_service(service_context);
    assert(clock_tick<10000);
}
int maze_store_open(maze_store_t *s,maze_archive_t *a)
{
    opens++;memset(s,0,sizeof(*s));maze_archive_default(a);
    if(fail_open && opens==1) return MAZE_STORE_IO;
    s->revision=17;s->upgrade_pending=true;a->revision=18;
    a->levels[1].unlocked=a->levels[2].unlocked=a->levels[3].unlocked=true;
    if(boundary) { a->revision=UINT64_MAX;s->revision=UINT64_MAX-(boundary==2);s->upgrade_pending=boundary==2; }
    return 0;
}
int maze_store_save(maze_store_t *s,const maze_archive_t *a)
{
    assert(shown->page==MAZE_NAV_SELECT);
    if(boundary) { assert(boundary==2 && a->revision==UINT64_MAX && s->revision==UINT64_MAX-1);saves++;s->revision=UINT64_MAX;s->upgrade_pending=false;return 0; }
    assert(a->revision==18 && s->revision==17);
    saves++;
    if(fail_first && saves==1) return MAZE_STORE_IO;
    s->revision=18;s->upgrade_pending=false;return 0;
}
bool maze_ui_take_command(const maze_navigation_t *n,maze_ui_command_t *out)
{
    takes++;
    if(boundary)
    {
        if(phase++==0) { out->kind=MAZE_UI_SELECT;out->index=0;return true; }
        if(phase==2) { out->kind=MAZE_UI_GAME;out->game_event=MAZE_EVENT_START;return true; }
        return false;
    }
    if(n->page==MAZE_NAV_STORAGE_ERROR) { assert(fail_open && opens==1 && !saves);out->kind=MAZE_UI_SAVE;return true; }
    assert(saves>=1); /* 退回旧attempted_revision时，这里必须失败。 */
    if(fail_first && saves==1)
    {
        assert(n->store_error==MAZE_STORE_IO);out->kind=MAZE_UI_SAVE;return true;
    }
    assert(saves==(fail_first?2u:1u));
    switch(phase++)
    {
    case 0: out->kind=MAZE_UI_SCORES;break;
    case 1:case 2:case 3:case 4:case 5:
        assert(n->score_index==(phase-2)%4);out->kind=MAZE_UI_SCORE_LEVEL;break;
    case 6:out->kind=MAZE_UI_BACK;break;
    case 7:out->kind=MAZE_UI_SELECT;out->index=3;break;
    default:
        assert(n->page==MAZE_NAV_GAME && n->index==3 && n->app.state==MAZE_APP_READY);
        assert(loads==1 && opens==(fail_open?2u:1u));longjmp(stop,1);
    }
    return true;
}
static void terminal_callback(void)
{
    static maze_navigation_t n;maze_navigation_init(&n,1000);
    n.progress.archive.levels[1].unlocked=true;
    assert(maze_navigation_select(&n,1)==0);
    maze_progress_before_event(&n.progress,&n.app,MAZE_EVENT_START);
    maze_app_event(&n.app,MAZE_EVENT_START,clock_tick);
    for(unsigned i=0;i<20;i++)
    {
        current_sample=(maze_imu_sample_t){.generation=n.app.generation,.sequence=i+1,
            .read_start_tick=clock_tick,.raw={0,0,-16000}};
        maze_navigation_sample(&n,&current_sample,clock_tick);clock_tick+=50;
    }
    assert(n.app.state==MAZE_APP_PLAYING);
    n.app.model.x=n.app.level->goal.x;n.app.model.y=n.app.level->goal.y;
    uint64_t before=n.progress.archive.revision;
    sample_available=true;current_sample.sequence++;current_sample.read_start_tick=clock_tick;
    service_during_flush(&n);
    assert(n.app.state==MAZE_APP_WON && n.progress.archive.revision==before+1);
    assert(n.progress.archive.levels[2].unlocked && n.progress.archive.levels[1].modes[0].count==1);
    maze_navigation_update(&n,clock_tick);service_during_flush(&n);
    assert(n.progress.archive.revision==before+1 && n.progress.archive.levels[1].modes[0].count==1);
}
int main(void)
{
    for(unsigned mode=0;mode<3;mode++)
    {
        fail_first=mode==1;fail_open=mode==2;saves=takes=phase=opens=loads=clock_tick=0;
        if(!setjmp(stop)) board_main();
        assert(saves==(mode==1?2u:1u));
    }
    for(boundary=1;boundary<=2;boundary++)
    {
        fail_first=fail_open=false;saves=takes=phase=opens=loads=clock_tick=0;
        if(!setjmp(stop)) board_main();
    }
    terminal_callback();
    puts("PASS: direct v2 max and migrated max show storage error without new save/wrap; actual main migration precedes navigation, failed same-revision retry, four-score cycle, loading and callback WON/outer observe exactly once");
}
