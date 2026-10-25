#ifndef MAZE_MODEL_H
#define MAZE_MODEL_H
#include <stdint.h>
#include "maze_level.h"

/* 运动候选值，实际手感及性能仍须板端确认。 */
#define MAZE_SPEED_MAX 120.0f
#define MAZE_INPUT_FULL_RAW 8192.0f

typedef enum { MAZE_MODEL_RUNNING, MAZE_MODEL_WON, MAZE_MODEL_LOST } maze_model_result_t;
typedef struct
{
    const maze_level_t *level;
    float x, y, vx, vy, step_elapsed_ms;
    maze_model_result_t result;
} maze_model_t;

void maze_model_reset(maze_model_t *model, const maze_level_t *level);
void maze_model_step(maze_model_t *model, float input_x, float input_y, uint32_t dt_ms);
#endif
