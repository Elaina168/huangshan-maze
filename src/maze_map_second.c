/* 已确认冰弯迷宫，几何源自布局设计，不含运行状态。 */
#include "maze_map.h"
static const char *const lines[]={
    "+-+-+-+-+-+-+-+",
    "|.|. . . .|. .|",
    "+ +-+ +-+ +-+ +",
    "|. .|.|. . . .|",
    "+-+ + +-+-+-+ +",
    "|. .|.|. . .|.|",
    "+ +-+ + +-+ + +",
    "|~ ~|.|.|~ ~|.|",
    "+-+ + + + +-+ +",
    "|.|.|. .|.|. .|",
    "+ + +-+-+ + +-+",
    "|. . . . .|. .|",
    "+-+-+-+-+-+-+-+"
};
static const maze_map_object_t objects[]={
    {MAZE_MAP_START,{25,25,7}},
    {MAZE_MAP_GOAL,{325,275,14}},
    {MAZE_MAP_TRAP,{67,225,8}},
    {MAZE_MAP_TRAP,{225,117,8}},
    {MAZE_MAP_TRAP,{133,125,8}},
    {MAZE_MAP_TRAP,{275,83,8}}
};
const maze_map_definition_t maze_map_second={
    "ice_bends",7,6,lines,sizeof(lines)/sizeof(lines[0]),
    objects,sizeof(objects)/sizeof(objects[0]),NULL,0
};
