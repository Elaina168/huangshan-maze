/* 使用真实LVGL帧验证可见容量、黑暗遮挡与四玩法设置。 */
#include <math.h>
#define main base_ui_main
#include "ui_refresh_harness.c"
#undef main
static unsigned orb_pixels(void){unsigned count=0;uint16_t cyan=lv_color_to_u16(lv_color_hex(0x7dd3fc));for(unsigned y=80;y<380;y++)for(unsigned x=20;x<370;x++)count+=frame[y*390+x]==cyan;return count;}
int main(void){lv_init();lv_display_t*d=lv_display_create(390,450);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);lv_display_set_buffers(d,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);assert(!maze_ui_init());maze_navigation_init(&navigation,1000);refresh(d);
 assert(caption(lv_screen_active(),"体感迷宫"));
 assert(!caption(lv_screen_active(),"体感迷宫 | 选择关卡"));
 assert(!caption(lv_screen_active(),"USB在右侧 | 轻倾加速"));
 assert(!caption(lv_screen_active(),"选关后点击开始并放稳校准"));
 assert(navigation.lighting);assert(!caption(lv_screen_active(),"灯效：开")&&!caption(lv_screen_active(),"灯效：关"));
 navigation.lighting=false;navigation.lighting_error=-7;navigation.page_version++;refresh(d);
 assert(caption(lv_screen_active(),"灯效：故障 RGB (-7)"));
 maze_ui_command_t cmd;click("视野：明亮");assert(maze_ui_take_command(&navigation,&cmd)&&cmd.kind==MAZE_UI_DARK);maze_navigation_toggle_dark(&navigation);refresh(d);assert(caption(lv_screen_active(),"视野：黑暗"));screenshot("stage7-selection.ppm");
 navigation.progress.archive.levels[0].losses=UINT64_MAX;
 for(unsigned i=0;i<5;i++)maze_score_insert(&navigation.progress.archive.levels[0],(maze_score_t){i==4?UINT64_MAX:1000+i,UINT32_MAX,3-i%4});
 navigation.page=MAZE_NAV_SCORES;navigation.page_version++;refresh(d);assert(caption(lv_screen_active(),"5. 184467440737095516.15 秒\n黑暗+反转 | 失误:4294967295"));assert(!caption(lv_screen_active(),"累计失误:18446744073709551615"));screenshot("stage7-five-scores.ppm");
 assert(!caption(lv_screen_active(),"切换模式"));maze_navigation_back(&navigation);assert(!maze_navigation_select(&navigation,0));refresh(d);assert(caption(lv_screen_active(),"准备就绪"));assert(!caption(lv_screen_active(),"准备就绪 | USB在右侧"));navigation.app.state=MAZE_APP_PLAYING;refresh(d);
 uint16_t dark=lv_color_to_u16(lv_color_hex(0x05090d));assert(frame[379*390+369]==dark&&orb_pixels()==128);screenshot("stage7-dark-start.ppm");
 for(unsigned i=0;i<100;i++){navigation.app.model.x=25+fminf(i*7,650);navigation.app.model.y=25+fminf(i*6,550);lv_tick_inc(50);refresh(d);assert(orb_pixels()==128);}
 screenshot("stage7-dark-edge.ppm");
 for(unsigned i=0;i<3;i++){
  maze_navigation_back(&navigation);navigation.progress.archive.levels[i].unlocked=true;assert(!maze_navigation_select(&navigation,i));refresh(d);assert(caption(lv_screen_active(),"准备就绪"));
  navigation.app.state=MAZE_APP_PLAYING;navigation.app.model.keys=3;navigation.app.model.switches=5;navigation.app.model.checkpoint=1;refresh(d);
  assert(caption(lv_screen_active(),navigation.app.level->mechanism_count?"游玩中 | 检查点已存":"游玩中 | 失误:0"));
  for(unsigned j=0;j<navigation.app.level->surface_count;j++){
   maze_surface_area_t area=navigation.app.level->surfaces[j];navigation.app.model.x=area.area.x+area.area.width/2;navigation.app.model.y=area.area.y+area.area.height/2;refresh(d);
   assert(caption(lv_screen_active(),navigation.app.level->mechanism_count?"游玩中 | 检查点已存":"游玩中 | 失误:0"));
  }
  maze_navigation_back(&navigation);navigation.page=MAZE_NAV_SCORES;navigation.score_index=i;navigation.progress.archive.levels[i].losses=UINT64_MAX;navigation.page_version++;refresh(d);assert(!caption(lv_screen_active(),"累计失误:18446744073709551615"));
 }
 export_font();maze_ui_clear();puts("PASS: dark toggle, exact five two-line rows including maximum numbers fit, no mode sublists; dark scroll keeps complete ball and clips viewport");}
