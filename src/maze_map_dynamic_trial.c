/* 第5关：磁场、等待激光、闸门与可躲避的巡逻块串联。 */
#include "maze_map.h"
static const char *const lines[]={
    "+-+-+-+-+-+-+-+-+-+-+-+-+",
    "|. . . ~ . . . . . . . .|",
    "+ + + + + + + + + + + + +",
    "|. . . ~ . . . . . . . .|",
    "+-+-+-+-+-+-+-+-+-+-+ + +",
    "|.|.|.|.|.|. . .|.|.|. .|",
    "+-+-+-+-+-+-+-+-+-+-+ + +",
    "|. . . . . . ~ . . . . .|",
    "+ + + + + + + + + + + + +",
    "|. . . . . . ~ . . . . .|",
    "+-+-+ + + +-+-+-+-+-+-+-+",
    "|.|.|. . .|.|.|.|.|.|.|.|",
    "+-+-+ + + +-+-+-+-+-+-+-+",
    "|.|.|. . . . . . : . .|.|",
    "+-+-+ + + + + + + + + +-+",
    "|.|.|. . . . . . : . .|.|",
    "+-+-+-+-+-+-+-+-+-+ + +-+",
    "|.|.|.|.|.|.|.|.|.|. .|.|",
    "+-+-+-+-+-+-+-+-+-+ + +-+",
    "|.|.|.|.|.|.|.|.|.|. .|.|",
    "+-+-+-+-+-+-+-+-+-+-+-+-+",
};
static const maze_map_object_t objects[]={
    {MAZE_MAP_START,{25,25,7}}, {MAZE_MAP_GOAL,{525,475,14}},
    {MAZE_MAP_TRAP,{250,383,8}}, {MAZE_MAP_TRAP,{483,400,8}}
};
static const maze_mechanism_t mechanisms[]={
    {MAZE_MECH_CHECKPOINT,{140,185,20,20},0},
    {MAZE_MECH_CHECKPOINT,{430,360,20,20},0}
};
static const maze_dynamic_t dynamics[]={
    {.kind=MAZE_DYNAMIC_MAGNET,.area={140,40,20,20},.force=240,.outer_radius=70,.inner_radius=12},
    {.kind=MAZE_DYNAMIC_LASER,.area={240,4,4,92},.off_ms=1800,.warning_ms=600,.on_ms=1200},
    {.kind=MAZE_DYNAMIC_GATE,.area={396,154,8,92},.off_ms=2400,.warning_ms=600,.on_ms=1800},
    {.kind=MAZE_DYNAMIC_PATROL,.area={285,320,22,22},.end={385,320},.speed=35,.dwell_ms=400},
    {.kind=MAZE_DYNAMIC_LASER,.area={454,430,92,4},.off_ms=1800,.warning_ms=600,.on_ms=1200,.phase_ms=1000}
};
const maze_map_definition_t maze_map_dynamic_trial={
    "dynamic_trial",12,10,lines,sizeof(lines)/sizeof(lines[0]),
    objects,sizeof(objects)/sizeof(objects[0]),mechanisms,sizeof(mechanisms)/sizeof(mechanisms[0]),
    dynamics,sizeof(dynamics)/sizeof(dynamics[0])
};
