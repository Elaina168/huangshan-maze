/* 动态机关行为测试，直接调用产品模型。 */
#include "maze_model.h"
#include "maze_app.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static maze_level_t level(maze_dynamic_t *d) {
    return (maze_level_t){.width=600,.height=500,.ball_radius=7,.start={70,100},
        .goal={570,470,10},.dynamics=d,.dynamic_count=1};
}
int main(void) {
    maze_dynamic_t d={.kind=MAZE_DYNAMIC_MAGNET,.area={90,90,20,20},
        .force=240,.outer_radius=70,.inner_radius=12};
    maze_level_t l=level(&d);maze_model_t m;maze_model_reset(&m,&l);
    maze_model_step(&m,0,0,10);assert(m.vx>0 && m.x>70);
    m.x=100;m.y=100;m.vx=m.vy=0;maze_model_step(&m,0,0,10);
    assert(m.vx==0 && m.vy==0);
    m.x=170;m.vx=0;maze_model_step(&m,0,0,10);assert(m.vx==0);
    m.x=70;for(int i=0;i<200;i++) maze_model_step(&m,-8192,0,10);
    assert(m.x<30);
    maze_model_reset(&m,&l);m.x=112;maze_model_step(&m,0,0,1);
    assert(fabsf(m.vx+0.240f/1.003f)<0.00001f);
    maze_dynamic_t magnets[2]={d,d};l.dynamics=magnets;l.dynamic_count=2;
    maze_model_reset(&m,&l);m.x=112;maze_model_step(&m,0,0,1);
    assert(fabsf(m.vx+0.240f/1.003f)<0.00001f);
    puts("PASS: magnet center/edge, inner peak, combined force cap and full-input escape");
    d=(maze_dynamic_t){.kind=MAZE_DYNAMIC_LASER,.area={100,70,4,60},.off_ms=1,.warning_ms=1,.on_ms=20};
    l=level(&d);maze_model_reset(&m,&l);m.x=93;m.dynamic_us=2000;
    maze_model_step(&m,0,0,1);assert(m.result==MAZE_MODEL_LOST && m.loss_reason==MAZE_LOSS_LASER);
    maze_model_reset(&m,&l);m.x=102;m.dynamic_us=1999;
    maze_model_step(&m,0,0,1);assert(m.result==MAZE_MODEL_LOST && m.step_elapsed_ms<0.01f);
    d.kind=MAZE_DYNAMIC_GATE;maze_model_reset(&m,&l);m.x=102;m.dynamic_us=2000;
    maze_model_step(&m,0,0,10);assert(m.result==MAZE_MODEL_RUNNING && m.gate_pending==1);
    m.x=120;maze_model_step(&m,0,0,1);assert(!m.gate_pending);
    puts("PASS: laser tangent and step-boundary activation; occupied gate waits until exit");
    d=(maze_dynamic_t){.kind=MAZE_DYNAMIC_PATROL,.area={40,90,20,20},.end={140,90},.speed=35,.dwell_ms=400};
    l=level(&d);maze_model_reset(&m,&l);m.dynamic_us=400000;
    for(int i=0;i<100;i++) maze_model_step(&m,0,0,10);
    fprintf(stderr,"patrol result=%d x=%f t=%llu reason=%d\n",m.result,m.x,(unsigned long long)m.dynamic_us,m.loss_reason);
    assert(m.result==MAZE_MODEL_RUNNING && m.x>100);
    maze_rect_t wall={90,0,8,500};l.walls=&wall;l.wall_count=1;
    maze_model_reset(&m,&l);m.dynamic_us=400000;
    for(int i=0;i<150 && m.result==MAZE_MODEL_RUNNING;i++) maze_model_step(&m,0,0,10);
    assert(m.result==MAZE_MODEL_LOST && m.loss_reason==MAZE_LOSS_CRUSH);
    puts("PASS: stationary ball pushed by relative sweep; wall squeeze fails once");
    d=(maze_dynamic_t){.kind=MAZE_DYNAMIC_LASER,.area={100,70,4,60},.off_ms=1,.warning_ms=1,.on_ms=20};
    l=level(&d);maze_model_reset(&m,&l);m.x=102;
    maze_model_step(&m,0,0,2);assert(m.result==MAZE_MODEL_LOST && m.dynamic_us==2000);
    puts("PASS: exact final-step laser activation is not deferred");
    maze_app_t app;maze_app_init(&app,&l,1000);app.state=MAZE_APP_PAUSED;app.model.dynamic_us=1234;
    maze_app_update(&app,10000);assert(app.model.dynamic_us==1234);
    app.state=MAZE_APP_CALIBRATING;app.started_tick=10000;maze_app_update(&app,10020);assert(app.model.dynamic_us==1234);
    app.state=MAZE_APP_DEVICE_ERROR;maze_app_update(&app,20000);assert(app.model.dynamic_us==1234);
    d.kind=MAZE_DYNAMIC_GATE;l=level(&d);
    maze_mechanism_t cp={MAZE_MECH_CHECKPOINT,{110,90,20,20},0};l.mechanisms=&cp;l.mechanism_count=1;
    maze_model_reset(&m,&l);m.x=105;m.dynamic_us=2000;
    maze_model_step(&m,0,0,1);assert(m.checkpoint==1 && m.checkpoint_dynamic_us==2000);
    uint64_t saved=m.checkpoint_dynamic_us;m.dynamic_us=900000;m.result=MAZE_MODEL_LOST;m.keys=7;
    assert(maze_model_respawn(&m)&&m.dynamic_us==saved&&m.gate_pending==1&&!m.keys);
    maze_model_reset(&m,&l);assert(!m.dynamic_us&&!m.gate_pending&&!m.checkpoint);
    puts("PASS: paused/calibration/error freeze; checkpoint phase and gate snapshot rollback; replay reset");
    d=(maze_dynamic_t){.kind=MAZE_DYNAMIC_LASER,.area={100,70,4,60},.off_ms=1,.warning_ms=1,.on_ms=100};
    l=level(&d);l.ball_radius=5;maze_model_reset(&m,&l);
    m.x=96.96875f;m.y=66.0234375f;m.vx=125;m.vy=-93.75f;m.dynamic_us=2000;
    maze_model_step(&m,5120,-3840,1);assert(m.result==MAZE_MODEL_LOST && m.loss_reason==MAZE_LOSS_LASER);
    puts("PASS: independent review exact moving corner-tangent laser contact");
    maze_model_reset(&m,&l);m.x=94.875f;m.y=100;m.vx=125;m.dynamic_us=101000;
    maze_model_step(&m,5120,0,1);assert(m.result==MAZE_MODEL_RUNNING&&m.dynamic_us==102000);
    puts("PASS: exact laser-off boundary contact remains safe");
    cp=(maze_mechanism_t){MAZE_MECH_CHECKPOINT,{110,90,20,20},0};l.mechanisms=&cp;l.mechanism_count=1;
    maze_model_reset(&m,&l);m.x=104.75f;m.vx=125;
    maze_model_step(&m,5120,0,2);assert(m.result==MAZE_MODEL_LOST&&!m.checkpoint);
    puts("PASS: laser activation at same instant precedes checkpoint");
    maze_dynamic_t pair[2]={{.kind=MAZE_DYNAMIC_PATROL,.area={40,90,20,20},.end={140,90},.speed=35,.dwell_ms=400},
      {.kind=MAZE_DYNAMIC_LASER,.area={90,70,4,60},.off_ms=1,.warning_ms=1,.on_ms=10000}};
    l=level(pair);maze_circle_t trap={90,100,3};l.traps=&trap;l.trap_count=1;
    maze_model_reset(&m,&l);m.dynamic_us=400000;
    for(int i=0;i<200&&m.result==MAZE_MODEL_RUNNING;i++)maze_model_step(&m,0,0,10);
    assert(m.result==MAZE_MODEL_LOST&&m.loss_reason==MAZE_LOSS_TRAP);
    l.trap_count=0;l.dynamic_count=2;maze_model_reset(&m,&l);m.dynamic_us=400000;
    for(int i=0;i<200&&m.result==MAZE_MODEL_RUNNING;i++)maze_model_step(&m,0,0,10);
    assert(m.result==MAZE_MODEL_LOST&&m.loss_reason==MAZE_LOSS_LASER);
    uint64_t stopped=m.dynamic_us;maze_model_step(&m,0,0,100);assert(m.dynamic_us==stopped);
    puts("PASS: actual patrol push sweeps trap/laser; result freezes time");
    maze_app_t a,b;l.trap_count=0;l.dynamic_count=1;maze_app_init(&a,&l,1000);maze_app_init(&b,&l,1000);
    a.state=b.state=MAZE_APP_PLAYING;a.input.x=b.input.x=1500;
    for(unsigned t=25;t<=1000;t+=25){b.last_read_tick=t;maze_app_update(&b,t);if(t%50==0){a.last_read_tick=t;maze_app_update(&a,t);}}
    assert(a.model.dynamic_us==b.model.dynamic_us&&fabsf(a.model.x-b.model.x)<0.001f);
    puts("PASS: 20Hz/40Hz app calling gives identical dynamic clock and motion");
    return 0;
}
