#ifndef MAZE_MODEL_H
#define MAZE_MODEL_H
#include "maze_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "maze_level.h"

/* 运动候选值，实际手感及性能仍须板端确认。 */

typedef enum { MAZE_MODEL_RUNNING, MAZE_MODEL_WON, MAZE_MODEL_LOST } maze_model_result_t;
typedef enum { MAZE_LOSS_TRAP, MAZE_LOSS_LASER, MAZE_LOSS_CRUSH } maze_loss_reason_t;
/* phase为0关闭/开放、1预警、2激活；巡逻和磁场始终为2。 */
typedef struct { maze_rect_t area; unsigned phase; bool blocked; } maze_dynamic_view_t;
typedef struct
{
    const maze_level_t *level;
    float x, y, vx, vy, step_elapsed_ms;
    maze_model_result_t result;
    uint32_t keys, switches, broken;
    unsigned checkpoint;
    uint32_t checkpoint_keys, checkpoint_switches, checkpoint_broken;
    uint64_t dynamic_us, checkpoint_dynamic_us;
    uint32_t gate_pending, checkpoint_gate_pending;
    maze_loss_reason_t loss_reason;
} maze_model_t;

void maze_model_dynamic_view(const maze_model_t *model, unsigned index, maze_dynamic_view_t *view);
void maze_model_reset(maze_model_t *model, const maze_level_t *level);
bool maze_model_respawn(maze_model_t *model);
void maze_model_step(maze_model_t *model, float input_x, float input_y, uint32_t dt_ms);
#endif
