#include "maze_catalog.h"

const maze_catalog_entry_t maze_catalog[]={
    {"冰岔迷宫",&maze_map_frost_fork},
    {"冰弯迷宫",&maze_map_second},
    {"冰环迷宫",&maze_map_ice_loops},
    {"机关试炼",&maze_map_mechanism_trial}
};
const unsigned maze_catalog_count=sizeof(maze_catalog)/sizeof(maze_catalog[0]);
