/* 纯C状态、校准和运动时基：无I2C、线程或LVGL依赖。 */
#include "maze_app.h"
#include <string.h>

static bool exceeds(maze_app_t *a, uint32_t ticks, unsigned ms)
{
    return (uint64_t)ticks * 1000 > (uint64_t)ms * a->tick_hz;
}

static void fail(maze_app_t *a, int error)
{
    if (a->state == MAZE_APP_CALIBRATING)
        a->state = a->calibration_origin == MAZE_CAL_NEW ? MAZE_APP_CALIBRATION_FAILED :
            a->calibration_origin == MAZE_CAL_PAUSED ? MAZE_APP_PAUSED : MAZE_APP_DEVICE_ERROR;
    else a->state = MAZE_APP_DEVICE_ERROR;
    a->error = error;
    a->input = (maze_input_t){0,0};
    a->motion_units = 0;
    a->model.vx = a->model.vy = 0;
}

void maze_app_init(maze_app_t *a, const maze_level_t *level, uint32_t tick_hz)
{
    memset(a,0,sizeof(*a));
    a->level = level;
    a->tick_hz = tick_hz;
    a->state = MAZE_APP_READY;
    maze_model_reset(&a->model,level);
}

static void calibrate(maze_app_t *a, uint32_t now_tick, maze_calibration_origin_t origin)
{
    a->generation++;
    a->state = MAZE_APP_CALIBRATING;
    a->started_tick = now_tick;
    a->window_count = 0;
    a->last_gap_count = 0;
    a->has_sequence = a->has_read = false;
    a->error = 0;
    a->input = (maze_input_t){0,0};
    a->calibration_origin = origin;
    a->last_update_tick = now_tick;
    a->motion_units = 0;
    a->model.vx = a->model.vy = 0;
}

void maze_app_begin(maze_app_t *a, uint32_t now_tick)
{
    maze_model_reset(&a->model,a->level);
    a->elapsed_ms = 0;
    calibrate(a,now_tick,MAZE_CAL_NEW);
}

void maze_app_cancel(maze_app_t *a)
{
    a->generation++;
    a->state = MAZE_APP_READY;
    a->window_count = 0;
    a->has_reference = a->has_read = a->has_sequence = false;
    a->error = 0;
    a->input = (maze_input_t){0,0};
    a->motion_units = 0;
    a->elapsed_ms = 0;
    maze_model_reset(&a->model,a->level);
}

void maze_app_update(maze_app_t *a, uint32_t now_tick)
{
    if (a->state == MAZE_APP_CALIBRATING &&
        (uint64_t)(uint32_t)(now_tick-a->started_tick)*1000 >=
        (uint64_t)MAZE_CALIBRATION_BUDGET_MS*a->tick_hz)
        fail(a,MAZE_ERROR_CALIBRATION_TIMEOUT);
    else if (a->state == MAZE_APP_PLAYING)
    {
        if (exceeds(a,now_tick-a->last_read_tick,MAZE_INPUT_MAX_AGE_MS))
        { fail(a,MAZE_ERROR_INPUT_STALE); return; }
        uint32_t delta=now_tick-a->last_update_tick;
        if (exceeds(a,delta,MAZE_UPDATE_MAX_GAP_MS))
        { fail(a,MAZE_ERROR_TIMING_GAP); return; }
        a->elapsed_ms += (double)delta*1000/a->tick_hz;
        a->motion_units += (uint64_t)delta*1000;
        uint64_t step=(uint64_t)MAZE_PHYSICS_STEP_MS*a->tick_hz;
        while (a->motion_units>=step)
        {
            maze_model_step(&a->model,a->input.x,a->input.y,MAZE_PHYSICS_STEP_MS);
            if (a->model.result!=MAZE_MODEL_RUNNING)
            {
                a->elapsed_ms = a->elapsed_ms-(double)a->motion_units/a->tick_hz+a->model.step_elapsed_ms;
                a->motion_units=0;
                a->state=a->model.result==MAZE_MODEL_WON?MAZE_APP_WON:MAZE_APP_LOST;
                a->input=(maze_input_t){0,0};
                break;
            }
            a->motion_units-=step;
        }
    }
    a->last_update_tick=now_tick;
}

void maze_app_request(const maze_app_t *a, uint32_t *generation, bool *enabled)
{
    *generation=a->generation;
    *enabled=a->state==MAZE_APP_CALIBRATING || a->state==MAZE_APP_PLAYING || a->state==MAZE_APP_PAUSED;
}

void maze_app_event(maze_app_t *a, maze_event_t event, uint32_t now_tick)
{
    switch (event)
    {
    case MAZE_EVENT_START:
        if (a->state==MAZE_APP_READY) maze_app_begin(a,now_tick);
        break;
    case MAZE_EVENT_PAUSE:
        if (a->state!=MAZE_APP_PLAYING) break;
        maze_app_update(a,now_tick);
        if (a->state==MAZE_APP_PLAYING)
        {
            a->state=MAZE_APP_PAUSED;
            a->motion_units=0;
            a->model.vx=a->model.vy=0;
        }
        break;
    case MAZE_EVENT_RESUME:
        if (a->state!=MAZE_APP_PAUSED) break;
        if (!a->has_read || exceeds(a,now_tick-a->last_read_tick,MAZE_INPUT_MAX_AGE_MS))
            fail(a,MAZE_ERROR_INPUT_STALE);
        else if (a->error) fail(a,a->error);
        else
        {
            a->state=MAZE_APP_PLAYING;
            a->last_update_tick=now_tick;
            a->motion_units=0;
        }
        break;
    case MAZE_EVENT_RECALIBRATE:
        if (a->state==MAZE_APP_PAUSED) calibrate(a,now_tick,MAZE_CAL_PAUSED);
        break;
    case MAZE_EVENT_REPLAY:
        if (a->state==MAZE_APP_PAUSED || a->state==MAZE_APP_WON || a->state==MAZE_APP_LOST)
            maze_app_begin(a,now_tick);
        break;
    case MAZE_EVENT_RETRY:
        if (a->state==MAZE_APP_CALIBRATION_FAILED) calibrate(a,now_tick,MAZE_CAL_NEW);
        else if (a->state==MAZE_APP_DEVICE_ERROR) calibrate(a,now_tick,MAZE_CAL_RECOVERY);
        break;
    case MAZE_EVENT_EXIT:
        if (a->state!=MAZE_APP_READY && a->state!=MAZE_APP_PLAYING) maze_app_cancel(a);
        break;
    }
}

static float dead_zone(int32_t value)
{
    if (value > MAZE_INPUT_DEAD_ZONE_RAW) return value-MAZE_INPUT_DEAD_ZONE_RAW;
    if (value < -MAZE_INPUT_DEAD_ZONE_RAW) return value+MAZE_INPUT_DEAD_ZONE_RAW;
    return 0;
}

void maze_app_sample(maze_app_t *a, const maze_imu_sample_t *s, uint32_t now_tick)
{
    if (s->generation != a->generation) return;
    /* 校准截止独立于硬件返回；就绪输入先核对样本自身的时间。 */
    if (a->state == MAZE_APP_CALIBRATING)
    {
        maze_app_update(a,now_tick);
        if (a->state!=MAZE_APP_CALIBRATING) return;
    }
    if (a->state != MAZE_APP_CALIBRATING && a->state != MAZE_APP_PLAYING && a->state != MAZE_APP_PAUSED) return;
    if (a->has_sequence && s->sequence == a->last_sequence) return;
    bool missing = a->has_sequence && (uint32_t)(s->sequence-a->last_sequence) != 1;
    a->has_sequence = true;
    a->last_sequence = s->sequence;
    if (s->error)
    {
        if (a->state==MAZE_APP_PAUSED) { a->error=s->error; a->input=(maze_input_t){0,0}; }
        else fail(a,s->error);
        return;
    }
    if (exceeds(a,now_tick-s->read_start_tick,MAZE_INPUT_MAX_AGE_MS))
    {
        if (a->state == MAZE_APP_PLAYING) fail(a,MAZE_ERROR_INPUT_STALE);
        else if (a->state==MAZE_APP_PAUSED) { a->error=MAZE_ERROR_INPUT_STALE; a->input=(maze_input_t){0,0}; }
        else { a->window_count=0; a->has_read=false; }
        return;
    }
    /* 最新快照允许跳过中间发布，采样间隔必须来自生产者，不能用消费间隔代替。 */
    bool gap = exceeds(a,s->read_gap_tick,MAZE_INPUT_MAX_GAP_MS) || s->read_gap_count!=a->last_gap_count;
    a->last_gap_count = s->read_gap_count;
    if (gap && a->state == MAZE_APP_PLAYING) { fail(a,MAZE_ERROR_INPUT_GAP); return; }
    a->has_read = true;
    a->last_read_tick = s->read_start_tick;
    if (a->state == MAZE_APP_PLAYING || a->state==MAZE_APP_PAUSED)
    {
        a->error=gap?MAZE_ERROR_INPUT_GAP:0;
        a->input.x = dead_zone((int32_t)s->raw[1]-a->reference[1]);
        a->input.y = dead_zone(a->reference[0]-(int32_t)s->raw[0]);
        return;
    }

    if (gap || missing) a->window_count=0;
    if (a->window_count == MAZE_CALIBRATION_COUNT)
    {
        memmove(&a->window[0],&a->window[1],sizeof(a->window[0])*(MAZE_CALIBRATION_COUNT-1));
        a->window_count--;
    }
    unsigned n = a->window_count++;
    memcpy(a->window[n].raw,s->raw,sizeof(s->raw));
    a->window[n].tick = s->read_start_tick;
    if (a->window_count != MAZE_CALIBRATION_COUNT) return;
    if ((uint64_t)(uint32_t)(s->read_start_tick-a->window[0].tick)*1000 <
        (uint64_t)MAZE_CALIBRATION_SPAN_MS*a->tick_hz) return;
    int32_t sums[3]={0};
    for (unsigned axis=0;axis<3;axis++)
    {
        int32_t low=a->window[0].raw[axis], high=low;
        for (unsigned i=0;i<MAZE_CALIBRATION_COUNT;i++)
        {
            int32_t value=a->window[i].raw[axis];
            if (value<low) low=value;
            if (value>high) high=value;
            sums[axis]+=value;
        }
        if (high-low > MAZE_CALIBRATION_RANGE_RAW) return;
    }
    for (unsigned axis=0;axis<3;axis++) a->reference[axis]=sums[axis]/MAZE_CALIBRATION_COUNT;
    a->has_reference = true;
    a->state = a->calibration_origin==MAZE_CAL_NEW?MAZE_APP_PLAYING:MAZE_APP_PAUSED;
    a->last_update_tick=now_tick;
    a->input = (maze_input_t){0,0};
}
