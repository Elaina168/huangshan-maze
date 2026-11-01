#ifndef MAZE_NAVIGATION_H
#define MAZE_NAVIGATION_H
#include "maze_app.h"
#include "maze_catalog.h"
#include "maze_progress.h"

typedef enum { MAZE_NAV_SELECT, MAZE_NAV_GAME, MAZE_NAV_MAP_ERROR,
               MAZE_NAV_SCORES, MAZE_NAV_STORAGE_ERROR } maze_navigation_page_t;
enum { MAZE_NAV_BAD_INDEX=-2101, MAZE_NAV_LOCKED=-2102 };
typedef struct
{
    maze_navigation_page_t page;
    uint32_t page_version, tick_hz;
    unsigned index;
    bool reversed, app_initialized;
    int error;
    int store_error;
    bool store_repair;
    bool save_pending;
    unsigned score_index,score_mode;
    maze_map_t map;
    maze_app_t app;
    maze_progress_t progress;
} maze_navigation_t;

/* 实例须静态存储，不得按值搬迁；选择/下一关前由UI解除旧场景引用。 */
void maze_navigation_init(maze_navigation_t *nav, uint32_t tick_hz);
int maze_navigation_select(maze_navigation_t *nav, unsigned index);
void maze_navigation_back(maze_navigation_t *nav);
void maze_navigation_toggle_direction(maze_navigation_t *nav);
int maze_navigation_next(maze_navigation_t *nav);
void maze_navigation_sample(maze_navigation_t *nav, const maze_imu_sample_t *sample, uint32_t now_tick);
void maze_navigation_update(maze_navigation_t *nav, uint32_t now_tick);
void maze_navigation_request(const maze_navigation_t *nav, uint32_t *generation, bool *enabled);
bool maze_navigation_can_save(const maze_navigation_t *nav);
#endif
