#ifndef MAZE_CAMERA_H
#define MAZE_CAMERA_H
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    float x,y,world_width,world_height;
    uint32_t last_tick;
} maze_camera_t;
void maze_camera_reset(maze_camera_t *camera,float world_width,float world_height,
                       float ball_x,float ball_y,uint32_t now_tick);
void maze_camera_update(maze_camera_t *camera,float ball_x,float ball_y,
                        uint32_t now_tick,uint32_t tick_hz,bool playing);
#endif
