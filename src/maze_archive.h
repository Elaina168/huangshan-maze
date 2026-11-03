#ifndef MAZE_ARCHIVE_H
#define MAZE_ARCHIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MAZE_ARCHIVE_V1_BYTES 353
#define MAZE_ARCHIVE_V2_BYTES 517
#define MAZE_ARCHIVE_V3_BYTES 681
#define MAZE_ARCHIVE_V4_BYTES 845
#define MAZE_ARCHIVE_V5_BYTES 982
#define MAZE_ARCHIVE_MAX_BYTES MAZE_ARCHIVE_V5_BYTES
#define MAZE_ARCHIVE_BYTES 524
#define MAZE_ARCHIVE_LEVELS 3
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
typedef struct {
    bool valid;
    uint16_t generator_version;
    uint32_t seed;
} maze_random_record_t;
typedef struct
{
    uint64_t revision;
    maze_level_record_t levels[MAZE_ARCHIVE_LEVELS];
    maze_random_record_t random;
} maze_archive_t;
void maze_archive_default(maze_archive_t *archive);
uint32_t maze_archive_crc32(const uint8_t *bytes,size_t size);
int maze_archive_encode(const maze_archive_t *archive,uint8_t out[MAZE_ARCHIVE_BYTES]);
/* 接受v1至v6，按稳定ID保留三关；旧两关/随机成绩仅校验后丢弃。 */
int maze_archive_decode(const uint8_t *bytes,size_t size,maze_archive_t *archive);
#endif
