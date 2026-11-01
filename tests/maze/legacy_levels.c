#include "legacy_levels.h"

static const maze_rect_t walls[]={{100,0,12,210},{220,90,12,210}};
static const maze_circle_t traps[]={{55,140,10},{178,278,10},{305,150,12}};
const maze_level_t maze_level_one={
    .width=350,.height=300,.ball_radius=7,.start={24,24},
    .walls=walls,.wall_count=sizeof(walls)/sizeof(walls[0]),
    .traps=traps,.trap_count=sizeof(traps)/sizeof(traps[0]),
    .goal={326,276,14},.id="level_one"
};
static const maze_surface_area_t lab_surfaces[]={{{175,0,175,300},MAZE_SURFACE_ICE}};
const maze_level_t maze_level_surface_lab={
    .width=350,.height=300,.ball_radius=7,.start={42,75},.goal={318,268,14},
    .id="surface_lab",.surfaces=lab_surfaces,
    .surface_count=sizeof(lab_surfaces)/sizeof(lab_surfaces[0])
};
