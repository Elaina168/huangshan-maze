/* 独立进度联测，显式注入会话状态；实际模型/校准由核心测试覆盖。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "maze_progress.h"
static const maze_level_t level={.id="mechanism_trial"};
int main(void)
{
    maze_archive_t archive;maze_archive_default(&archive);
    maze_progress_t p;maze_progress_init(&p,&archive);
    maze_app_t a={.level=&level,.state=MAZE_APP_READY};
    maze_progress_before_event(&p,&a,MAZE_EVENT_START);
    assert(p.level==3 && p.active);
    a.model.checkpoint=1;a.state=MAZE_APP_LOST;a.elapsed_ms=123.25;
    maze_progress_observe(&p,&a);maze_progress_observe(&p,&a);
    assert(p.mistakes==1 && p.archive.levels[3].modes[0].losses==1 && p.accounted);
    maze_progress_before_event(&p,&a,MAZE_EVENT_RESPAWN);
    assert(!p.accounted);
    a.elapsed_ms=0;a.state=MAZE_APP_CALIBRATING;
    maze_progress_observe(&p,&a);
    assert(maze_progress_time_cs(&p,&a)==12 && p.mistakes==1);
    a.state=MAZE_APP_CALIBRATION_FAILED;maze_progress_observe(&p,&a);
    maze_progress_before_event(&p,&a,MAZE_EVENT_RETRY);
    a.state=MAZE_APP_PLAYING;a.elapsed_ms=20.75;
    assert(maze_progress_time_cs(&p,&a)==14);
    a.state=MAZE_APP_LOST;maze_progress_observe(&p,&a);maze_progress_observe(&p,&a);
    assert(p.mistakes==2 && p.archive.levels[3].modes[0].losses==2 && p.archive.revision==2);
    /* 无检查点的非法复活不解除已结算标志。 */
    a.model.checkpoint=0;maze_progress_before_event(&p,&a,MAZE_EVENT_RESPAWN);assert(p.accounted);
    a.model.checkpoint=1;maze_progress_before_event(&p,&a,MAZE_EVENT_RESPAWN);
    a.elapsed_ms=55.75;a.state=MAZE_APP_WON;maze_progress_observe(&p,&a);
    assert(p.finished && p.mistakes==2 && p.archive.levels[3].modes[0].count==1);
    assert(p.archive.levels[3].modes[0].scores[0].time_cs==19 && p.archive.levels[3].modes[0].scores[0].mistakes==2);
    uint64_t revision=p.archive.revision;
    maze_progress_observe(&p,&a);assert(p.archive.revision==revision);
    for(unsigned i=0;i<3;i++) assert(!p.archive.levels[i].completed && !p.archive.levels[i].modes[0].losses);
    maze_progress_before_event(&p,&a,MAZE_EVENT_REPLAY);
    assert(!p.finished && !p.accounted && !p.mistakes && p.accrued_ms==0);
    a.state=MAZE_APP_PAUSED;a.elapsed_ms=100;maze_progress_leave(&p,&a);
    assert(!p.active && p.archive.levels[3].modes[0].count==1);
    puts("PASS: fourth checkpoint respawn cumulative sub-ms/time/loss exactly once, invalid event, calibration retry, final rank and original record isolation");
}
