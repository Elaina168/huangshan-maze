#ifndef MAZE_APP_H
#define MAZE_APP_H
#include "maze_imu.h"
#include "maze_model.h"

/* 本阶段均为预研候选值，不是正式硬件性能基线。 */
#define MAZE_CALIBRATION_COUNT 20
#define MAZE_CALIBRATION_SPAN_MS 950
#define MAZE_CALIBRATION_RANGE_RAW 300
#define MAZE_CALIBRATION_BUDGET_MS 3000
#define MAZE_INPUT_DEAD_ZONE_RAW 300
#define MAZE_INPUT_MAX_AGE_MS 100
#define MAZE_INPUT_MAX_GAP_MS MAZE_IMU_MAX_GAP_MS
#define MAZE_PHYSICS_STEP_MS 10
#define MAZE_UPDATE_MAX_GAP_MS 100

typedef struct { float x, y; } maze_input_t;
typedef enum
{
    MAZE_APP_READY, MAZE_APP_CALIBRATING, MAZE_APP_PLAYING,
    MAZE_APP_CALIBRATION_FAILED, MAZE_APP_DEVICE_ERROR,
    MAZE_APP_PAUSED, MAZE_APP_WON, MAZE_APP_LOST
} maze_app_state_t;
typedef enum
{
    MAZE_EVENT_START, MAZE_EVENT_PAUSE, MAZE_EVENT_RESUME, MAZE_EVENT_RECALIBRATE,
    MAZE_EVENT_REPLAY, MAZE_EVENT_RETRY, MAZE_EVENT_EXIT
} maze_event_t;
typedef enum { MAZE_CAL_NEW, MAZE_CAL_PAUSED, MAZE_CAL_RECOVERY } maze_calibration_origin_t;
enum
{
    MAZE_ERROR_INPUT_STALE = -1001,
    MAZE_ERROR_INPUT_GAP = -1002,
    MAZE_ERROR_CALIBRATION_TIMEOUT = -1003,
    MAZE_ERROR_TIMING_GAP = -1004
};
typedef struct
{
    const maze_level_t *level;
    uint32_t tick_hz, generation, started_tick, last_read_tick, last_sequence;
    uint32_t last_gap_count;
    bool has_sequence, has_read, has_reference;
    maze_app_state_t state;
    maze_calibration_origin_t calibration_origin;
    maze_model_t model;
    uint32_t last_update_tick;
    uint64_t motion_units;
    double elapsed_ms;
    int error;
    int32_t reference[3];
    maze_input_t input;
    unsigned window_count;
    struct { int16_t raw[3]; uint32_t tick; } window[MAZE_CALIBRATION_COUNT];
} maze_app_t;

void maze_app_init(maze_app_t *app, const maze_level_t *level, uint32_t tick_hz);
void maze_app_begin(maze_app_t *app, uint32_t now_tick);
void maze_app_cancel(maze_app_t *app);
void maze_app_sample(maze_app_t *app, const maze_imu_sample_t *sample, uint32_t now_tick);
void maze_app_update(maze_app_t *app, uint32_t now_tick);
void maze_app_event(maze_app_t *app, maze_event_t event, uint32_t now_tick);
void maze_app_request(const maze_app_t *app, uint32_t *generation, bool *enabled);
#endif
