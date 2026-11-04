#ifndef MAZE_ARCHIVE_H
#define MAZE_ARCHIVE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "maze_config.h"
#define MAZE_ARCHIVE_V1_BYTES 353
#define MAZE_ARCHIVE_V2_BYTES 517
#define MAZE_ARCHIVE_V3_BYTES 681
#define MAZE_ARCHIVE_V4_BYTES 845
#define MAZE_ARCHIVE_V5_BYTES 982
#define MAZE_ARCHIVE_V6_BYTES 524
#define MAZE_ARCHIVE_MAX_BYTES MAZE_ARCHIVE_V5_BYTES
#define MAZE_ARCHIVE_BYTES 356
#define MAZE_ARCHIVE_LEVELS 3
#define MAZE_ARCHIVE_RANKS 5
enum { MAZE_ARCHIVE_FORMAT=-3001, MAZE_ARCHIVE_VERSION=-3002, MAZE_ARCHIVE_CRC=-3003 };
typedef struct { uint64_t time_cs; uint32_t mistakes; unsigned mode; } maze_score_t;
typedef struct {
 char id[32];bool unlocked;unsigned completed;
 uint64_t losses;unsigned count;maze_score_t scores[MAZE_ARCHIVE_RANKS];
} maze_level_record_t;
typedef struct {bool valid;uint16_t generator_version;uint32_t seed;} maze_random_record_t;
typedef struct {uint64_t revision;maze_level_record_t levels[MAZE_ARCHIVE_LEVELS];maze_random_record_t random;} maze_archive_t;
void maze_archive_default(maze_archive_t *archive);
bool maze_score_precedes(maze_score_t a,maze_score_t b);
unsigned maze_score_insert(maze_level_record_t *level,maze_score_t score);
uint32_t maze_archive_crc32(const uint8_t *bytes,size_t size);
int maze_archive_encode(const maze_archive_t *archive,uint8_t out[MAZE_ARCHIVE_BYTES]);
/* 接受v1至v7，旧各模式榜合并后仅保留可显示前5。 */
int maze_archive_decode(const uint8_t *bytes,size_t size,maze_archive_t *archive);
#endif
