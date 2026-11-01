#ifndef MAZE_STORE_H
#define MAZE_STORE_H
#include "maze_archive.h"
#include <flashdb.h>
enum { MAZE_STORE_NO_ARCHIVE=-3101, MAZE_STORE_IO=-3102, MAZE_STORE_CONFLICT=-3103,
       MAZE_STORE_STALE=-3104, MAZE_STORE_PARTITION=-3105 };
typedef struct
{
    struct fdb_kvdb db[2];
    const struct fal_partition *parts[2];
    bool mounted[2],repair;
    int status[2],active;
    uint64_t revision;
    uint8_t current[MAZE_ARCHIVE_BYTES];
} maze_store_t;
/* 实例静态存放，仅安全状态的主线程调用；不自动覆盖未知格式档案。 */
int maze_store_open(maze_store_t *store,maze_archive_t *archive);
int maze_store_save(maze_store_t *store,const maze_archive_t *archive);
/* 项目NOR适配层的写闸门：-1只读，0/1仅允许对应副本写擦。 */
void maze_store_flash_allow(int bank);
#endif
