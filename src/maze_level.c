/* 仅保存第一关几何，不持有UI对象或本轮运行数据。 */
#include "maze_level.h"

static const maze_rect_t walls[]={{100,0,12,210},{220,90,12,210}};
static const maze_circle_t traps[]={{55,140,10},{178,278,10},{305,150,12}};

const maze_level_t maze_level_one={
    .width=350, .height=300, .ball_radius=7, .start={24,24},
    .walls=walls, .wall_count=sizeof(walls)/sizeof(walls[0]),
    .traps=traps, .trap_count=sizeof(traps)/sizeof(traps[0]),
    .goal={326,276,14}
};
