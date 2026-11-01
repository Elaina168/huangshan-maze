/* 使用真实装载器检查定义边界、几何基线和保守路线。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "maze_map.h"
#include "frost_fixture.h"

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
    for (unsigned run=0;run<3;run++)
    {
        assert(maze_map_load(&maze_map_frost_fork,&loaded)==MAZE_MAP_OK);
        const maze_level_t *l=&loaded.level;
        assert(!strcmp(l->id,"frost_fork") && l->width==350 && l->height==300);
        assert(l->ball_radius==7 && l->start.x==25 && l->start.y==25);
        assert(l->goal.x==325 && l->goal.y==275 && l->goal.radius==14);
        assert(l->wall_count==29 && l->surface_count==2 && l->trap_count==3);
        assert(l->walls==loaded.walls && l->traps==loaded.traps && l->surfaces==loaded.surfaces);
        for (unsigned i=0;i<29;i++) assert(!memcmp(&l->walls[i],&expected_walls[i],sizeof(maze_rect_t)));
        for (unsigned i=0;i<3;i++) assert(!memcmp(&l->traps[i],&expected_traps[i],sizeof(maze_circle_t)));
        for (unsigned i=0;i<2;i++)
        {
            assert(!memcmp(&l->surfaces[i].area,&expected_ice[i],sizeof(maze_rect_t)));
            assert(l->surfaces[i].surface==MAZE_SURFACE_ICE);
        }
        assert(loaded.checked_nodes>0 && loaded.checked_nodes<=4331);
    }
    printf("PASS: exact JSON geometry/ID/internal pointers after 3 loads; map=%zu workspace=%zu bytes\n",
           sizeof(loaded),maze_map_workspace_size());
    maze_map_definition_t d=maze_map_frost_fork;
    d.id="";bad(&d,MAZE_MAP_FORMAT);d=maze_map_frost_fork;
    d.columns=0;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_frost_fork;d.columns=8;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_frost_fork;d.rows=7;bad(&d,MAZE_MAP_FORMAT);
    d=maze_map_frost_fork;d.line_count--;bad(&d,MAZE_MAP_FORMAT);
    char rows[13][17];const char *lines[13];
    for (unsigned i=0;i<13;i++) { strcpy(rows[i],maze_map_frost_fork.lines[i]);lines[i]=rows[i]; }
    d=maze_map_frost_fork;d.lines=lines;
    rows[0][0]='?';bad(&d,MAZE_MAP_FORMAT);rows[0][0]='+';
    rows[1][1]='#';bad(&d,MAZE_MAP_FORMAT);rows[1][1]='.';
    rows[0][1]=' ';bad(&d,MAZE_MAP_FORMAT);rows[0][1]='-';
    rows[1][0]=' ';bad(&d,MAZE_MAP_FORMAT);rows[1][0]='|';
    rows[0][14]='\0';bad(&d,MAZE_MAP_FORMAT);rows[0][14]='+';
    rows[0][15]='x';rows[0][16]='\0';bad(&d,MAZE_MAP_FORMAT);rows[0][15]='\0';
    puts("PASS: bounded dimensions, row count/length, position characters and closed boundary");
    maze_map_object_t objects[MAZE_MAP_MAX_OBJECTS];
    memcpy(objects,maze_map_frost_fork.objects,maze_map_frost_fork.object_count*sizeof(*objects));
    d=maze_map_frost_fork;d.objects=objects;
    d.object_count=MAZE_MAP_MAX_OBJECTS+1;bad(&d,MAZE_MAP_OBJECT);
    for (unsigned i=0;i<MAZE_MAP_MAX_OBJECTS;i++) objects[i]=(maze_map_object_t){MAZE_MAP_TRAP,{50,25,1}};
    d.object_count=MAZE_MAP_MAX_OBJECTS;bad(&d,MAZE_MAP_OBJECT);
    memcpy(objects,maze_map_frost_fork.objects,maze_map_frost_fork.object_count*sizeof(*objects));d.object_count=5;
    objects[1].kind=MAZE_MAP_TRAP;bad(&d,MAZE_MAP_OBJECT);objects[1]=maze_map_frost_fork.objects[1];
    objects[2].kind=MAZE_MAP_START;bad(&d,MAZE_MAP_OBJECT);objects[2]=maze_map_frost_fork.objects[2];
    objects[2].kind=(maze_map_object_kind_t)99;bad(&d,MAZE_MAP_OBJECT);objects[2]=maze_map_frost_fork.objects[2];
    for (unsigned i=0;i<5;i++)
    {
        maze_map_object_t old=objects[i];
        objects[i].circle.x=NAN;bad(&d,MAZE_MAP_GEOMETRY);objects[i]=old;
        objects[i].circle.radius=0;bad(&d,MAZE_MAP_GEOMETRY);objects[i]=old;
        objects[i].circle.radius=INFINITY;bad(&d,MAZE_MAP_GEOMETRY);objects[i]=old;
    }
    objects[0].circle.x=24;bad(&d,MAZE_MAP_GEOMETRY);objects[0]=maze_map_frost_fork.objects[0];
    objects[0].circle.radius=22;bad(&d,MAZE_MAP_GEOMETRY);objects[0]=maze_map_frost_fork.objects[0];
    objects[2].circle.x=1;bad(&d,MAZE_MAP_GEOMETRY);objects[2]=maze_map_frost_fork.objects[2];
    objects[1].circle.x=25;objects[1].circle.y=25;bad(&d,MAZE_MAP_GEOMETRY);objects[1]=maze_map_frost_fork.objects[1];
    objects[2].circle=(maze_circle_t){25,50,8};bad(&d,MAZE_MAP_GEOMETRY);objects[2]=maze_map_frost_fork.objects[2];
    objects[2].circle=objects[3].circle;bad(&d,MAZE_MAP_GEOMETRY);
    puts("PASS: missing/duplicate/unknown/over-capacity objects and nonfinite/radius/bounds/overlap/wall rejection");
    for (unsigned y=0;y<13;y++)
        for (unsigned x=0;x<15;x++)
        {
            if (y%2 && x%2) rows[y][x]='~';
            else if (y%2) rows[y][x]='|';
            else if (x%2) rows[y][x]='-';
        }
    d=maze_map_frost_fork;d.lines=lines;bad(&d,MAZE_MAP_NO_ROUTE);
    assert(loaded.level.wall_count==MAZE_MAP_MAX_WALLS && loaded.level.surface_count==MAZE_MAP_MAX_CELLS);
    for (unsigned y=1;y<12;y++)
        for (unsigned x=1;x<14;x++) if ((x%2)!=(y%2)) rows[y][x]=' ';
    assert(maze_map_load(&d,&loaded)==MAZE_MAP_OK);
    assert(loaded.level.wall_count==0 && loaded.level.surface_count==MAZE_MAP_MAX_CELLS);
    puts("PASS: full wall/surface capacities and zero internal walls without guard corruption");
    static const char *const open[]={"+-+-+-+","|. . .|","+-+-+-+"};
    static const char *const closed[]={"+-+-+-+","|.|. .|","+-+-+-+"};
    maze_map_object_t corridor_objects[]={
        {MAZE_MAP_START,{25,25,7}},{MAZE_MAP_GOAL,{125,25,8}},
        {MAZE_MAP_TRAP,{75,25,22}}
    };
    d=(maze_map_definition_t){"test",3,1,open,3,corridor_objects,2};
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
