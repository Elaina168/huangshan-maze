/* 纯C导航负责地图生命周期及会话边界，不操作显示或IMU硬件。 */
#include "maze_navigation.h"
#include <string.h>

void maze_navigation_init(maze_navigation_t *n, uint32_t tick_hz)
{
    memset(n,0,sizeof(*n));
    n->page=MAZE_NAV_SELECT;
    n->active_map=&n->maps[1];
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
    maze_map_t *target=n->active_map==&n->maps[0]?&n->maps[1]:&n->maps[0];
    n->error=maze_map_load(maze_catalog[index].definition,target);
    if (n->error)
    {
        n->page=MAZE_NAV_MAP_ERROR;
        return n->error;
    }
    n->random=false;
    n->active_map=target;
    if (!n->app_initialized)
    {
        maze_app_init(&n->app,&n->active_map->level,n->tick_hz);
        n->app_initialized=true;
    }
    maze_app_bind_level(&n->app,&n->active_map->level,n->reversed);
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
    if(n->page==MAZE_NAV_GENERATING||n->page==MAZE_NAV_RANDOM_ERROR){maze_navigation_random_cancel(n);return;}
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
    if (n->page!=MAZE_NAV_GAME || n->app.state!=MAZE_APP_WON || n->random ||
        n->index+1>=maze_catalog_count) return 0;
    if(!maze_progress_unlocked(&n->progress,maze_catalog[n->index+1].definition->id)) return MAZE_NAV_LOCKED;
    return load(n,n->index+1);
}

static bool background_paused(const maze_navigation_t*n)
{
 return (n->page==MAZE_NAV_GENERATING||n->page==MAZE_NAV_RANDOM_ERROR)&&n->random_origin_page==MAZE_NAV_GAME&&n->app.state==MAZE_APP_PAUSED;
}
void maze_navigation_sample(maze_navigation_t *n, const maze_imu_sample_t *sample, uint32_t now_tick)
{
    if (n->page==MAZE_NAV_GAME||background_paused(n)) maze_app_sample(&n->app,sample,now_tick);
}

void maze_navigation_update(maze_navigation_t *n, uint32_t now_tick)
{
    if (n->page==MAZE_NAV_GAME||background_paused(n))
    {
        maze_app_update(&n->app,now_tick);
        maze_progress_observe(&n->progress,&n->app);
    }
}

void maze_navigation_request(const maze_navigation_t *n, uint32_t *generation, bool *enabled)
{
    *generation=n->app_initialized?n->app.generation:0;
    *enabled=false;
    if (n->page==MAZE_NAV_GAME||background_paused(n)) maze_app_request(&n->app,generation,enabled);
}

bool maze_navigation_can_save(const maze_navigation_t *n)
{
    return n->page==MAZE_NAV_SELECT || n->page==MAZE_NAV_STORAGE_ERROR ||
        (n->page==MAZE_NAV_GAME && (n->app.state==MAZE_APP_WON || n->app.state==MAZE_APP_LOST || (n->random&&n->app.state==MAZE_APP_READY)));
}

static void random_begin(maze_navigation_t*n,uint32_t seed,bool exact)
{
 if(n->page!=MAZE_NAV_RANDOM_ERROR)n->random_origin_page=n->page;
 n->random_target=n->active_map==&n->maps[0]?1:0;
 n->random_exact=exact;n->random_checking=n->publish_ready=n->random_timing_started=false;
 n->random_check_error=n->error=0;n->random_max_step_tick=n->random_total_tick=0;
 n->random_job_id++;n->page=MAZE_NAV_GENERATING;n->page_version++;
 maze_generator_begin(&n->generator,&n->generated[n->random_target],seed,exact);
}
int maze_navigation_random_open(maze_navigation_t*n,uint32_t seed)
{
 if(n->page!=MAZE_NAV_SELECT)return 0;
 if(n->progress.archive.random.valid&&n->progress.archive.random.generator_version!=MAZE_GENERATOR_VERSION)
 {n->random_origin_page=MAZE_NAV_SELECT;n->page=MAZE_NAV_RANDOM_ERROR;n->error=MAZE_RANDOM_VERSION;n->page_version++;return n->error;}
 bool exact=n->progress.archive.random.valid;
 random_begin(n,exact?n->progress.archive.random.seed:seed,exact);return 0;
}
int maze_navigation_random_replace(maze_navigation_t*n)
{
 if(n->page!=MAZE_NAV_RANDOM_ERROR && (n->page!=MAZE_NAV_GAME||!n->random||
   (n->app.state!=MAZE_APP_READY&&n->app.state!=MAZE_APP_PAUSED&&n->app.state!=MAZE_APP_WON&&n->app.state!=MAZE_APP_LOST)))return 0;
 uint32_t seed=n->progress.archive.random.seed*1664525u+1013904223u;
 random_begin(n,seed,false);return 0;
}
static int random_fail(maze_navigation_t*n,int error)
{
 maze_map_cancel();maze_generator_cancel(&n->generator);n->publish_ready=false;
 n->page=MAZE_NAV_RANDOM_ERROR;n->error=error;n->page_version++;return error;
}
int maze_navigation_random_step(maze_navigation_t*n,uint32_t now,uint32_t(*clock_now)(void))
{
 if(n->page!=MAZE_NAV_GENERATING)return 0;
 if(n->publish_ready)return MAZE_RANDOM_READY;
 if(!n->random_timing_started){n->random_started_tick=now;n->random_timing_started=true;}
 if((uint64_t)(uint32_t)(now-n->random_started_tick)*1000>=(uint64_t)3000*n->tick_hz)return random_fail(n,MAZE_RANDOM_TIMEOUT);
 uint32_t start=clock_now?clock_now():now;
 int result;
 if(!n->random_checking){
  result=maze_generator_step(&n->generator,512);
  if(!result){n->maps[n->random_target].checked_nodes=0;maze_map_begin(&n->generated[n->random_target].definition,&n->maps[n->random_target]);n->random_checking=true;result=MAZE_RANDOM_PENDING;}
 }else{
  result=maze_map_step(128,1024,now+(10*n->tick_hz+999)/1000,clock_now);
  if(!result){n->publish_ready=true;result=MAZE_RANDOM_READY;}
  else if(result!=MAZE_MAP_PENDING){n->random_check_error=result;result=maze_generator_reject(&n->generator);n->random_checking=false;}
 }
 uint32_t end=clock_now?clock_now():now;
 if(end-start>n->random_max_step_tick)n->random_max_step_tick=end-start;
 n->random_total_tick=end-n->random_started_tick;
 if((uint64_t)n->random_total_tick*1000>=(uint64_t)3000*n->tick_hz)return random_fail(n,MAZE_RANDOM_TIMEOUT);
 if(result<0)return random_fail(n,result);
 return result;
}
void maze_navigation_random_cancel(maze_navigation_t*n)
{
 if(n->page!=MAZE_NAV_GENERATING&&n->page!=MAZE_NAV_RANDOM_ERROR)return;
 maze_map_cancel();maze_generator_cancel(&n->generator);n->publish_ready=false;n->random_job_id++;
 n->page=n->random_origin_page;n->error=0;n->page_version++;
}
int maze_navigation_random_accept(maze_navigation_t*n,uint32_t job_id)
{
 if(n->page!=MAZE_NAV_GENERATING||!n->publish_ready||job_id!=n->random_job_id)return 0;
 if(!n->random_exact&&n->progress.archive.revision==UINT64_MAX){n->progress.revision_exhausted=true;return random_fail(n,MAZE_RANDOM_REVISION);}
 if(n->app_initialized){maze_progress_leave(&n->progress,&n->app);maze_app_cancel(&n->app);}
 if(!n->random_exact)maze_progress_random_accept(&n->progress,MAZE_GENERATOR_VERSION,n->generated[n->random_target].seed);
 n->active_map=&n->maps[n->random_target];n->random=true;
 if(!n->app_initialized){maze_app_init(&n->app,&n->active_map->level,n->tick_hz);n->app_initialized=true;}
 maze_app_bind_level(&n->app,&n->active_map->level,n->reversed);
 n->page=MAZE_NAV_GAME;n->page_version++;n->publish_ready=false;n->error=0;return 0;
}
