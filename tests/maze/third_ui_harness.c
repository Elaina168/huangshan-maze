/* 正式三关界面使用真实LVGL；位置回放只验证显示，不冒充真机通关。 */
#include "rtthread.h"
#include <math.h>
#define main original_ui_main
#include "ui_refresh_harness.c"
#undef main
static unsigned services,minimum_ball=100000,parts;
static bool trace_ball;
static unsigned ball_pixels(void)
{
    unsigned n=0;uint16_t color=lv_color_to_u16(lv_color_hex(0x7dd3fc));
    for(unsigned y=80;y<380;y++) for(unsigned x=20;x<370;x++) n+=frame[y*390+x]==color;
    return n;
}
static void traced_flush(lv_display_t *d,const lv_area_t *a,uint8_t *data)
{
    flush(d,a,data);
    if(trace_ball) { unsigned n=ball_pixels();if(n<minimum_ball) minimum_ball=n;parts++; }
}
static void service(void *context) { (*(unsigned *)context)++; }
int main(void)
{
    lv_init();lv_display_t *d=lv_display_create(390,450);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(d,traced_flush);assert(maze_ui_init()==0);
    assert(lv_display_get_buf_active(d)->header.h==300);
    assert(!lv_display_is_double_buffered(d));
    maze_ui_set_service(service,&services);
    maze_navigation_init(&navigation,1000);
    navigation.progress.archive.levels[1].unlocked=true;
    navigation.progress.archive.levels[2].unlocked=true;
    maze_ui_render(&navigation);lv_refr_now(d);check_labels(lv_screen_active());
    const char *names[]={"冰岔迷宫","冰弯迷宫","冰环迷宫"};
    for(unsigned i=0;i<3;i++)
    {
        lv_obj_t *c=caption(lv_screen_active(),names[i]);assert(c);
        lv_obj_t *row=lv_obj_get_parent(lv_obj_get_parent(c));
        assert(lv_obj_get_y(row)==112+(int)i*46);
    }
    screenshot("third-selection.ppm");
    click(names[2]);maze_ui_loading();lv_refr_now(d);
    maze_ui_command_t command;assert(!maze_ui_take_command(&navigation,&command));
    assert(caption(lv_screen_active(),"正在载入地图"));
    assert(!caption(lv_screen_active(),"开始"));
    assert(maze_navigation_select(&navigation,2)==0);
    maze_ui_render(&navigation);lv_refr_now(d);check_labels(lv_screen_active());
    lv_obj_t *view=lv_obj_get_child(lv_screen_active(),4),*world=lv_obj_get_child(view,0);
    assert(lv_obj_get_width(world)==700 && lv_obj_get_height(world)==600);
    assert(lv_obj_get_child_count(world)==154);
    navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);
    unsigned full=ball_pixels();assert(full>80);
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    trace_ball=true;
    for(unsigned i=0;i<140;i++)
    {
        navigation.app.model.x=25+fminf(i*5,650);
        navigation.app.model.y=25+fminf(i*5,550);
        lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
    }
    trace_ball=false;lv_mem_monitor(&after);
    assert(after.free_size==before.free_size && minimum_ball>=full && ball_pixels()==full);
    printf("formal third: ball_min=%u full=%u parts=%u heap_before=%zu heap_after=%zu max_used=%zu\n",
           minimum_ball,full,parts,before.free_size,after.free_size,after.max_used);
    assert(lv_obj_get_x(world)==-350 && lv_obj_get_y(world)==-300);
    screenshot("third-edge.ppm");
    click("暂停");navigation.app.state=MAZE_APP_WON;
    assert(!maze_ui_take_command(&navigation,&command));
    maze_ui_render(&navigation);lv_refr_now(d);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"成功到达终点") && caption(lv_screen_active(),"下一关"));
    assert(services>140);
    maze_ui_clear();
    puts("PASS: formal three-level menu/loading, common single buffer/service, large-world clipping, stale result click and final boundary");
}
