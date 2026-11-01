/* 纯C导航负责地图生命周期及会话边界，不操作显示或IMU硬件。 */
#include "maze_navigation.h"
#include <string.h>

void maze_navigation_init(maze_navigation_t *n, uint32_t tick_hz)
{
    memset(n,0,sizeof(*n));
    n->page=MAZE_NAV_SELECT;
    n->page_version=1;
    n->tick_hz=tick_hz;
    maze_archive_t archive;maze_archive_default(&archive);
    maze_progress_init(&n->progress,&archive);
}

static int load(maze_navigation_t *n, unsigned index)
{
    /* 先取消仍有效的旧关卡，再覆写唯一地图；错误页的旧app不能再读地图。 */
    if (n->page==MAZE_NAV_GAME)
    {
        maze_progress_leave(&n->progress,&n->app);
        maze_app_cancel(&n->app);
    }
    n->page_version++;
    n->index=index;
    n->error=maze_map_load(maze_catalog[index].definition,&n->map);
    if (n->error)
    {
        n->page=MAZE_NAV_MAP_ERROR;
        return n->error;
    }
    if (!n->app_initialized)
    {
        maze_app_init(&n->app,&n->map.level,n->tick_hz);
        n->app_initialized=true;
    }
    maze_app_bind_level(&n->app,&n->map.level,n->reversed);
    n->page=MAZE_NAV_GAME;
    return 0;
}

int maze_navigation_select(maze_navigation_t *n, unsigned index)
{
    /* 目录索引来自触摸命令，是导航输入边界。 */
    if (index>=maze_catalog_count) return MAZE_NAV_BAD_INDEX;
    if (n->page!=MAZE_NAV_SELECT) return 0;
    if(!maze_progress_unlocked(&n->progress,maze_catalog[index].definition->id)) return MAZE_NAV_LOCKED;
    return load(n,index);
}

void maze_navigation_back(maze_navigation_t *n)
{
    if (n->page==MAZE_NAV_SELECT) return;
    if (n->page==MAZE_NAV_GAME)
    {
        maze_progress_leave(&n->progress,&n->app);
        maze_app_cancel(&n->app);
    }
    n->page=MAZE_NAV_SELECT;
    n->page_version++;
    n->error=0;
}

void maze_navigation_toggle_direction(maze_navigation_t *n)
{
    if (n->page!=MAZE_NAV_SELECT) return;
    n->reversed=!n->reversed;
    n->page_version++;
}

int maze_navigation_next(maze_navigation_t *n)
{
    if (n->page!=MAZE_NAV_GAME || n->app.state!=MAZE_APP_WON ||
        n->index+1>=maze_catalog_count) return 0;
    if(!maze_progress_unlocked(&n->progress,maze_catalog[n->index+1].definition->id)) return MAZE_NAV_LOCKED;
    return load(n,n->index+1);
}

void maze_navigation_sample(maze_navigation_t *n, const maze_imu_sample_t *sample, uint32_t now_tick)
{
    if (n->page==MAZE_NAV_GAME) maze_app_sample(&n->app,sample,now_tick);
}

void maze_navigation_update(maze_navigation_t *n, uint32_t now_tick)
{
    if (n->page==MAZE_NAV_GAME)
    {
        maze_app_update(&n->app,now_tick);
        maze_progress_observe(&n->progress,&n->app);
    }
}

void maze_navigation_request(const maze_navigation_t *n, uint32_t *generation, bool *enabled)
{
    *generation=n->app_initialized?n->app.generation:0;
    *enabled=false;
    if (n->page==MAZE_NAV_GAME) maze_app_request(&n->app,generation,enabled);
}

bool maze_navigation_can_save(const maze_navigation_t *n)
{
    return n->page==MAZE_NAV_SELECT || n->page==MAZE_NAV_STORAGE_ERROR ||
        (n->page==MAZE_NAV_GAME && (n->app.state==MAZE_APP_WON || n->app.state==MAZE_APP_LOST));
}
