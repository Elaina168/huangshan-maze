/* 纯C输入逻辑：无I2C、线程或LVGL依赖。 */
#include "maze_app.h"
#include <string.h>

static bool exceeds(maze_app_t *a, uint32_t ticks, unsigned ms)
{
    return (uint64_t)ticks * 1000 > (uint64_t)ms * a->tick_hz;
}

static void fail(maze_app_t *a, int error)
{
    a->state = a->state == MAZE_APP_CALIBRATING ?
        MAZE_APP_CALIBRATION_FAILED : MAZE_APP_INPUT_ERROR;
    a->error = error;
    a->input = (maze_input_t){0,0};
}

void maze_app_init(maze_app_t *a, const maze_level_t *level, uint32_t tick_hz)
{
    memset(a,0,sizeof(*a));
    a->level = level;
    a->tick_hz = tick_hz;
    a->state = MAZE_APP_READY;
}

void maze_app_begin(maze_app_t *a, uint32_t now_tick)
{
    a->generation++;
    a->state = MAZE_APP_CALIBRATING;
    a->started_tick = now_tick;
    a->window_count = 0;
    a->has_sequence = a->has_read = false;
    a->error = 0;
    a->input = (maze_input_t){0,0};
}

void maze_app_cancel(maze_app_t *a)
{
    a->generation++;
    a->state = MAZE_APP_READY;
    a->window_count = 0;
    a->has_reference = a->has_read = a->has_sequence = false;
    a->error = 0;
    a->input = (maze_input_t){0,0};
}

void maze_app_update(maze_app_t *a, uint32_t now_tick)
{
    if (a->state == MAZE_APP_CALIBRATING &&
        (uint64_t)(uint32_t)(now_tick-a->started_tick)*1000 >=
        (uint64_t)MAZE_CALIBRATION_BUDGET_MS*a->tick_hz)
        fail(a,MAZE_ERROR_CALIBRATION_TIMEOUT);
    else if (a->state == MAZE_APP_INPUT_READY &&
             exceeds(a,now_tick-a->last_read_tick,MAZE_INPUT_MAX_AGE_MS))
        fail(a,MAZE_ERROR_INPUT_STALE);
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
    if (a->state == MAZE_APP_CALIBRATING) maze_app_update(a,now_tick);
    if (a->state != MAZE_APP_CALIBRATING && a->state != MAZE_APP_INPUT_READY) return;
    if (a->has_sequence && s->sequence == a->last_sequence) return;
    bool missing = a->has_sequence && (uint32_t)(s->sequence-a->last_sequence) != 1;
    a->has_sequence = true;
    a->last_sequence = s->sequence;
    if (s->error) { fail(a,s->error); return; }
    if (exceeds(a,now_tick-s->read_start_tick,MAZE_INPUT_MAX_AGE_MS))
    {
        if (a->state == MAZE_APP_INPUT_READY) fail(a,MAZE_ERROR_INPUT_STALE);
        else { a->window_count=0; a->has_read=false; }
        return;
    }
    bool gap = a->has_read && exceeds(a,s->read_start_tick-a->last_read_tick,MAZE_INPUT_MAX_GAP_MS);
    if (gap && a->state == MAZE_APP_INPUT_READY) { fail(a,MAZE_ERROR_INPUT_GAP); return; }
    a->has_read = true;
    a->last_read_tick = s->read_start_tick;
    if (a->state == MAZE_APP_INPUT_READY)
    {
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
    a->state = MAZE_APP_INPUT_READY;
    a->input = (maze_input_t){0,0};
}
