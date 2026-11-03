#ifndef MAZE_LEVEL_H
#define MAZE_LEVEL_H

typedef struct { float x, y; } maze_point_t;
typedef struct { float x, y, width, height; } maze_rect_t;
typedef struct { float x, y, radius; } maze_circle_t;

typedef enum { MAZE_SURFACE_NORMAL, MAZE_SURFACE_ICE, MAZE_SURFACE_ROUGH } maze_surface_t;
typedef struct { maze_rect_t area; maze_surface_t surface; } maze_surface_area_t;

typedef enum
{
    MAZE_MECH_KEY, MAZE_MECH_SWITCH, MAZE_MECH_KEY_DOOR,
    MAZE_MECH_SWITCH_DOOR, MAZE_MECH_BREAKABLE, MAZE_MECH_CHECKPOINT
} maze_mechanism_kind_t;
typedef struct { maze_mechanism_kind_t kind; maze_rect_t area; unsigned link; } maze_mechanism_t;

typedef enum { MAZE_DYNAMIC_MAGNET, MAZE_DYNAMIC_LASER, MAZE_DYNAMIC_GATE, MAZE_DYNAMIC_PATROL } maze_dynamic_kind_t;
typedef struct {
    maze_dynamic_kind_t kind;
    maze_rect_t area;
    maze_point_t end;
    float force, outer_radius, inner_radius, speed;
    unsigned off_ms, warning_ms, on_ms, dwell_ms, phase_ms;
} maze_dynamic_t;

typedef struct maze_level
{
    float width, height, ball_radius;
    maze_point_t start;
    const maze_rect_t *walls;
    unsigned wall_count;
    const maze_circle_t *traps;
    unsigned trap_count;
    maze_circle_t goal;
    const maze_surface_area_t *surfaces;
    unsigned surface_count;
    const char *id;
    const maze_mechanism_t *mechanisms;
    unsigned mechanism_count;
    const maze_dynamic_t *dynamics;
    unsigned dynamic_count;
} maze_level_t;

maze_surface_t maze_level_surface_at(const maze_level_t *level, float x, float y);
#endif
