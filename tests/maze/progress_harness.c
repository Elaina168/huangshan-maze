/* 统计使用独立挑战身份，不依赖IMU代次或UI刷新次数。 */
#include <assert.h>
#include <stdio.h>
#include "maze_progress.h"
#include <string.h>
static const maze_level_t first={.id="frost_fork"},second={.id="ice_bends"};
static maze_progress_t p;
static maze_app_t a;
static void start(bool reversed)
{
    a=(maze_app_t){.level=&first,.reversed=reversed,.state=MAZE_APP_READY};
    maze_progress_before_event(&p,&a,MAZE_EVENT_START);a.state=MAZE_APP_PLAYING;
}
static void result(maze_app_state_t state,double ms)
{
    a.state=state;a.elapsed_ms=ms;maze_progress_observe(&p,&a);
}
int main(void)
{
    maze_archive_t archive;maze_archive_default(&archive);maze_progress_init(&p,&archive);
    assert(maze_progress_unlocked(&p,"frost_fork") && !maze_progress_unlocked(&p,"ice_bends"));
    start(false);
    result(MAZE_APP_LOST,15);maze_progress_observe(&p,&a);
    assert(p.mistakes==1 && p.archive.levels[0].modes[0].losses==1 && p.archive.revision==1);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.elapsed_ms=0;
    result(MAZE_APP_LOST,16);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.elapsed_ms=0;
    result(MAZE_APP_WON,19);maze_progress_observe(&p,&a);
    assert(p.mistakes==2 && maze_progress_time_cs(&p,&a)==5);
    assert(p.rank==1 && p.archive.levels[0].modes[0].scores[0].time_cs==5);
    assert(p.archive.levels[0].modes[0].scores[0].mistakes==2);
    assert(p.archive.levels[1].unlocked && p.archive.levels[0].completed==1);
    uint64_t revision=p.archive.revision;
    maze_progress_observe(&p,&a);assert(p.archive.revision==revision);
    puts("PASS: cumulative full-ms timing, loss exactly once, victory rank and shared unlock");

    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.state=MAZE_APP_PLAYING;a.elapsed_ms=120;
    a.state=MAZE_APP_DEVICE_ERROR;a.generation++;
    maze_progress_before_event(&p,&a,MAZE_EVENT_RETRY);
    a.state=MAZE_APP_PAUSED;a.generation++;maze_progress_observe(&p,&a);
    assert(p.mistakes==0 && maze_progress_time_cs(&p,&a)==12);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.elapsed_ms=0;
    result(MAZE_APP_WON,90);
    assert(maze_progress_time_cs(&p,&a)==21 && p.mistakes==0);
    start(true);result(MAZE_APP_WON,100);
    assert(p.archive.levels[0].completed==3 && p.archive.levels[0].modes[1].count==1);
    assert(p.archive.levels[0].modes[0].count==2);
    puts("PASS: pause replay accumulates, device recovery does not duplicate time/loss, mode isolation");

    for(unsigned i=0;i<7;i++) { start(false);result(MAZE_APP_WON,200+i*10); }
    assert(p.archive.levels[0].modes[0].count==5);
    assert(p.archive.levels[0].modes[0].scores[0].time_cs==5);
    assert(p.archive.levels[0].modes[0].scores[4].time_cs==22);
    start(false);a.state=MAZE_APP_PAUSED;a.elapsed_ms=55;
    maze_progress_leave(&p,&a);assert(!p.active && maze_progress_time_cs(&p,&a)==0);
    assert(p.archive.levels[0].modes[0].count==5);
    start(false);result(MAZE_APP_LOST,30);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.state=MAZE_APP_CALIBRATING;a.elapsed_ms=0;
    maze_progress_before_event(&p,&a,MAZE_EVENT_EXIT);a.state=MAZE_APP_READY;
    maze_progress_before_event(&p,&a,MAZE_EVENT_START);a.state=MAZE_APP_PLAYING;
    result(MAZE_APP_WON,20);assert(p.mistakes==1 && maze_progress_time_cs(&p,&a)==5);
    assert(p.rank==1 && p.archive.levels[0].modes[0].scores[1].mistakes==2);
    puts("PASS: top-5 truncation, unfinished exit excluded, calibration-cancel cannot discard prior challenge");
    start(false);result(MAZE_APP_LOST,30);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.elapsed_ms=0;
    result(MAZE_APP_WON,20);assert(p.rank==2); /* 完全同分的新纪录排在已有同分之后。 */
    maze_progress_leave(&p,&a);
    unsigned first_count=p.archive.levels[0].modes[1].count;
    uint32_t first_losses=p.archive.levels[0].modes[1].losses;
    a=(maze_app_t){.level=&second,.reversed=true,.state=MAZE_APP_READY};
    maze_progress_before_event(&p,&a,MAZE_EVENT_START);a.state=MAZE_APP_PLAYING;
    result(MAZE_APP_LOST,30);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);a.elapsed_ms=0;
    result(MAZE_APP_WON,70);
    assert(p.level==1 && p.rank==1 && p.archive.levels[1].completed==2);
    assert(p.archive.levels[1].modes[1].scores[0].time_cs==10);
    assert(p.archive.levels[1].modes[1].losses==1 && p.archive.levels[1].modes[0].count==0);
    assert(p.archive.levels[0].modes[1].count==first_count && p.archive.levels[0].modes[1].losses==first_losses);
    puts("PASS: exact-tie stable rank, second-level score/loss isolation and final-level completion boundary");
    uint8_t bytes[MAZE_ARCHIVE_BYTES];maze_archive_t decoded;
    assert(maze_archive_encode(&p.archive,bytes)==0 && maze_archive_decode(bytes,sizeof(bytes),&decoded)==0);
    puts("PASS: progress archive fits actual persistent format");
    /* 最大修订不能接受不可表示的新结算，也不能回绕并覆盖旧档。 */
    for(unsigned outcome=0;outcome<2;outcome++)
    {
        maze_archive_default(&decoded);decoded.revision=UINT64_MAX;
        maze_progress_init(&p,&decoded);start(false);
        maze_archive_encode(&p.archive,bytes);
        result(outcome?MAZE_APP_WON:MAZE_APP_LOST,100);
        uint8_t unchanged[MAZE_ARCHIVE_BYTES];maze_archive_encode(&p.archive,unchanged);
        assert(!memcmp(bytes,unchanged,sizeof(bytes)));
        assert(p.revision_exhausted && p.archive.revision==UINT64_MAX);
        result(outcome?MAZE_APP_WON:MAZE_APP_LOST,100);
        assert(p.archive.revision==UINT64_MAX);
    }
    puts("PASS: maximum revision rejects new win/loss explicitly, archive unchanged and never wraps");
    return 0;
}
