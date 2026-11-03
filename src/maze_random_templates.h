#ifndef MAZE_RANDOM_TEMPLATES_H
#define MAZE_RANDOM_TEMPLATES_H
#include "maze_random.h"
/* 模板均在100px宏格内，门户在四边中点；影响包络不越格。 */
maze_point_t maze_random_point(maze_point_t p,unsigned rotation);
maze_rect_t maze_random_rect(maze_rect_t a,unsigned rotation);
void maze_random_dynamic(maze_generated_t*,unsigned node,unsigned rotation,maze_template_kind_t kind);
#endif
