/* 第4关：固定支路、锁存门、直线冲墙与检查点后的必经坑道。 */
#include "maze_map.h"
static const char *const lines[]={
    "+-+-+-+-+-+-+-+-+",
    "|. . . ~ ~ . . .|",
    "+ + +-+-+-+-+-+ +",
    "|:|:|. . . . . .|",
    "+ + + +-+-+-+ +-+",
    "|. .|. . .|. . .|",
    "+ +-+ +-+-+-+-+-+",
    "|. .|. . . . . .|",
    "+ +-+-+-+-+-+-+ +",
    "|. . . . .|: . .|",
    "+ +-+-+-+-+ +-+-+",
    "|. .|. . : . . .|",
    "+ +-+ +-+-+-+-+-+",
    "|. .|. . . . . ~|",
    "+ +-+-+-+-+-+-+ +",
    "|. . . . . . .|.|",
    "+-+-+-+-+-+-+-+-+",
};
static const maze_map_object_t objects[]={
    {MAZE_MAP_START,{25,25,7}},
    {MAZE_MAP_GOAL,{375,375,14}},
    {MAZE_MAP_TRAP,{175,283,8}},
    {MAZE_MAP_TRAP,{383,350,8}}
};
static const maze_mechanism_t mechanisms[]={
    {MAZE_MECH_KEY,{65,115,20,20},0},
    {MAZE_MECH_KEY_DOOR,{146,0,8,54},0},
    {MAZE_MECH_SWITCH,{265,115,20,20},0},
    {MAZE_MECH_SWITCH_DOOR,{246,46,8,58},0},
    {MAZE_MECH_BREAKABLE,{296,146,8,58},0},
    {MAZE_MECH_CHECKPOINT,{315,215,20,20},0}
};
const maze_map_definition_t maze_map_mechanism_trial={
    "mechanism_trial",8,8,lines,sizeof(lines)/sizeof(lines[0]),
    objects,sizeof(objects)/sizeof(objects[0]),mechanisms,sizeof(mechanisms)/sizeof(mechanisms[0])
};
