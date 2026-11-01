/* 地图内容只在此定义；字符网格描述地形/边界，对象表保留精确坐标。 */
#include "maze_map.h"

static const char *const lines[]={
    "+-+-+-+-+-+-+-+",
    "|. .|. .|. . .|",
    "+-+ + + +-+-+ +",
    "|.|.|.|.|. . .|",
    "+ + + + + +-+ +",
    "|. .|.|. .|. .|",
    "+ +-+ +-+-+-+ +",
    "|. .|. . . .|.|",
    "+-+ + +-+-+ + +",
    "|.|.|~ ~ .|.|.|",
    "+ + +-+ +-+ + +",
    "|. . . . . .|.|",
    "+-+-+-+-+-+-+-+"
};
static const maze_map_object_t objects[]={
    {MAZE_MAP_START,{25,25,7}},
    {MAZE_MAP_GOAL,{325,275,14}},
    {MAZE_MAP_TRAP,{83,75,8}},
    {MAZE_MAP_TRAP,{183,75,8}},
    {MAZE_MAP_TRAP,{317,175,8}}
};
const maze_map_definition_t maze_map_frost_fork={
    MAZE_LEVEL_ID_FROST_FORK,7,6,lines,sizeof(lines)/sizeof(lines[0]),
    objects,sizeof(objects)/sizeof(objects[0])
};
