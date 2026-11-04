/* 实际main配受控I/O：三关菜单、迁移保存和随机娱乐不记分。 */
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include "maze_feedback.h"
#define main board_main
#include "../../../src/main.c"
#undef main
static jmp_buf stop;
static maze_navigation_t *shown;
static void(*service)(void*);static void*context;
static uint32_t tick;static unsigned phase,saves,random_saves;
static bool fail_first,in_flush;static unsigned run_mode,loops;
static unsigned light_step,rgb_calls,wdt_feeds;static struct rt_device wdt;static maze_feedback_t visual;
static maze_archive_t random_snapshot;
int rt_kprintf(const char*f,...){(void)f;return 0;}
rt_tick_t rt_tick_get(void){return tick;}
rt_tick_t rt_tick_from_millisecond(int ms){return (uint32_t)ms;}
void rt_thread_mdelay(int ms){tick+=(unsigned)ms;}
#include "drivers/watchdog.h"
void rt_hw_watchdog_hook(int enabled){assert(!enabled);}
void HAL_PMU_Reboot(void){assert(0);}
struct rt_device *rt_device_find(const char*n){static struct rt_device d;return !strcmp(n,"wdt")?&wdt:&d;}
int rt_device_control(struct rt_device*d,int c,void*p){assert(!in_flush);if(d==&wdt){if(c==RT_DEVICE_CTRL_WDT_GET_STATUS)*(uint32_t*)p=1;else if(c==RT_DEVICE_CTRL_WDT_KEEPALIVE)wdt_feeds++;else assert(c==RT_DEVICE_CTRL_WDT_SET_TIMEOUT);return 0;}if(c==PWM_CMD_SET_COLOR&&shown&&shown->lighting){rgb_calls++;if(fail_first&&rgb_calls==1)return -7;}return 0;}
int maze_imu_init(void){return 0;}
void maze_imu_request(uint32_t g,bool enabled){(void)g;(void)enabled;}
bool maze_imu_latest(maze_imu_sample_t*out){(void)out;return false;}
int maze_ui_init(void){return 0;}
void maze_ui_set_service(void(*f)(void*),void*p){service=f;context=p;}
void maze_ui_render(const maze_navigation_t*n){shown=(maze_navigation_t*)n;maze_feedback_observe(&visual,n,tick);}
bool maze_ui_rgb(uint32_t now,uint32_t hz,bool enabled,uint32_t*color){assert(!in_flush);return maze_feedback_rgb(&visual,now,hz,enabled,color);}
void maze_ui_clear(void){}
void maze_ui_loading(void){assert(shown->page==MAZE_NAV_SELECT);}
void maze_ui_process(void){
 if(run_mode==2)tick+=5001;
 if(run_mode==3||run_mode==5){if(!loops){assert(!maze_navigation_select(shown,0));shown->app.state=MAZE_APP_DEVICE_ERROR;shown->app.error=-1002;if(run_mode==5){char *args[]={"maze_watchdog_test","confirm"};assert(!maze_watchdog_test(2,args));}}if(++loops>300){assert(shown->app.state==MAZE_APP_DEVICE_ERROR&&shown->app.error==-1002&&wdt_feeds>10);longjmp(stop,1);}}
 tick+=shown->page==MAZE_NAV_GENERATING?1:20;unsigned cursor=shown->generator.cursor;in_flush=true;service(context);in_flush=false;assert(cursor==shown->generator.cursor);assert(tick<20000);}
int maze_store_open(maze_store_t*s,maze_archive_t*a){memset(s,0,sizeof(*s));maze_archive_default(a);s->revision=17;s->upgrade_pending=true;a->revision=18;return 0;}
int maze_store_save(maze_store_t*s,const maze_archive_t*a){saves++;if(fail_first&&saves==1)return MAZE_STORE_IO;assert(a->revision>s->revision||s->repair);s->revision=a->revision;s->upgrade_pending=false;return 0;}
bool maze_ui_take_command(const maze_navigation_t*n,maze_ui_command_t*out){
 if(run_mode>=2)return false;
 if(n->store_error){assert(fail_first&&saves==1);out->kind=MAZE_UI_SAVE;return true;}
 if(light_step==0){assert(n->lighting);light_step++;return false;}
 if(light_step==1){if(!rgb_calls)return false;light_step++;if(fail_first)assert(!n->lighting&&n->lighting_error==-7);else assert(n->lighting);out->kind=MAZE_UI_DARK;return true;}
 if(light_step==2){light_step++;out->kind=MAZE_UI_TOGGLE;return true;}
 assert(n->dark&&n->reversed);
 switch(phase){
 case 0:assert(saves==(fail_first?2u:1u));out->kind=MAZE_UI_SCORES;break;
 case 1:assert(n->page==MAZE_NAV_SCORES&&n->score_index==0);out->kind=MAZE_UI_SCORE_LEVEL;break;
 case 2:assert(n->score_index==1);out->kind=MAZE_UI_SCORE_LEVEL;break;
 case 3:assert(n->score_index==2);out->kind=MAZE_UI_SCORE_LEVEL;break;
 case 4:assert(n->score_index==0);out->kind=MAZE_UI_BACK;break;
 case 5:assert(n->page==MAZE_NAV_SELECT);out->kind=MAZE_UI_SELECT;out->index=0;break;
 case 6:assert(n->page==MAZE_NAV_GAME&&!strcmp(n->app.level->id,"ice_loops")&&maze_app_mode(&n->app)==3);out->kind=MAZE_UI_BACK;break;
 case 7:if(fail_first)assert(!n->lighting&&n->lighting_error==-7);out->kind=MAZE_UI_RANDOM;break;
 case 8:if(n->page==MAZE_NAV_GENERATING)return false;assert(n->random&&n->app.state==MAZE_APP_READY);random_snapshot=n->progress.archive;random_saves=saves;out->kind=MAZE_UI_GAME;out->game_event=MAZE_EVENT_START;break;
 case 9:assert(n->app.state==MAZE_APP_CALIBRATING);shown->app.state=MAZE_APP_LOST;return phase++,false;
 case 10:assert(saves==random_saves&&!memcmp(&n->progress.archive,&random_snapshot,sizeof(random_snapshot)));out->kind=MAZE_UI_GAME;out->game_event=MAZE_EVENT_REPLAY;break;
 case 11:assert(n->app.state==MAZE_APP_CALIBRATING);shown->app.state=MAZE_APP_WON;return phase++,false;
 default:assert(n->app.state==MAZE_APP_WON&&n->progress.mistakes==0&&n->progress.rank==0&&saves==random_saves&&!memcmp(&n->progress.archive,&random_snapshot,sizeof(random_snapshot)));longjmp(stop,1);
 }
 phase++;return true;
}
int main(void){for(unsigned i=0;i<6;i++){run_mode=i;loops=wdt_feeds=0;watchdog_test_requested=false;tick=phase=saves=light_step=rgb_calls=0;shown=NULL;memset(&visual,0,sizeof(visual));in_flush=false;fail_first=i==1;if(i==4){char *args[]={"maze_watchdog_test","confirm"};assert(maze_watchdog_test(1,args)==-RT_ERROR);assert(!watchdog_test_requested);assert(!maze_watchdog_test(2,args));}
 if(!setjmp(stop)){int result=board_main();assert((i==2||i==4)&&result==-RT_ETIMEOUT&&wdt_feeds==2);}
}puts("PASS: actual main three fixed score cycle/selection, migration retry, random map seed save once; win/loss/replay never saves scores; four-mode bind and optional RGB error visible without game failure; no RGB I/O/generation/watchdog in flush; late UI completion and menu stall stop feeding, recognized sensor error remains live");}
