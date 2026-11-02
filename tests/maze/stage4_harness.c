/* 第4阶段真实模型与地图验收。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "maze_app.h"
#include "maze_map.h"
static void basic(void)
{
    maze_mechanism_t objects[]={
        {MAZE_MECH_KEY,{30,45,10,10},0},
        {MAZE_MECH_KEY_DOOR,{60,0,8,100},0},
        {MAZE_MECH_CHECKPOINT,{90,45,10,10},0}};
    maze_level_t l={.width=300,.height=100,.ball_radius=5,.start={10,50},
        .goal={280,50,5},.mechanisms=objects,.mechanism_count=3};
    maze_model_t m; maze_model_reset(&m,&l);
    m.vx=180; maze_model_step(&m,8192,0,100);
    maze_model_step(&m,8192,0,100);
    assert(m.keys==1);
    for (unsigned i=0;i<4;i++) maze_model_step(&m,8192,0,100);
    assert(m.x>68 && m.checkpoint==3);
    m.result=MAZE_MODEL_LOST;
    assert(maze_model_respawn(&m));
    assert(m.x==95 && m.y==50 && m.keys==1 && m.vx==0);
    maze_model_reset(&m,&l); assert(!m.keys && !m.checkpoint);
    puts("PASS stage4 core");

}

static maze_level_t empty(void)
{
    return (maze_level_t){.width=500,.height=200,.ball_radius=5,
        .start={25,75},.goal={475,175,5}};
}
static void physics(void)
{
    maze_level_t l=empty(); maze_model_t m;
    maze_surface_area_t surface={{0,0,500,200},MAZE_SURFACE_ROUGH};
    l.surfaces=&surface;l.surface_count=1;
    maze_model_reset(&m,&l);m.vx=100;maze_model_step(&m,0,0,100);
    assert(fabsf(m.vx-100/1.9f)<0.001f);
    float rough=m.vx; surface.surface=MAZE_SURFACE_NORMAL;
    maze_model_reset(&m,&l);m.vx=100;maze_model_step(&m,0,0,100);assert(m.vx>rough);
    float normal=m.vx;surface.surface=MAZE_SURFACE_ICE;
    maze_model_reset(&m,&l);m.vx=100;maze_model_step(&m,0,0,100);assert(m.vx>normal);
    surface.surface=MAZE_SURFACE_ROUGH;maze_model_step(&m,0,0,10);assert(m.vx>80);
    l.surfaces=NULL;l.surface_count=0;
    maze_mechanism_t wall={MAZE_MECH_BREAKABLE,{60,0,8,200},0};
    l.mechanisms=&wall;l.mechanism_count=1;
    maze_model_reset(&m,&l);m.vx=50;maze_model_step(&m,0,0,1000);
    assert(!m.broken && m.x<=55);
    maze_model_reset(&m,&l);m.x=54;m.vx=20;m.vy=170;maze_model_step(&m,0,0,100);
    assert(!m.broken && m.vx==0 && m.vy>0);
    maze_model_reset(&m,&l);m.x=54;m.vx=180;maze_model_step(&m,8192,0,100);
    assert(m.broken==1 && m.x>60 && fabsf(m.vx-117)<0.01f);
    wall.kind=MAZE_MECH_KEY_DOOR;
    maze_model_reset(&m,&l);m.x=54;m.vx=180;maze_model_step(&m,8192,0,100);
    assert(m.x==55 && m.vx==0);m.keys=1;maze_model_step(&m,8192,0,100);assert(m.x>55);
    wall.kind=MAZE_MECH_SWITCH_DOOR;
    maze_model_reset(&m,&l);m.x=54;m.vx=180;maze_model_step(&m,8192,0,100);
    assert(m.x==55 && m.vx==0);m.switches=1;maze_model_step(&m,8192,0,100);assert(m.x>55);
    wall.kind=MAZE_MECH_BREAKABLE;
    maze_rect_t regular=wall.area;l.walls=&regular;l.wall_count=1;
    maze_model_reset(&m,&l);m.x=54;m.vx=180;maze_model_step(&m,8192,0,100);
    assert(m.x==55 && !m.broken && m.vx==0);
    puts("PASS rough continuity; slow/grazing impacts; real break and ordinary wall");
}
static void order_and_snapshot(void)
{
    maze_mechanism_t objects[]={
        {MAZE_MECH_CHECKPOINT,{40,65,20,20},0},
        {MAZE_MECH_KEY,{80,65,20,20},0},
        {MAZE_MECH_SWITCH,{120,65,20,20},1},
        {MAZE_MECH_BREAKABLE,{160,0,8,200},0}};
    maze_level_t l=empty();l.mechanisms=objects;l.mechanism_count=4;
    maze_model_t m;maze_model_reset(&m,&l);
    for(unsigned i=0;i<12;i++) maze_model_step(&m,8192,0,100);
    assert(m.checkpoint==1 && m.keys==1 && m.switches==2 && m.broken==8);
    assert(m.checkpoint_keys==0 && m.checkpoint_switches==0 && m.checkpoint_broken==0);
    m.result=MAZE_MODEL_LOST;assert(maze_model_respawn(&m));
    assert(!m.keys && !m.switches && !m.broken && m.x==50);
    m.keys=1;maze_model_step(&m,0,0,10);assert(m.checkpoint_keys==0);
    maze_circle_t pit={65,75,5};l.traps=&pit;l.trap_count=1;
    maze_model_reset(&m,&l);m.vx=180;maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_LOST && !m.keys && m.checkpoint==1);
    assert(m.x<objects[1].area.x);
    /* 同时触发时先落坑，禁止获得坑后的钥匙。 */
    objects[1].area=(maze_rect_t){65,70,10,10};
    maze_model_reset(&m,&l);m.vx=180;maze_model_step(&m,8192,0,1000);
    assert(m.result==MAZE_MODEL_LOST && !m.keys);
    puts("PASS swept chronology, checkpoint snapshot rollback and no resnapshot");
}
static void app_respawn(void)
{
    maze_mechanism_t cp={MAZE_MECH_CHECKPOINT,{40,65,20,20},0};
    maze_level_t l=empty();l.mechanisms=&cp;l.mechanism_count=1;
    maze_app_t a;maze_app_init(&a,&l,1000);
    a.model.checkpoint=1;a.model.checkpoint_keys=2;a.model.keys=3;
    a.model.result=MAZE_MODEL_LOST;a.state=MAZE_APP_LOST;a.elapsed_ms=1200;
    unsigned generation=a.generation;
    maze_app_event(&a,MAZE_EVENT_RESPAWN,100);
    assert(a.generation==generation+1 && a.state==MAZE_APP_CALIBRATING && a.elapsed_ms==0);
    assert(a.model.keys==2 && a.model.x==50);
    maze_imu_sample_t old={.generation=generation,.sequence=1,.read_start_tick=100};
    maze_app_sample(&a,&old,100);assert(a.window_count==0);
    for (unsigned i=0;i<MAZE_CALIBRATION_COUNT;i++)
    {
        maze_imu_sample_t sample={.generation=a.generation,.sequence=i+1,.read_start_tick=100+i*50};
        maze_app_sample(&a,&sample,100+i*50);
    }
    assert(a.state==MAZE_APP_PLAYING && a.model.keys==2 && a.elapsed_ms==0);
    a.state=MAZE_APP_LOST;a.model.result=MAZE_MODEL_LOST;
    maze_app_event(&a,MAZE_EVENT_RESPAWN,2000);maze_app_update(&a,5000);
    assert(a.state==MAZE_APP_CALIBRATION_FAILED && a.model.keys==2);
    maze_app_event(&a,MAZE_EVENT_RETRY,5000);assert(a.model.keys==2);
    maze_app_event(&a,MAZE_EVENT_EXIT,5000);assert(a.state==MAZE_APP_READY && !a.model.keys);
    a.state=MAZE_APP_LOST;maze_app_event(&a,MAZE_EVENT_RESPAWN,6000);assert(a.state==MAZE_APP_LOST);
    a.model.checkpoint=1;maze_app_event(&a,MAZE_EVENT_REPLAY,6000);assert(!a.model.checkpoint && !a.model.keys);
    puts("PASS respawn generation, calibration failure/retry, stale sample, exit/replay");
}
static maze_map_t map;
static void validation(void)
{
    assert(maze_map_load(&maze_map_mechanism_trial,&map)==MAZE_MAP_OK);
    assert(map.level.mechanism_count==6 && map.level.width==400 && map.level.height==400);
    static const char *const lines[]={"+-+-+","|. .|","+-+-+"};
    maze_map_object_t objects[]={{MAZE_MAP_START,{25,25,7}},{MAZE_MAP_GOAL,{75,25,8}}};
    maze_mechanism_t mech[]={{MAZE_MECH_KEY,{65,15,20,20},0},
        {MAZE_MECH_KEY_DOOR,{46,0,8,50},0}};
    maze_map_definition_t d={"locked",2,1,lines,3,objects,2,mech,2};
    assert(maze_map_load(&d,&map)==MAZE_MAP_NO_ROUTE);
    mech[1].link=1;assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
    mech[1].link=8;assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
    mech[1].link=0;mech[0].area.x=-1;assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
    mech[0].area.x=65;d.mechanism_count=17;assert(maze_map_load(&d,&map)==MAZE_MAP_OBJECT);
    assert(maze_map_load(&maze_map_mechanism_trial,&map)==MAZE_MAP_OK);
    /* 逐一封闭坑所在必经通道的出口，必须使全图无解，禁止死胡同摆坑。 */
    char blocked[17][18];const char *rows[17];
    for(unsigned i=0;i<17;i++) { strcpy(blocked[i],maze_map_mechanism_trial.lines[i]);rows[i]=blocked[i]; }
    d=maze_map_mechanism_trial;d.lines=rows;d.object_count=2;
    blocked[11][8]='|';assert(maze_map_load(&d,&map)==MAZE_MAP_NO_ROUTE);
    strcpy(blocked[11],maze_map_mechanism_trial.lines[11]);
    blocked[14][15]='-';assert(maze_map_load(&d,&map)==MAZE_MAP_NO_ROUTE);
    assert(maze_map_load(&maze_map_mechanism_trial,&map)==MAZE_MAP_OK);
    puts("PASS map bounds, references, capacity, self-locked reachability; both trap corridors mandatory");
}
static void move_to(maze_model_t *m,float x,float y)
{
    for (unsigned i=0;i<3000;i++)
    {
        float ex=x-m->x,ey=y-m->y;
        if (hypotf(ex,ey)<1.3f && hypotf(m->vx,m->vy)<4) return;
        maze_surface_t s=maze_level_surface_at(m->level,m->x,m->y);
        float drag=s==MAZE_SURFACE_ICE?MAZE_DRAG_ICE:s==MAZE_SURFACE_ROUGH?MAZE_DRAG_ROUGH:MAZE_DRAG_NORMAL;
        float ax=12*ex+(drag-7)*m->vx,ay=12*ey+(drag-7)*m->vy;
        maze_model_step(m,ax*MAZE_INPUT_FULL_RAW/MAZE_ACCEL_MAX,ay*MAZE_INPUT_FULL_RAW/MAZE_ACCEL_MAX,10);
        if(m->result!=MAZE_MODEL_RUNNING) return;
    }
    fprintf(stderr,"stuck target %.0f %.0f at %.2f %.2f keys%u switches%u broken%u\n",x,y,m->x,m->y,m->keys,m->switches,m->broken);
    assert(0);
}
static void formal_route(void)
{
    assert(maze_map_load(&maze_map_mechanism_trial,&map)==MAZE_MAP_OK);
    maze_model_t m;maze_model_reset(&m,&map.level);
    static const maze_point_t before[]={
        {75,25},{75,125},{75,25},{375,25},{375,75},{325,75},{325,125},{275,125},
        {325,125},{325,75},{125,75},{125,175},{375,175},{375,225},{325,225},
        {275,225},{275,275},{225,275},{175,283}};
    for(unsigned i=0;i<sizeof(before)/sizeof(before[0]);i++) move_to(&m,before[i].x,before[i].y);
    assert(m.result==MAZE_MODEL_LOST && m.checkpoint==6 && m.keys==1 && m.switches==1 && m.broken==16);
    assert(maze_model_respawn(&m));
    assert(m.x==325 && m.y==225 && m.keys==1 && m.switches==1 && m.broken==16);
    static const maze_point_t after[]={
        {275,225},{275,275},{225,275},{225,265},{125,265},{125,325},{375,325},{383,350}};
    for(unsigned i=0;i<sizeof(after)/sizeof(after[0]);i++) move_to(&m,after[i].x,after[i].y);
    assert(m.result==MAZE_MODEL_LOST);
    assert(maze_model_respawn(&m));
    static const maze_point_t safe[]={
        {275,225},{275,275},{225,275},{225,265},{125,265},{125,325},
        {365,325},{365,365},{375,375}};
    for(unsigned i=0;i<sizeof(safe)/sizeof(safe[0]);i++) move_to(&m,safe[i].x,safe[i].y);
    assert(m.result==MAZE_MODEL_WON);
    puts("PASS real fourth-map trajectory: key branch, mandatory key/switch doors, speed break, both mandatory pit losses, checkpoint respawns and win");
}
int main(void)
{
    setbuf(stdout,NULL);basic();physics();order_and_snapshot();app_respawn();validation();formal_route();
    return 0;
}
