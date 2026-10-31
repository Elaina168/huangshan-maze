/* 状态、时基与异步代次联合测试，运行真实app和model。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "maze_app.h"

static const maze_level_t open_level={.width=4000,.height=4000,.ball_radius=5,
    .start={10,50},.goal={3900,3900,3}};

static void near(double actual, double expected)
{
    if (fabs(actual-expected)>0.003) fprintf(stderr,"actual=%g expected=%g\n",actual,expected);
    assert(fabs(actual-expected)<=0.003);
}

static void sample(maze_app_t *a, uint32_t tick, int raw_x, int raw_y, int error)
{
    maze_imu_sample_t s={.generation=a->generation,.sequence=a->last_sequence+1,
        .read_start_tick=tick,.read_gap_tick=a->has_read?tick-a->last_read_tick:0,
        .raw={raw_x,raw_y,-16000},.error=error};
    maze_app_sample(a,&s,tick);
}

static uint32_t finish_calibration(maze_app_t *a, uint32_t start, int ref_x, int ref_y)
{
    for (unsigned i=0;i<20;i++) sample(a,start+i*a->tick_hz/20,ref_x,ref_y,0);
    return start+19*a->tick_hz/20;
}

static uint32_t start_game(maze_app_t *a, uint32_t start, uint32_t hz)
{
    maze_app_init(a,&open_level,hz);
    maze_app_event(a,MAZE_EVENT_START,start);
    uint32_t now=finish_calibration(a,start,0,0);
    assert(a->state==MAZE_APP_PLAYING);
    near(a->elapsed_ms,0); near(a->model.x,10);
    return now;
}

static void move(maze_app_t *a, uint32_t tick)
{
    sample(a,tick,0,8492,0);
    maze_app_update(a,tick);
}

static void pause_resume(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    move(&a,now+50); near(a.elapsed_ms,50); assert(a.model.x>10 && a.model.x<16);
    maze_app_event(&a,MAZE_EVENT_PAUSE,now+55);
    assert(a.state==MAZE_APP_PAUSED); near(a.elapsed_ms,55); near(a.model.vx,0);
    float x=a.model.x;
    maze_app_update(&a,now+10055); near(a.elapsed_ms,55); near(a.model.x,x);
    /* 10秒暂停后的新样本，下一条连续输入使继续有效。 */
    sample(&a,now+10055,0,8492,0); sample(&a,now+10105,0,8492,0);
    maze_app_event(&a,MAZE_EVENT_RESUME,now+10105);
    assert(a.state==MAZE_APP_PLAYING);
    move(&a,now+10110); near(a.model.x,x); near(a.elapsed_ms,60);
    move(&a,now+10115); near(a.model.x,x+0.0582524); near(a.elapsed_ms,65);
    uint32_t gen=a.generation;
    maze_app_event(&a,MAZE_EVENT_START,now+10115);
    maze_app_event(&a,MAZE_EVENT_RESUME,now+10115);
    maze_app_event(&a,MAZE_EVENT_REPLAY,now+10115);
    assert(a.state==MAZE_APP_PLAYING && a.generation==gen);
    puts("PASS: calibration starts time, 10s pause, no catch-up and invalid events");
}

static void replay_cancel(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    move(&a,now+5);
    maze_app_event(&a,MAZE_EVENT_PAUSE,now+5);
    uint32_t old=a.generation;
    maze_app_event(&a,MAZE_EVENT_REPLAY,now+5);
    assert(a.state==MAZE_APP_CALIBRATING && a.generation!=old);
    near(a.model.x,10); near(a.elapsed_ms,0);
    uint32_t ready=finish_calibration(&a,now+5,0,0);
    move(&a,ready+5); near(a.model.x,10);
    move(&a,ready+10); near(a.model.x,10.0582524);
    maze_app_event(&a,MAZE_EVENT_PAUSE,ready+10);
    uint32_t prior=a.generation;
    maze_app_event(&a,MAZE_EVENT_EXIT,ready+10);
    assert(a.state==MAZE_APP_READY && a.generation!=prior && !a.has_reference);
    maze_imu_sample_t late={.generation=prior,.sequence=99,.error=-8};
    maze_app_sample(&a,&late,ready+11);
    assert(a.state==MAZE_APP_READY);
    maze_app_event(&a,MAZE_EVENT_START,ready+12);
    late.error=0; maze_app_sample(&a,&late,ready+13);
    assert(a.state==MAZE_APP_CALIBRATING && a.window_count==0);
    maze_app_event(&a,MAZE_EVENT_EXIT,ready+14);
    assert(a.state==MAZE_APP_READY);
    bool enabled; uint32_t gen;
    maze_app_request(&a,&gen,&enabled); assert(!enabled && gen==a.generation);
    puts("PASS: replay/exit clear partial steps, new generations and late result isolation");
}

static void recalibration(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    move(&a,now+50); maze_app_event(&a,MAZE_EVENT_PAUSE,now+50);
    float x=a.model.x; double elapsed=a.elapsed_ms;
    maze_app_event(&a,MAZE_EVENT_RECALIBRATE,now+50);
    maze_app_update(&a,now+3050);
    assert(a.state==MAZE_APP_PAUSED && a.error==MAZE_ERROR_CALIBRATION_TIMEOUT);
    assert(a.reference[0]==0 && a.reference[1]==0);
    near(a.model.x,x); near(a.elapsed_ms,elapsed);
    maze_app_event(&a,MAZE_EVENT_RECALIBRATE,now+3050);
    finish_calibration(&a,now+3050,123,456);
    assert(a.state==MAZE_APP_PAUSED && a.reference[0]==123 && a.reference[1]==456);
    near(a.model.x,x); near(a.elapsed_ms,elapsed);
    maze_app_event(&a,MAZE_EVENT_RESUME,now+4101);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==MAZE_ERROR_INPUT_STALE);
    puts("PASS: paused recalibration success/failure preserve round and stale resume fails");
}

static void recovery(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    move(&a,now+50); float x=a.model.x; double elapsed=a.elapsed_ms;
    sample(&a,now+60,0,0,-8);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==-8);
    near(a.model.vx,0); near(a.model.x,x); near(a.elapsed_ms,elapsed);
    bool enabled; uint32_t gen; maze_app_request(&a,&gen,&enabled); assert(!enabled);
    maze_app_event(&a,MAZE_EVENT_RETRY,now+60); sample(&a,now+60,0,0,-9);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==-9);
    maze_app_event(&a,MAZE_EVENT_RETRY,now+70);
    uint32_t ready=finish_calibration(&a,now+70,10,20);
    assert(a.state==MAZE_APP_PAUSED);
    near(a.model.x,x); near(a.elapsed_ms,elapsed);
    maze_app_request(&a,&gen,&enabled); assert(enabled);
    maze_app_event(&a,MAZE_EVENT_RESUME,ready); assert(a.state==MAZE_APP_PLAYING);
    maze_app_event(&a,MAZE_EVENT_PAUSE,ready);
    sample(&a,ready+10,0,0,-7);
    assert(a.state==MAZE_APP_PAUSED);
    maze_app_event(&a,MAZE_EVENT_RESUME,ready+10);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==-7);
    puts("PASS: I/O freezes round, recovery retries pause, paused read error checked on resume");
}

static void timing(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    move(&a,now+100); assert(a.model.x>10 && a.model.x<22); near(a.elapsed_ms,100);
    float before_error=a.model.x;
    near(a.model.step_elapsed_ms,10); assert(a.motion_units==0);
    sample(&a,now+150,0,8492,0);
    move(&a,now+201);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==MAZE_ERROR_TIMING_GAP);
    near(a.model.x,before_error); near(a.elapsed_ms,100);
    now=start_game(&a,UINT32_MAX-500,1000);
    move(&a,now+5); near(a.model.x,10); near(a.elapsed_ms,5);
    move(&a,now+10); near(a.model.x,10.0582524); near(a.elapsed_ms,10);
    now=start_game(&a,0,1280);
    for(unsigned i=1;i<=16;i++) move(&a,now+i);
    near(a.elapsed_ms,12.5); near(a.model.x,10.0582524);
    puts("PASS: 100/101ms scheduler boundary, partial step, tick wrap and 1280Hz fraction");
}

static void terminal(void)
{
    for(unsigned lost=0;lost<2;lost++)
    {
        maze_circle_t trap={11.03f,50,1};
        maze_level_t l=open_level;
        if(lost) { l.traps=&trap; l.trap_count=1; }
        else l.goal=trap;
        maze_app_t a; maze_app_init(&a,&l,1000);
        maze_app_event(&a,MAZE_EVENT_START,0); finish_calibration(&a,0,0,0);
        move(&a,955); near(a.elapsed_ms,5);
        move(&a,960);
        assert(a.state==(lost?MAZE_APP_LOST:MAZE_APP_WON));
        near(a.elapsed_ms,5.15); near(a.model.x,10.03);
        double elapsed=a.elapsed_ms; float x=a.model.x;
        sample(&a,1000,0,8492,-8); maze_app_update(&a,100000);
        near(a.elapsed_ms,elapsed); near(a.model.x,x);
        bool enabled; uint32_t gen; maze_app_request(&a,&gen,&enabled); assert(!enabled);
        maze_app_event(&a,MAZE_EVENT_REPLAY,100000);
        assert(a.state==MAZE_APP_CALIBRATING); near(a.elapsed_ms,0); near(a.model.x,10);
    }
    puts("PASS: fractional-step win/loss time, single settlement, terminal freeze and replay");
}

static void scheduler_independence(void)
{
    maze_app_t a[2];
    for (unsigned run=0;run<2;run++)
    {
        uint32_t now=start_game(&a[run],0,1000);
        sample(&a[run],now,0,8492,0);
        unsigned period=run?25:50;
        for (unsigned ms=period;ms<=10000;ms+=period)
        {
            if (ms%50==0) sample(&a[run],now+ms,0,8492,0);
            maze_app_update(&a[run],now+ms);
            assert(a[run].state==MAZE_APP_PLAYING);
        }
    }
    near(a[0].elapsed_ms,10000); near(a[1].elapsed_ms,10000);
    near(a[0].model.x,a[1].model.x);
    assert(a[0].model.x>10+MAZE_SPEED_MAX*9 && a[0].model.x<10+MAZE_SPEED_MAX*10);
    puts("PASS: same 50ms samples, 20/40Hz app update, fixed 10ms motion (host only)");
}

static void skipped_snapshot(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    maze_app_update(&a,now+50);
    /* 后台每51ms发布，UI只取最新快照；本次update距前次仅52ms。 */
    maze_imu_sample_t latest={.generation=a.generation,.sequence=a.last_sequence+2,
        .read_start_tick=now+102,.read_gap_tick=51,.raw={0,8492,-16000}};
    maze_app_sample(&a,&latest,now+102);
    assert(a.state==MAZE_APP_PLAYING && a.error==0);
    maze_app_update(&a,now+102);
    assert(a.state==MAZE_APP_PLAYING);
    puts("PASS: skipped latest snapshots do not invent a sensor read gap");
}

static void hidden_read_gap(void)
{
    maze_app_t a; uint32_t now=start_game(&a,0,1000);
    maze_app_update(&a,now+50);
    maze_app_update(&a,now+100);
    /* 生产者曾间隔101ms，随后间隔50ms；UI只取得最后一条。 */
    maze_imu_sample_t latest={.generation=a.generation,.sequence=a.last_sequence+2,
        .read_start_tick=now+151,.read_gap_tick=50,.read_gap_count=1,.raw={0,8492,-16000}};
    maze_app_sample(&a,&latest,now+152);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==MAZE_ERROR_INPUT_GAP);
    maze_app_begin(&a,2000);
    for (unsigned i=0;i<20;i++)
    {
        maze_imu_sample_t stable={.generation=a.generation,.sequence=i+3,
            .read_start_tick=2152+i*50,.read_gap_tick=50,.read_gap_count=1,.raw={0,0,-16000}};
        maze_app_sample(&a,&stable,stable.read_start_tick);
    }
    assert(a.state==MAZE_APP_PLAYING && a.window_count==20);
    puts("PASS: real producer gap cannot disappear behind a newer healthy snapshot");
}

int main(void)
{
    setbuf(stdout,NULL);
    pause_resume(); replay_cancel(); recalibration(); recovery(); timing(); terminal(); scheduler_independence(); skipped_snapshot(); hidden_read_gap();
    return 0;
}
