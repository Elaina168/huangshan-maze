/* 实际试验主循环使用受控tick/输入/触摸；不链接任何存储实现。 */
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include "rtthread.h"
#include "maze_ui.h"
#include "maze_scroll_trial.h"
static jmp_buf done;
static uint32_t now,generation;static bool enabled;static unsigned stage;
static const maze_app_t *app;
static void (*flush_service)(void *);
static void *flush_context;
static bool unchunked,starved,recovery,injected,recovered,single;
static unsigned serviced;
void maze_ui_set_service(void (*service)(void *),void *context)
{ flush_service=service;flush_context=context; }
void rt_thread_mdelay(int ms) { now+=(uint32_t)ms; }
rt_tick_t rt_tick_get(void) { return now; }
rt_tick_t rt_tick_from_millisecond(int ms) { return ms; }
int rt_kprintf(const char *fmt,...) { (void)fmt;return 0; }
void maze_ui_process(void)
{
    if(stage==1 && app && app->state==MAZE_APP_PLAYING)
    {
        if(unchunked || (recovery && !injected)) { now+=128;injected=true; }
        else if(single) { now+=60;if(flush_service) { flush_service(flush_context);serviced++; } }
        else for(unsigned i=0;i<8;i++)
        {
            now+=16;
            if(flush_service) { flush_service(flush_context);serviced++; }
        }
    }
    else now+=20;
    assert(now<20000);
    if((unchunked || starved) && app && app->state==MAZE_APP_DEVICE_ERROR)
    {
        assert(app->error==(unchunked?MAZE_ERROR_TIMING_GAP:MAZE_ERROR_INPUT_STALE));
        longjmp(done,2);
    }
    if(stage==7 && app->state==MAZE_APP_READY) { assert(!enabled);longjmp(done,1); }
}
void maze_ui_render_trial(const maze_app_t *a) { app=a; }
void maze_imu_request(uint32_t gen,bool on) { generation=gen;enabled=on; }
bool maze_imu_latest(maze_imu_sample_t *s)
{
    if(!enabled || (starved && stage==1 && app->state==MAZE_APP_PLAYING)) return false;
    *s=(maze_imu_sample_t){.generation=generation,.sequence=now/50,
        .read_start_tick=now/50*50,.raw={0,0,-16000}};
    return true;
}
bool maze_ui_take_trial_event(const maze_app_t *a,maze_event_t *e)
{
    if(recovery && !recovered && a->state==MAZE_APP_DEVICE_ERROR)
    { assert(a->error==MAZE_ERROR_TIMING_GAP);*e=MAZE_EVENT_RETRY;recovered=true;return true; }
    if(recovery && recovered && stage==1 && a->state==MAZE_APP_PAUSED)
    { *e=MAZE_EVENT_RESUME;return true; }
    if(stage==0 && a->state==MAZE_APP_READY) { *e=MAZE_EVENT_START;stage++;return true; }
    if(stage==1 && a->state==MAZE_APP_PLAYING && a->elapsed_ms>=200) { *e=MAZE_EVENT_PAUSE;stage++;return true; }
    if(stage==2 && a->state==MAZE_APP_PAUSED) { *e=MAZE_EVENT_RESUME;stage++;return true; }
    if(stage==3 && a->state==MAZE_APP_PLAYING) { *e=MAZE_EVENT_PAUSE;stage++;return true; }
    if(stage==4 && a->state==MAZE_APP_PAUSED) { *e=MAZE_EVENT_REPLAY;stage++;return true; }
    if(stage==5 && a->state==MAZE_APP_CALIBRATING) { *e=MAZE_EVENT_EXIT;stage++;return true; }
    if(stage==6 && a->state==MAZE_APP_READY) { stage++;return false; }
    assert(a->state!=MAZE_APP_DEVICE_ERROR && a->state!=MAZE_APP_CALIBRATION_FAILED);
    return false;
}
int main(int argc,char **argv)
{
    unchunked=argc>1 && argv[1][0]=='u';starved=argc>1 && argv[1][0]=='s';recovery=argc>1 && argv[1][0]=='r';single=argc>1 && argv[1][0]=='b';
    printf("runner mode=%s\n",argc>1?argv[1]:"sliced");
    if(!setjmp(done)) { maze_scroll_trial_run();assert(0); }
    if(!unchunked && !starved) assert(serviced>(single?2u:8u));
    if(recovery) assert(injected && recovered);
    puts("PASS: selected display-service/input scenario; no store/progress linked");
}
