/* 对真实纯C校准/输入模块注入带时间戳的样本，不替代硬件。 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "maze_app.h"

static const maze_level_t test_level={.width=4000,.height=4000,.ball_radius=5,
    .start={10,50},.goal={3900,3900,3}};

static void point(maze_app_t *a, uint32_t seq, uint32_t read_tick, uint32_t now,
                  int x, int y, int z)
{
    maze_imu_sample_t s = {.generation=a->generation, .sequence=seq,
        .read_start_tick=read_tick, .read_gap_tick=a->has_read?read_tick-a->last_read_tick:0, .raw={x,y,z}};
    maze_app_sample(a,&s,now);
}
static void calibrate(maze_app_t *a, uint32_t start, uint32_t hz)
{
    maze_app_init(a,&test_level,hz);
    maze_app_begin(a,start);
    for (unsigned i=0;i<20;i++) point(a,i+1,start+i*hz/20,start+i*hz/20,0,0,-16000);
    assert(a->state==MAZE_APP_PLAYING && a->has_reference);
}
int main(void)
{
    maze_app_t a;
    maze_app_init(&a,&test_level,1000); maze_app_begin(&a,0);
    for(unsigned i=0;i<19;i++) point(&a,i+1,i*50,i*50,i==0?300:0,0,-16000);
    assert(a.state==MAZE_APP_CALIBRATING && a.window_count==19);
    point(&a,19,900,900,0,0,-16000);
    assert(a.window_count==19);
    point(&a,20,950,950,0,0,-16000);
    assert(a.state==MAZE_APP_PLAYING && a.reference[0]==15);
    puts("PASS: 19/20 samples, duplicate sequence, 950ms and range300 boundary");

    maze_app_init(&a,&test_level,1000); maze_app_begin(&a,0);
    for(unsigned i=0;i<20;i++) point(&a,i+1,i*50,i*50,i%2?301:0,0,-16000);
    assert(a.state==MAZE_APP_CALIBRATING);
    maze_app_update(&a,3000);
    assert(a.state==MAZE_APP_CALIBRATION_FAILED && a.error==MAZE_ERROR_CALIBRATION_TIMEOUT);
    puts("PASS: range301 rejected and 3000ms timeout without reference commit");

    calibrate(&a,0,1000);
    maze_app_update(&a,1000);
    maze_app_update(&a,1050);
    point(&a,21,1000,1100,-301,-301,-16000);
    assert(a.state==MAZE_APP_PLAYING && a.input.x==-1 && a.input.y==1);
    point(&a,22,1050,1100,300,300,-16000);
    assert(a.input.x==0 && a.input.y==0);
    point(&a,23,1100,1100,301,301,-16000);
    assert(a.input.x==1 && a.input.y==-1);
    maze_app_update(&a,1100);
    maze_app_update(&a,1200);
    assert(a.state==MAZE_APP_PLAYING);
    maze_app_update(&a,1201);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.input.x==0 && a.input.y==0);
    puts("PASS: USB-right signs, dead zone300 and age100/101 boundary");

    calibrate(&a,0,1000); point(&a,21,1050,1050,1000,0,-16000);
    assert(a.state==MAZE_APP_PLAYING);
    point(&a,22,1151,1151,1000,0,-16000);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==MAZE_ERROR_INPUT_GAP);
    calibrate(&a,0,1000); point(&a,21,1000,1101,0,0,-16000);
    assert(a.state==MAZE_APP_DEVICE_ERROR && a.error==MAZE_ERROR_INPUT_STALE);
    puts("PASS: read gap100/101 and late sample rejection");

    maze_app_init(&a,&test_level,1000); maze_app_begin(&a,0);
    for(unsigned i=0;i<5;i++) point(&a,i+1,i*50,i*50,0,0,-16000);
    point(&a,7,250,250,0,0,-16000); assert(a.window_count==1);
    point(&a,8,351,351,0,0,-16000); assert(a.window_count==1);
    point(&a,9,400,501,0,0,-16000); assert(a.window_count==0);
    maze_imu_sample_t old={.generation=a.generation-1,.sequence=10,.error=-8};
    maze_app_sample(&a,&old,502); assert(a.state==MAZE_APP_CALIBRATING);
    maze_app_update(&a,3000); assert(a.state==MAZE_APP_CALIBRATION_FAILED);
    puts("PASS: missing sequence/gap/stale reset window, old generation ignored");

    calibrate(&a,UINT32_MAX-500,1000);
    point(&a,21,499,499,-301,-301,-16000);
    assert(a.input.x==-1 && a.input.y==1);
    calibrate(&a,0,100);
    point(&a,21,100,110,-301,-301,-16000);
    assert(a.input.x==-1 && a.input.y==1);
    maze_app_update(&a,111); assert(a.state==MAZE_APP_DEVICE_ERROR);
    puts("PASS: tick wrap and 100Hz tick conversion");

    calibrate(&a,0,1000); maze_app_begin(&a,2000);
    maze_app_update(&a,5000);
    assert(a.has_reference && a.reference[2]==-16000);
    maze_app_begin(&a,6000);
    maze_imu_sample_t bad={.generation=a.generation,.sequence=1,.error=-8};
    maze_app_sample(&a,&bad,6000);
    assert(a.state==MAZE_APP_CALIBRATION_FAILED && a.error==-8);
    uint32_t old_gen=a.generation; maze_app_cancel(&a);
    assert(a.generation!=old_gen && a.state==MAZE_APP_READY && !a.has_reference);
    maze_app_sample(&a,&bad,6001); assert(a.state==MAZE_APP_READY);
    puts("PASS: timeout preserves reference, I/O failure and cancel invalidation");

    maze_app_init(&a,&test_level,1000); maze_app_begin(&a,0);
    for(unsigned i=0;i<20;i++) point(&a,i+1,i*49,i*49,0,0,-16000);
    assert(a.state==MAZE_APP_CALIBRATING && a.window_count==20);
    point(&a,21,1000,1000,0,0,-16000);
    assert(a.state==MAZE_APP_PLAYING);
    maze_app_begin(&a,2000);
    for(unsigned i=0;i<19;i++) point(&a,i+1,2050+i*50,2050+i*50,0,0,-16000);
    point(&a,20,3000,5000,0,0,-16000);
    assert(a.state==MAZE_APP_CALIBRATION_FAILED && a.error==MAZE_ERROR_CALIBRATION_TIMEOUT);
    puts("PASS: short span not accepted, sliding window succeeds, late result cannot beat deadline");
    return 0;
}
