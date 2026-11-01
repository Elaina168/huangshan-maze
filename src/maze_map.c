/* 板端定义装载与静态检查；不依赖RTOS、UI、文件系统或动态分配。 */
#include "maze_map.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define CHECK_COLUMNS (MAZE_MAP_MAX_COLUMNS*MAZE_MAP_CELL_PX/MAZE_MAP_CHECK_STEP_PX+1)
#define CHECK_ROWS (MAZE_MAP_MAX_ROWS*MAZE_MAP_CELL_PX/MAZE_MAP_CHECK_STEP_PX+1)
#define CHECK_NODES (CHECK_COLUMNS*CHECK_ROWS)
/* 大于主线程栈的工作区必须是静态存储，不进入任何线程局部栈。 */
static struct
{
    uint16_t queue[CHECK_NODES];
    uint8_t seen[(CHECK_NODES+7)/8];
} work;

size_t maze_map_workspace_size(void) { return sizeof(work); }

static float distance_sq(float ax, float ay, float bx, float by)
{
    float dx=ax-bx,dy=ay-by;
    return dx*dx+dy*dy;
}

/* 水平/竖直线段到矩形的最短距离；点是长度为零的线段。 */
static float wall_distance_sq(float ax, float ay, float bx, float by, maze_rect_t w)
{
    float dx=fmaxf(fmaxf(w.x-fmaxf(ax,bx),fminf(ax,bx)-w.x-w.width),0);
    float dy=fmaxf(fmaxf(w.y-fmaxf(ay,by),fminf(ay,by)-w.y-w.height),0);
    return dx*dx+dy*dy;
}

static int format_valid(const maze_map_definition_t *d)
{
    if (!d->id[0] || !d->columns || !d->rows || d->columns>MAZE_MAP_MAX_COLUMNS ||
        d->rows>MAZE_MAP_MAX_ROWS || d->line_count!=2*d->rows+1) return 0;
    unsigned width=2*d->columns+1;
    for (unsigned y=0;y<d->line_count;y++)
    {
        if (strlen(d->lines[y])!=width) return 0;
        for (unsigned x=0;x<width;x++)
        {
            char c=d->lines[y][x];
            if (!(x%2) && !(y%2)) { if (c!='+') return 0; }
            else if (!(y%2))
            {
                if (c!='-' && c!=' ') return 0;
                if ((y==0 || y+1==d->line_count) && c!='-') return 0;
            }
            else if (!(x%2))
            {
                if (c!='|' && c!=' ') return 0;
                if ((x==0 || x+1==width) && c!='|') return 0;
            }
            else if (c!='.' && c!='~') return 0;
        }
    }
    return 1;
}

static int circle_clear(const maze_level_t *l, maze_circle_t c)
{
    if (!isfinite(c.x) || !isfinite(c.y) || !isfinite(c.radius) || c.radius<=0 ||
        c.x<c.radius || c.y<c.radius || c.x+c.radius>l->width || c.y+c.radius>l->height) return 0;
    for (unsigned i=0;i<l->wall_count;i++)
        if (wall_distance_sq(c.x,c.y,c.x,c.y,l->walls[i])<c.radius*c.radius) return 0;
    return 1;
}

static int circles_overlap(maze_circle_t a, maze_circle_t b)
{
    float r=a.radius+b.radius;
    return distance_sq(a.x,a.y,b.x,b.y)<=r*r;
}

static int centered(maze_circle_t c)
{
    return fmodf(c.x,MAZE_MAP_CELL_PX)==MAZE_MAP_CELL_PX/2 &&
           fmodf(c.y,MAZE_MAP_CELL_PX)==MAZE_MAP_CELL_PX/2;
}

static int segment_clear(const maze_level_t *l, float ax, float ay, float bx, float by)
{
    float r=l->ball_radius;
    if (fminf(ax,bx)<r || fminf(ay,by)<r || fmaxf(ax,bx)>l->width-r || fmaxf(ay,by)>l->height-r) return 0;
    for (unsigned i=0;i<l->wall_count;i++)
        if (wall_distance_sq(ax,ay,bx,by,l->walls[i])<r*r) return 0;
    for (unsigned i=0;i<l->trap_count;i++)
    {
        maze_circle_t t=l->traps[i];
        float x=fminf(fmaxf(t.x,fminf(ax,bx)),fmaxf(ax,bx));
        float y=fminf(fmaxf(t.y,fminf(ay,by)),fmaxf(ay,by));
        /* 保留球心落坑且含边界的产品规则，不让BFS跨过格点之间的细坑。 */
        if (distance_sq(x,y,t.x,t.y)<=t.radius*t.radius) return 0;
    }
    return 1;
}

static int has_route(maze_map_t *out)
{
    const maze_level_t *l=&out->level;
    unsigned nx=(unsigned)l->width/MAZE_MAP_CHECK_STEP_PX+1;
    unsigned ny=(unsigned)l->height/MAZE_MAP_CHECK_STEP_PX+1;
    unsigned start=(unsigned)(l->start.y/MAZE_MAP_CHECK_STEP_PX)*nx+(unsigned)(l->start.x/MAZE_MAP_CHECK_STEP_PX);
    unsigned head=0,tail=1;
    memset(work.seen,0,sizeof(work.seen));
    work.queue[0]=(uint16_t)start;work.seen[start/8]|=(uint8_t)(1u<<(start%8));
    out->checked_nodes=0;
    while (head<tail)
    {
        unsigned node=work.queue[head++],ix=node%nx,iy=node/nx;
        float x=ix*MAZE_MAP_CHECK_STEP_PX,y=iy*MAZE_MAP_CHECK_STEP_PX;
        out->checked_nodes++;
        if (distance_sq(x,y,l->goal.x,l->goal.y)<=l->goal.radius*l->goal.radius) return 1;
        const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
        for (unsigned k=0;k<4;k++)
        {
            int xx=(int)ix+dx[k],yy=(int)iy+dy[k];
            if (xx<0 || yy<0 || (unsigned)xx>=nx || (unsigned)yy>=ny) continue;
            unsigned next=(unsigned)yy*nx+(unsigned)xx;
            if (work.seen[next/8]&(1u<<(next%8))) continue;
            if (!segment_clear(l,x,y,xx*MAZE_MAP_CHECK_STEP_PX,yy*MAZE_MAP_CHECK_STEP_PX)) continue;
            work.seen[next/8]|=(uint8_t)(1u<<(next%8));
            work.queue[tail++]=(uint16_t)next;
        }
    }
    return 0;
}

int maze_map_load(const maze_map_definition_t *d, maze_map_t *out)
{
    if (!format_valid(d)) return MAZE_MAP_FORMAT;
    if (d->object_count>MAZE_MAP_MAX_OBJECTS) return MAZE_MAP_OBJECT;
    memset(out,0,sizeof(*out));
    maze_level_t *l=&out->level;
    l->id=d->id;l->width=d->columns*MAZE_MAP_CELL_PX;l->height=d->rows*MAZE_MAP_CELL_PX;
    l->walls=out->walls;l->surfaces=out->surfaces;l->traps=out->traps;
    float half=MAZE_MAP_WALL_PX/2;
    for (unsigned y=0;y<d->rows;y++)
        for (unsigned x=0;x<d->columns;x++)
        {
            float px=x*MAZE_MAP_CELL_PX,py=y*MAZE_MAP_CELL_PX;
            if (d->lines[2*y+1][2*x+1]=='~')
                out->surfaces[l->surface_count++]=(maze_surface_area_t){{px,py,MAZE_MAP_CELL_PX,MAZE_MAP_CELL_PX},MAZE_SURFACE_ICE};
            if (x+1<d->columns && d->lines[2*y+1][2*x+2]=='|')
            {
                float lo=fmaxf(0,py-half),hi=fminf(l->height,py+MAZE_MAP_CELL_PX+half);
                out->walls[l->wall_count++]=(maze_rect_t){px+MAZE_MAP_CELL_PX-half,lo,MAZE_MAP_WALL_PX,hi-lo};
            }
            if (y+1<d->rows && d->lines[2*y+2][2*x+1]=='-')
            {
                float lo=fmaxf(0,px-half),hi=fminf(l->width,px+MAZE_MAP_CELL_PX+half);
                out->walls[l->wall_count++]=(maze_rect_t){lo,py+MAZE_MAP_CELL_PX-half,hi-lo,MAZE_MAP_WALL_PX};
            }
        }
    unsigned starts=0,goals=0;
    maze_circle_t start={0};
    for (unsigned i=0;i<d->object_count;i++)
    {
        maze_map_object_t o=d->objects[i];
        switch (o.kind)
        {
        case MAZE_MAP_START: start=o.circle;starts++;break;
        case MAZE_MAP_GOAL: l->goal=o.circle;goals++;break;
        case MAZE_MAP_TRAP:
            if (l->trap_count==MAZE_MAP_MAX_CELLS) return MAZE_MAP_OBJECT;
            out->traps[l->trap_count++]=o.circle;break;
        default: return MAZE_MAP_OBJECT;
        }
    }
    if (starts!=1 || goals!=1) return MAZE_MAP_OBJECT;
    l->start=(maze_point_t){start.x,start.y};l->ball_radius=start.radius;
    if (!circle_clear(l,start) || !circle_clear(l,l->goal) || !centered(start) || !centered(l->goal) ||
        2*start.radius>MAZE_MAP_CELL_PX-MAZE_MAP_WALL_PX || circles_overlap(start,l->goal)) return MAZE_MAP_GEOMETRY;
    for (unsigned i=0;i<l->trap_count;i++)
    {
        maze_circle_t t=l->traps[i];
        if (!circle_clear(l,t) || circles_overlap(start,t) || circles_overlap(l->goal,t)) return MAZE_MAP_GEOMETRY;
        for (unsigned k=0;k<i;k++) if (circles_overlap(t,l->traps[k])) return MAZE_MAP_GEOMETRY;
    }
    return has_route(out)?MAZE_MAP_OK:MAZE_MAP_NO_ROUTE;
}
