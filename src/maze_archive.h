#ifndef MAZE_ARCHIVE_H
#define MAZE_ARCHIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MAZE_ARCHIVE_BYTES 353
#define MAZE_ARCHIVE_LEVELS 2
#define MAZE_ARCHIVE_RANKS 5
enum { MAZE_ARCHIVE_FORMAT=-3001, MAZE_ARCHIVE_VERSION=-3002, MAZE_ARCHIVE_CRC=-3003 };
typedef struct { uint64_t time_cs; uint32_t mistakes; } maze_score_t;
typedef struct
{
    uint32_t losses;
    unsigned count;
    maze_score_t scores[MAZE_ARCHIVE_RANKS];
} maze_mode_record_t;
typedef struct
{
    char id[32];
    bool unlocked;
    unsigned completed;
    maze_mode_record_t modes[2];
} maze_level_record_t;
typedef struct
{
    uint64_t revision;
    maze_level_record_t levels[MAZE_ARCHIVE_LEVELS];
} maze_archive_t;
void maze_archive_default(maze_archive_t *archive);
uint32_t maze_archive_crc32(const uint8_t *bytes,size_t size);
int maze_archive_encode(const maze_archive_t *archive,uint8_t out[MAZE_ARCHIVE_BYTES]);
/* 仅成功时发布输出，拒绝外部存档的格式/版本/校验错误。 */
int maze_archive_decode(const uint8_t *bytes,size_t size,maze_archive_t *archive);
#endif
