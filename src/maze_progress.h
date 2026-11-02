#ifndef MAZE_PROGRESS_H
#define MAZE_PROGRESS_H
#include "maze_archive.h"
#include "maze_app.h"
typedef struct
{
    maze_archive_t archive;
    bool active,finished,accounted;
    bool revision_exhausted;
    unsigned level,mode,mistakes,rank;
    double accrued_ms;
} maze_progress_t;
void maze_progress_init(maze_progress_t *progress,const maze_archive_t *archive);
void maze_progress_before_event(maze_progress_t *progress,const maze_app_t *app,maze_event_t event);
void maze_progress_observe(maze_progress_t *progress,const maze_app_t *app);
void maze_progress_leave(maze_progress_t *progress,const maze_app_t *app);
uint64_t maze_progress_time_cs(const maze_progress_t *progress,const maze_app_t *app);
bool maze_progress_unlocked(const maze_progress_t *progress,const char *id);
#endif
