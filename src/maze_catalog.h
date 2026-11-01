#ifndef MAZE_CATALOG_H
#define MAZE_CATALOG_H
#include "maze_map.h"

/* 目录顺序决定下一关，名称供UI直接显示。 */
typedef struct
{
    const char *title;
    const maze_map_definition_t *definition;
} maze_catalog_entry_t;
extern const maze_catalog_entry_t maze_catalog[];
extern const unsigned maze_catalog_count;
#endif
