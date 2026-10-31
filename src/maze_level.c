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

/* 独立开放对比场；保留原关卡，正式迷宫布局待手感确认后设计。 */
static const maze_surface_area_t lab_surfaces[]={{{175,0,175,300},MAZE_SURFACE_ICE}};
const maze_level_t maze_level_surface_lab={
    .width=350,.height=300,.ball_radius=7,.start={42,75},.goal={318,268,14},
    .surfaces=lab_surfaces,.surface_count=sizeof(lab_surfaces)/sizeof(lab_surfaces[0])
};

/* 已确认的冰岔迷宫：三处偏置陷阱位于必经通道，冰面构成可选捷径。 */
static const maze_rect_t frost_walls[]={
    {0,46,54,8},
    {96,0,8,54},
    {196,0,8,54},
    {196,46,58,8},
    {246,46,58,8},
    {46,46,8,58},
    {96,46,8,58},
    {146,46,8,58},
    {196,46,8,58},
    {246,96,58,8},
    {96,96,8,58},
    {46,146,58,8},
    {146,96,8,58},
    {146,146,58,8},
    {246,96,8,58},
    {196,146,58,8},
    {246,146,58,8},
    {0,196,54,8},
    {96,146,8,58},
    {146,196,58,8},
    {196,196,58,8},
    {296,146,8,58},
    {46,196,8,58},
    {96,196,8,58},
    {96,246,58,8},
    {246,196,8,58},
    {196,246,58,8},
    {296,196,8,58},
    {296,246,8,54}
};
static const maze_circle_t frost_traps[]={{83,75,8},{183,75,8},{317,175,8}};
static const maze_surface_area_t frost_surfaces[]={{{100,200,50,50},MAZE_SURFACE_ICE},{{150,200,50,50},MAZE_SURFACE_ICE}};
const maze_level_t maze_level_frost_fork={
    .width=350,.height=300,.ball_radius=7,.start={25,25},.goal={325,275,14},
    .walls=frost_walls,.wall_count=sizeof(frost_walls)/sizeof(frost_walls[0]),
    .traps=frost_traps,.trap_count=sizeof(frost_traps)/sizeof(frost_traps[0]),
    .surfaces=frost_surfaces,.surface_count=sizeof(frost_surfaces)/sizeof(frost_surfaces[0])
};

maze_surface_t maze_level_surface_at(const maze_level_t *level, float x, float y)
{
    for (unsigned i=0;i<level->surface_count;i++)
    {
        maze_rect_t a=level->surfaces[i].area;
        if (x>=a.x && x<a.x+a.width && y>=a.y && y<a.y+a.height)
            return level->surfaces[i].surface;
    }
    return MAZE_SURFACE_NORMAL;
}
