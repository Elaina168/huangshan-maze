#ifndef MAZE_NAVIGATION_H
#define MAZE_NAVIGATION_H
#include "maze_app.h"
#include "maze_catalog.h"
#include "maze_progress.h"
#include "maze_random.h"

typedef enum { MAZE_NAV_SELECT, MAZE_NAV_GAME, MAZE_NAV_MAP_ERROR,
               MAZE_NAV_SCORES, MAZE_NAV_STORAGE_ERROR, MAZE_NAV_GENERATING, MAZE_NAV_RANDOM_ERROR } maze_navigation_page_t;
enum { MAZE_NAV_BAD_INDEX=-2101, MAZE_NAV_LOCKED=-2102 };
typedef struct
{
    maze_navigation_page_t page;
    uint32_t page_version, tick_hz;
    unsigned index;
    bool reversed,dark,lighting,app_initialized;
    int lighting_error;
    int error;
    int store_error;
    bool store_repair;
    bool save_pending;
    unsigned score_index;
    maze_map_t maps[2];
    maze_map_t *active_map;
    maze_app_t app;
    maze_progress_t progress;
    maze_generated_t generated[2];
    maze_generator_t generator;
    bool random,random_exact,random_checking,publish_ready,random_timing_started;
    maze_navigation_page_t random_origin_page;
    unsigned random_target;
    uint32_t random_job_id,random_started_tick,random_total_tick,random_max_step_tick;
    int random_check_error;
} maze_navigation_t;

/* 实例须静态存储，不得按值搬迁；选择/下一关前由UI解除旧场景引用。 */
void maze_navigation_init(maze_navigation_t *nav, uint32_t tick_hz);
int maze_navigation_select(maze_navigation_t *nav, unsigned index);
void maze_navigation_back(maze_navigation_t *nav);
void maze_navigation_toggle_direction(maze_navigation_t *nav);
void maze_navigation_toggle_dark(maze_navigation_t *nav);
int maze_navigation_next(maze_navigation_t *nav);
void maze_navigation_sample(maze_navigation_t *nav, const maze_imu_sample_t *sample, uint32_t now_tick);
void maze_navigation_update(maze_navigation_t *nav, uint32_t now_tick);
void maze_navigation_request(const maze_navigation_t *nav, uint32_t *generation, bool *enabled);
int maze_navigation_random_open(maze_navigation_t*,uint32_t seed);
int maze_navigation_random_replace(maze_navigation_t*);
int maze_navigation_random_step(maze_navigation_t*,uint32_t now,uint32_t(*clock_now)(void));
void maze_navigation_random_cancel(maze_navigation_t*);
int maze_navigation_random_accept(maze_navigation_t*,uint32_t job_id);
bool maze_navigation_can_save(const maze_navigation_t *nav);
#endif
