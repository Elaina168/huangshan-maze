/* 使用真实装载器检查定义边界、几何基线和保守路线。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "maze_map.h"


static struct { maze_map_t map; unsigned guard[8]; } storage;
#define loaded storage.map
static void bad(const maze_map_definition_t *def, int expected)
{
    for (unsigned i=0;i<8;i++) storage.guard[i]=0xa5a5a5a5u;
    int result=maze_map_load(def,&loaded);
    for (unsigned i=0;i<8;i++) assert(storage.guard[i]==0xa5a5a5a5u);
    if (result!=expected) printf("expected %d, got %d\n",expected,result);
    assert(result==expected);
}
int main(void)
{
    setbuf(stdout,NULL);
    maze_map_definition_t d=maze_map_ice_loops;
    d.id="";bad(&d,MAZE_MAP_FORMAT);d=maze_map_ice_loops;
    d.columns=0;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_ice_loops;d.columns=15;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_ice_loops;d.rows=13;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_ice_loops;d.line_count--;bad(&d,MAZE_MAP_FORMAT);
    /* 使用当前地图自身容量，避免把旧7×6尺寸误用于14×12地图。 */
    char rows[25][30];const char *lines[25];
    for(unsigned i=0;i<25;i++){strcpy(rows[i],maze_map_ice_loops.lines[i]);lines[i]=rows[i];}
    d=maze_map_ice_loops;d.lines=lines;rows[0][0]='?';bad(&d,MAZE_MAP_FORMAT);rows[0][0]='+';
    rows[1][1]='#';bad(&d,MAZE_MAP_FORMAT);rows[1][1]='.';
    rows[0][1]=' ';bad(&d,MAZE_MAP_FORMAT);rows[0][1]='-';
    rows[0][28]=0;bad(&d,MAZE_MAP_FORMAT);rows[0][28]='+';
    maze_map_object_t objects[MAZE_MAP_MAX_OBJECTS];
    memcpy(objects,maze_map_ice_loops.objects,maze_map_ice_loops.object_count*sizeof(*objects));
    d=maze_map_ice_loops;d.objects=objects;d.object_count=MAZE_MAP_MAX_OBJECTS+1;bad(&d,MAZE_MAP_OBJECT);d.object_count=maze_map_ice_loops.object_count;
    objects[0].circle.x=NAN;bad(&d,MAZE_MAP_GEOMETRY);objects[0]=maze_map_ice_loops.objects[0];
    objects[0].circle.radius=0;bad(&d,MAZE_MAP_GEOMETRY);objects[0]=maze_map_ice_loops.objects[0];
    objects[0].circle.x=1;bad(&d,MAZE_MAP_GEOMETRY);objects[0]=maze_map_ice_loops.objects[0];
    objects[1].circle=objects[0].circle;bad(&d,MAZE_MAP_GEOMETRY);
    puts("PASS: current-map dimensions/row characters/object capacity/nonfinite/radius/bounds/overlap and guard checks");
    static char full_rows[2*MAZE_MAP_MAX_ROWS+1][2*MAZE_MAP_MAX_COLUMNS+2];
    static const char *full_lines[2*MAZE_MAP_MAX_ROWS+1];
    for(unsigned y=0;y<=2*MAZE_MAP_MAX_ROWS;y++)
    {
        full_lines[y]=full_rows[y];
        for(unsigned x=0;x<=2*MAZE_MAP_MAX_COLUMNS;x++)
            full_rows[y][x]=!(x%2) && !(y%2)?'+':(y%2?(x%2?'~':'|'):'-');
        full_rows[y][2*MAZE_MAP_MAX_COLUMNS+1]=0;
    }
    maze_map_object_t full_objects[]={{MAZE_MAP_START,{25,25,7}},{MAZE_MAP_GOAL,{675,575,14}}};
    d=maze_map_ice_loops;d.objects=full_objects;d.object_count=2;d.columns=MAZE_MAP_MAX_COLUMNS;d.rows=MAZE_MAP_MAX_ROWS;
    d.lines=full_lines;d.line_count=2*MAZE_MAP_MAX_ROWS+1;bad(&d,MAZE_MAP_NO_ROUTE);
    assert(loaded.level.wall_count==MAZE_MAP_MAX_WALLS && loaded.level.surface_count==MAZE_MAP_MAX_CELLS);
    for(unsigned y=1;y<2*MAZE_MAP_MAX_ROWS;y++)
        for(unsigned x=1;x<2*MAZE_MAP_MAX_COLUMNS;x++)
            if((x%2)!=(y%2)) full_rows[y][x]=' ';
    assert(maze_map_load(&d,&loaded)==MAZE_MAP_OK);
    assert(loaded.level.wall_count==0 && loaded.level.surface_count==MAZE_MAP_MAX_CELLS);
    puts("PASS: full 14x12 wall/surface capacities and zero internal walls without guard corruption");
    static const char *const open[]={"+-+-+-+","|. . .|","+-+-+-+"};
    static const char *const closed[]={"+-+-+-+","|.|. .|","+-+-+-+"};
    maze_map_object_t corridor_objects[]={
        {MAZE_MAP_START,{25,25,7}},{MAZE_MAP_GOAL,{125,25,8}},
        {MAZE_MAP_TRAP,{75,25,22}}
    };
    d=(maze_map_definition_t){"test",3,1,open,3,corridor_objects,2,NULL,0};
    assert(maze_map_load(&d,&loaded)==MAZE_MAP_OK);
    d.lines=closed;bad(&d,MAZE_MAP_NO_ROUTE);d.lines=open;
    d.object_count=3;bad(&d,MAZE_MAP_NO_ROUTE);
    corridor_objects[2].circle.radius=8;
    assert(maze_map_load(&d,&loaded)==MAZE_MAP_OK);
    /* 三条格点通路的边中间各有细坑，端点均在坑外，不能跳过边段检查。 */
    maze_map_object_t tiny[]={
        {MAZE_MAP_START,{25,25,20}},{MAZE_MAP_GOAL,{125,25,8}},
        {MAZE_MAP_TRAP,{47.5f,20,1}},{MAZE_MAP_TRAP,{47.5f,25,1}},{MAZE_MAP_TRAP,{47.5f,30,1}}
    };
    d.objects=tiny;d.object_count=5;bad(&d,MAZE_MAP_NO_ROUTE);
    puts("PASS: disconnected corridor, blocking pit, safe bypass, and between-node pit crossings");
    return 0;
}
