/* 板端定义装载与静态检查；不依赖RTOS、UI、文件系统或动态分配。 */
#include "maze_map.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
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
            else if (c!='.' && c!='~' && c!=':') return 0;
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

static int mechanism_segment_clear(const maze_level_t *l, float ax, float ay, float bx, float by,
                                   uint32_t keys, uint32_t switches)
{
    if (!segment_clear(l,ax,ay,bx,by)) return 0;
    for (unsigned i=0;i<l->mechanism_count;i++)
    {
        maze_mechanism_t o=l->mechanisms[i];
        bool blocked=o.kind==MAZE_MECH_KEY_DOOR?!(keys&(1u<<o.link)):
                     o.kind==MAZE_MECH_SWITCH_DOOR?!(switches&(1u<<o.link)):false;
        /* 破墙仅作静态可通候选，动力学必经路线另由真实模型验证。 */
        if (blocked && wall_distance_sq(ax,ay,bx,by,o.area)<l->ball_radius*l->ball_radius) return 0;
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

/* 锁存状态只增加；每轮从起点重算闭门区域，直到不再获得钥匙/开关。 */
static int mechanisms_route(maze_map_t *out)
{
    const maze_level_t *l=&out->level;
    uint32_t keys=0,switches=0,reached=0;
    unsigned nx=(unsigned)l->width/MAZE_MAP_CHECK_STEP_PX+1;
    unsigned ny=(unsigned)l->height/MAZE_MAP_CHECK_STEP_PX+1;
    unsigned start=(unsigned)(l->start.y/MAZE_MAP_CHECK_STEP_PX)*nx+(unsigned)(l->start.x/MAZE_MAP_CHECK_STEP_PX);
    out->checked_nodes=0;
    for (unsigned pass=0;pass<=l->mechanism_count;pass++)
    {
        uint32_t next_keys=keys,next_switches=switches;
        bool goal=false;
        unsigned head=0,tail=1;
        memset(work.seen,0,sizeof(work.seen));
        work.queue[0]=(uint16_t)start;work.seen[start/8]|=(uint8_t)(1u<<(start%8));
        while (head<tail)
        {
            unsigned node=work.queue[head++],ix=node%nx,iy=node/nx;
            float x=ix*MAZE_MAP_CHECK_STEP_PX,y=iy*MAZE_MAP_CHECK_STEP_PX;
            out->checked_nodes++;
            if (distance_sq(x,y,l->goal.x,l->goal.y)<=l->goal.radius*l->goal.radius) goal=true;
            for (unsigned i=0;i<l->mechanism_count;i++)
            {
                maze_mechanism_t o=l->mechanisms[i];
                float r=fminf(o.area.width,o.area.height)/2+l->ball_radius;
                if (distance_sq(x,y,o.area.x+o.area.width/2,o.area.y+o.area.height/2)>r*r) continue;
                if (o.kind==MAZE_MECH_KEY) { next_keys|=1u<<o.link;reached|=1u<<i; }
                else if (o.kind==MAZE_MECH_SWITCH) { next_switches|=1u<<o.link;reached|=1u<<i; }
                else if (o.kind==MAZE_MECH_CHECKPOINT) reached|=1u<<i;
            }
            const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
            for (unsigned k=0;k<4;k++)
            {
                int xx=(int)ix+dx[k],yy=(int)iy+dy[k];
                if (xx<0 || yy<0 || (unsigned)xx>=nx || (unsigned)yy>=ny) continue;
                unsigned next=(unsigned)yy*nx+(unsigned)xx;
                if (work.seen[next/8]&(1u<<(next%8))) continue;
                if (!mechanism_segment_clear(l,x,y,xx*MAZE_MAP_CHECK_STEP_PX,yy*MAZE_MAP_CHECK_STEP_PX,keys,switches)) continue;
                work.seen[next/8]|=(uint8_t)(1u<<(next%8));work.queue[tail++]=(uint16_t)next;
            }
        }
        if (next_keys==keys && next_switches==switches)
        {
            if (!goal) return 0;
            for (unsigned i=0;i<l->mechanism_count;i++)
            {
                maze_mechanism_kind_t kind=l->mechanisms[i].kind;
                if ((kind==MAZE_MECH_KEY || kind==MAZE_MECH_SWITCH || kind==MAZE_MECH_CHECKPOINT) && !(reached&(1u<<i))) return 0;
            }
            return 1;
        }
        keys=next_keys;switches=next_switches;
    }
    return 0;
}

static int mechanisms_valid(const maze_level_t *l)
{
    for (unsigned i=0;i<l->mechanism_count;i++)
    {
        maze_mechanism_t o=l->mechanisms[i];maze_rect_t a=o.area;
        if (o.kind<MAZE_MECH_KEY || o.kind>MAZE_MECH_CHECKPOINT || !isfinite(a.x) ||
            !isfinite(a.y) || !isfinite(a.width) || !isfinite(a.height) || a.width<=0 || a.height<=0 ||
            a.x<0 || a.y<0 || a.x+a.width>l->width || a.y+a.height>l->height) return 0;
        if (o.kind!=MAZE_MECH_BREAKABLE && o.kind!=MAZE_MECH_CHECKPOINT && o.link>7) return 0;
        if (o.kind==MAZE_MECH_KEY_DOOR || o.kind==MAZE_MECH_SWITCH_DOOR)
        {
            bool found=false;
            maze_mechanism_kind_t trigger=o.kind==MAZE_MECH_KEY_DOOR?MAZE_MECH_KEY:MAZE_MECH_SWITCH;
            for (unsigned k=0;k<l->mechanism_count;k++)
                if (l->mechanisms[k].kind==trigger && l->mechanisms[k].link==o.link) found=true;
            if (!found) return 0;
        }
        bool solid=o.kind==MAZE_MECH_KEY_DOOR || o.kind==MAZE_MECH_SWITCH_DOOR || o.kind==MAZE_MECH_BREAKABLE;
        if (solid)
        {
            if (wall_distance_sq(l->start.x,l->start.y,l->start.x,l->start.y,a)<l->ball_radius*l->ball_radius ||
                wall_distance_sq(l->goal.x,l->goal.y,l->goal.x,l->goal.y,a)<l->goal.radius*l->goal.radius) return 0;
        }
        else
        {
            maze_circle_t c={a.x+a.width/2,a.y+a.height/2,fminf(a.width,a.height)/2};
            if (o.kind==MAZE_MECH_CHECKPOINT) c.radius=fmaxf(c.radius,l->ball_radius);
            if (!circle_clear(l,c)) return 0;
            for (unsigned k=0;k<l->trap_count;k++) if (circles_overlap(c,l->traps[k])) return 0;
            for (unsigned k=0;k<l->mechanism_count;k++)
            {
                maze_mechanism_t other=l->mechanisms[k];
                if ((other.kind==MAZE_MECH_KEY_DOOR || other.kind==MAZE_MECH_SWITCH_DOOR || other.kind==MAZE_MECH_BREAKABLE) &&
                    wall_distance_sq(c.x,c.y,c.x,c.y,other.area)<c.radius*c.radius) return 0;
            }
        }
    }
    return 1;
}

int maze_map_load(const maze_map_definition_t *d, maze_map_t *out)
{
    if (!format_valid(d)) return MAZE_MAP_FORMAT;
    if (d->object_count>MAZE_MAP_MAX_OBJECTS || d->mechanism_count>MAZE_MAP_MAX_MECHANISMS ||
        (d->mechanism_count && !d->mechanisms)) return MAZE_MAP_OBJECT;
    memset(out,0,sizeof(*out));
    maze_level_t *l=&out->level;
    l->id=d->id;l->width=d->columns*MAZE_MAP_CELL_PX;l->height=d->rows*MAZE_MAP_CELL_PX;
    l->mechanisms=d->mechanisms;l->mechanism_count=d->mechanism_count;
    l->walls=out->walls;l->surfaces=out->surfaces;l->traps=out->traps;
    float half=MAZE_MAP_WALL_PX/2;
    for (unsigned y=0;y<d->rows;y++)
        for (unsigned x=0;x<d->columns;x++)
        {
            float px=x*MAZE_MAP_CELL_PX,py=y*MAZE_MAP_CELL_PX;
            if (d->lines[2*y+1][2*x+1]=='~' || d->lines[2*y+1][2*x+1]==':')
                out->surfaces[l->surface_count++]=(maze_surface_area_t){{px,py,MAZE_MAP_CELL_PX,MAZE_MAP_CELL_PX},
                    d->lines[2*y+1][2*x+1]=='~'?MAZE_SURFACE_ICE:MAZE_SURFACE_ROUGH};
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
    if (!mechanisms_valid(l)) return MAZE_MAP_OBJECT;
    return (l->mechanism_count?mechanisms_route(out):has_route(out))?MAZE_MAP_OK:MAZE_MAP_NO_ROUTE;
}
