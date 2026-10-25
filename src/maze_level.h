#ifndef MAZE_LEVEL_H
#define MAZE_LEVEL_H

typedef struct { float x, y; } maze_point_t;
typedef struct { float x, y, width, height; } maze_rect_t;
typedef struct { float x, y, radius; } maze_circle_t;

typedef struct maze_level
{
    float width, height, ball_radius;
    maze_point_t start;
    const maze_rect_t *walls;
    unsigned wall_count;
    const maze_circle_t *traps;
    unsigned trap_count;
    maze_circle_t goal;
} maze_level_t;

extern const maze_level_t maze_level_one;
#endif
