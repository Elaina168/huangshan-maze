#ifndef MAZE_RANDOM_H
#define MAZE_RANDOM_H
#include "maze_map.h"
#include <stdbool.h>
#define MAZE_GENERATOR_VERSION 1
#define MAZE_RANDOM_ID "random_trial"
enum {MAZE_RANDOM_PENDING=1,MAZE_RANDOM_READY=2,MAZE_RANDOM_EXHAUSTED=-2201,MAZE_RANDOM_VERSION=-2202,MAZE_RANDOM_REBUILD=-2203,MAZE_RANDOM_TIMEOUT=-2204,MAZE_RANDOM_REVISION=-2205};
typedef enum {MAZE_TEMPLATE_NONE,MAZE_TEMPLATE_MAGNET,MAZE_TEMPLATE_LASER,MAZE_TEMPLATE_GATE,MAZE_TEMPLATE_PATROL,MAZE_TEMPLATE_KEY,MAZE_TEMPLATE_SWITCH,MAZE_TEMPLATE_DOOR,MAZE_TEMPLATE_CHECKPOINT,MAZE_TEMPLATE_TRAP} maze_template_kind_t;
typedef struct {
 maze_map_definition_t definition;
 char lines[21][26];const char *line_ptrs[21];
 maze_map_object_t objects[MAZE_MAP_MAX_OBJECTS];
 maze_mechanism_t mechanisms[MAZE_MAP_MAX_MECHANISMS];maze_dynamic_t dynamics[MAZE_MAP_MAX_DYNAMICS];
 uint32_t seed;uint8_t path[30],path_count,branches,links[30],types[30],dynamic_index[30];
 char terrain[30];
} maze_generated_t;
typedef struct {
 maze_generated_t *out;uint32_t seed,rng,visited;
 unsigned phase,attempt,depth,cursor;bool exact;
 uint8_t stack[30],parent[30],distance[30],start,goal;
 int result;
} maze_generator_t;
void maze_generator_begin(maze_generator_t*,maze_generated_t*,uint32_t seed,bool exact);
int maze_generator_step(maze_generator_t*,unsigned operations);
int maze_generator_reject(maze_generator_t*);
void maze_generator_cancel(maze_generator_t*);
#endif
