#ifndef MAZE_IMU_H
#define MAZE_IMU_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t generation;
    uint32_t sequence;
    uint32_t read_start_tick;
    int16_t raw[3];
    int error;
} maze_imu_sample_t;

int maze_imu_init(void);
void maze_imu_request(uint32_t generation, bool enabled);
bool maze_imu_latest(maze_imu_sample_t *out);
bool maze_imu_owned(void);
#endif
