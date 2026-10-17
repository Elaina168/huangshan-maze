#ifndef MAZE_APP_H
#define MAZE_APP_H
#include "maze_imu.h"

/* 本阶段均为预研候选值，不是正式硬件性能基线。 */
#define MAZE_CALIBRATION_COUNT 20
#define MAZE_CALIBRATION_SPAN_MS 950
#define MAZE_CALIBRATION_RANGE_RAW 300
#define MAZE_CALIBRATION_BUDGET_MS 3000
#define MAZE_INPUT_DEAD_ZONE_RAW 300
#define MAZE_INPUT_MAX_AGE_MS 100
#define MAZE_INPUT_MAX_GAP_MS 100

typedef struct maze_level maze_level_t;
typedef struct { float x, y; } maze_input_t;
typedef enum
{
    MAZE_APP_READY, MAZE_APP_CALIBRATING, MAZE_APP_INPUT_READY,
    MAZE_APP_CALIBRATION_FAILED, MAZE_APP_INPUT_ERROR
} maze_app_state_t;
enum
{
    MAZE_ERROR_INPUT_STALE = -1001,
    MAZE_ERROR_INPUT_GAP = -1002,
    MAZE_ERROR_CALIBRATION_TIMEOUT = -1003
};
typedef struct
{
    const maze_level_t *level;
    uint32_t tick_hz, generation, started_tick, last_read_tick, last_sequence;
    bool has_sequence, has_read, has_reference;
    maze_app_state_t state;
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
#endif
