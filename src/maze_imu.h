#ifndef MAZE_IMU_H
#define MAZE_IMU_H
#include "maze_config.h"
#include <stdbool.h>
#include <stdint.h>
/* 生产者记录断档事实，消费者仍决定校准清窗或游玩冻结。 */

typedef struct
{
    uint32_t generation;
    uint32_t sequence;
    uint32_t read_start_tick;
    uint32_t read_gap_tick; /* 同一采样会话中，相邻实际读取起点间隔；首条为0。 */
    uint32_t read_gap_count; /* 会话内超限累计次数，不被后续健康快照覆盖。 */
    int16_t raw[3];
    int error;
} maze_imu_sample_t;

int maze_imu_init(void);
void maze_imu_request(uint32_t generation, bool enabled);
bool maze_imu_latest(maze_imu_sample_t *out);
bool maze_imu_owned(void);
#endif
