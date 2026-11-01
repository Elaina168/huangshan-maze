/* 通用地面查询，正式关卡由字符串地图装载。 */
#include "maze_level.h"

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
