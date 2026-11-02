#ifndef MAZE_MAP_H
#define MAZE_MAP_H
#include <stddef.h>
#include "maze_config.h"
#include "maze_level.h"

typedef enum { MAZE_MAP_START, MAZE_MAP_GOAL, MAZE_MAP_TRAP } maze_map_object_kind_t;
typedef struct { maze_map_object_kind_t kind; maze_circle_t circle; } maze_map_object_t;
typedef struct
{
    const char *id;
    unsigned columns, rows;
    const char *const *lines;
    unsigned line_count;
    const maze_map_object_t *objects;
    unsigned object_count;
    const maze_mechanism_t *mechanisms;
    unsigned mechanism_count;
} maze_map_definition_t;

enum
{
    MAZE_MAP_OK=0, MAZE_MAP_FORMAT=-2001, MAZE_MAP_OBJECT=-2002,
    MAZE_MAP_GEOMETRY=-2003, MAZE_MAP_NO_ROUTE=-2004
};

typedef struct
{
    maze_level_t level;
    maze_rect_t walls[MAZE_MAP_MAX_WALLS];
    maze_surface_area_t surfaces[MAZE_MAP_MAX_CELLS];
    maze_circle_t traps[MAZE_MAP_MAX_CELLS];
    unsigned checked_nodes;
} maze_map_t;

/* 仅由UI/主线程在进入关卡前调用；结果内部有指针，不得按值搬迁或在游玩中覆写。
 * 定义的指针/字符串必须有效，id须在结果使用期间存活；失败结果不得交给模型。
 * 路径工作区静态复用，接口不重入。 */
int maze_map_load(const maze_map_definition_t *definition, maze_map_t *out);
size_t maze_map_workspace_size(void);
extern const maze_map_definition_t maze_map_frost_fork;
extern const maze_map_definition_t maze_map_second;
extern const maze_map_definition_t maze_map_ice_loops;
extern const maze_map_definition_t maze_map_mechanism_trial;
#endif
