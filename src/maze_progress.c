/* 挑战累计与存档内容纯逻辑；IMU代次不作为玩家尝试身份。 */
#include "maze_progress.h"
#include <string.h>
#include "maze_random.h"

void maze_progress_init(maze_progress_t *p,const maze_archive_t *archive)
{
    memset(p,0,sizeof(*p));p->archive=*archive;
}

static int index_of(const maze_progress_t *p,const char *id)
{
    for(unsigned i=0;i<MAZE_ARCHIVE_LEVELS;i++) if(!strcmp(p->archive.levels[i].id,id)) return (int)i;
    return -1;
}
bool maze_progress_unlocked(const maze_progress_t *p,const char *id)
{
    int index=index_of(p,id);
    return index>=0 && p->archive.levels[index].unlocked;
}
static void begin(maze_progress_t *p,const maze_app_t *a)
{
    p->active=true;p->finished=false;p->accounted=false;
    p->random=!strcmp(a->level->id,MAZE_RANDOM_ID);
    p->level=p->random?0:(unsigned)index_of(p,a->level->id);p->mode=maze_app_mode(a);
    p->accrued_ms=0;p->mistakes=p->rank=0;
}
static void fold(maze_progress_t *p,const maze_app_t *a)
{
    if(!p->accounted) { p->accrued_ms+=a->elapsed_ms;p->accounted=true; }
}
void maze_progress_before_event(maze_progress_t *p,const maze_app_t *a,maze_event_t event)
{
    if(event==MAZE_EVENT_START && a->state==MAZE_APP_READY)
    {
        if(!p->active || p->finished) begin(p,a);
        p->accounted=false;
    }
    else if(event==MAZE_EVENT_REPLAY && (a->state==MAZE_APP_WON ||
            a->state==MAZE_APP_LOST || a->state==MAZE_APP_PAUSED))
    {
        if(a->state==MAZE_APP_WON) begin(p,a);
        else if(p->active) { fold(p,a);p->accounted=false;p->rank=0; }
    }
    else if(event==MAZE_EVENT_RESPAWN && a->state==MAZE_APP_LOST && a->model.checkpoint && p->active)
    {
        /* 落坑已折入挑战，复活开始新段；校准期间不重复累计旧段。 */
        fold(p,a);p->accounted=false;p->rank=0;
    }
    else if(event==MAZE_EVENT_EXIT && p->active && a->state==MAZE_APP_CALIBRATING) fold(p,a);
}

uint64_t maze_progress_time_cs(const maze_progress_t *p,const maze_app_t *a)
{
    if(!p->active) return 0;
    return (uint64_t)((p->accrued_ms+(p->accounted?0:a->elapsed_ms))/10);
}

void maze_progress_observe(maze_progress_t *p,const maze_app_t *a)
{
    if(!p->active || p->finished || p->accounted ||
       (a->state!=MAZE_APP_LOST && a->state!=MAZE_APP_WON)) return;
    /* 娱乐模式只维护复活/重玩的局内状态，不统计成绩或失误。 */
    if(p->random){fold(p,a);p->finished=a->state==MAZE_APP_WON;return;}
    /* 外部档案可带最大修订；明确拒绝不可表示的新结算，保留整个旧快照。 */
    if(p->archive.revision==UINT64_MAX) { p->revision_exhausted=true;return; }
    fold(p,a);
    maze_level_record_t *level=&p->archive.levels[p->level];
    if(a->state==MAZE_APP_LOST)
    {
        p->mistakes++;level->losses++;p->archive.revision++;
        return;
    }
    p->rank=maze_score_insert(level,(maze_score_t){maze_progress_time_cs(p,a),p->mistakes,p->mode});
    bool changed=p->rank!=0;
    if(!p->random)
    {
        maze_level_record_t *level=&p->archive.levels[p->level];
        changed=changed || !(level->completed&(1u<<p->mode));
        level->completed|=1u<<p->mode;
        if(p->level+1<MAZE_ARCHIVE_LEVELS&&!p->archive.levels[p->level+1].unlocked)
        {p->archive.levels[p->level+1].unlocked=true;changed=true;}
    }
    if(changed) p->archive.revision++;
    p->finished=true;
}

void maze_progress_leave(maze_progress_t *p,const maze_app_t *a)
{
    if(p->active) fold(p,a);
    p->active=false;p->finished=false;
}

bool maze_progress_random_accept(maze_progress_t*p,uint16_t version,uint32_t seed)
{
    if(p->archive.revision==UINT64_MAX){p->revision_exhausted=true;return false;}
    p->archive.random=(maze_random_record_t){.valid=true,.generator_version=version,.seed=seed};
    p->archive.revision++;p->active=p->finished=p->accounted=false;p->accrued_ms=0;p->mistakes=p->rank=0;
    return true;
}
