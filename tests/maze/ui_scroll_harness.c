/* 复用真实LVGL测试设备，独立验证滚动试验界面。 */
#include "rtthread.h"
#include <math.h>
#define main original_ui_main
#define flush base_flush
#include "ui_refresh_harness.c"
#undef flush
#undef main
static uint32_t now;

static bool trace_ball;
static unsigned minimum_ball=100000,trace_parts;
static unsigned ball_pixels(void)
{
    unsigned count=0;uint16_t cyan=lv_color_to_u16(lv_color_hex(0x7dd3fc));
    for(unsigned y=80;y<380;y++) for(unsigned x=20;x<370;x++) count+=frame[y*390+x]==cyan;
    return count;
}
static void traced_flush(lv_display_t *display,const lv_area_t *area,uint8_t *data)
{
    base_flush(display,area,data);
    if(trace_ball)
    {
        unsigned count=ball_pixels();
        if(count<minimum_ball) minimum_ball=count;
        trace_parts++;
    }
}
static unsigned flush_calls;
static void count_service(void *context) { (*(unsigned *)context)++; }
extern const maze_map_definition_t maze_map_scroll_trial;
int main(void)
{
    lv_init();lv_display_t *display=lv_display_create(390,450);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,traced_flush);assert(maze_ui_init()==0);
    assert(lv_display_get_buf_active(display)->header.h==300);
    assert(!lv_display_is_double_buffered(display));
    maze_ui_set_service(count_service,&flush_calls);
    static maze_map_t map;assert(maze_map_load(&maze_map_scroll_trial,&map)==0);
    maze_app_t app;maze_app_init(&app,&map.level,1000);
    maze_ui_render_trial(&app);lv_refr_now(display);check_labels(lv_screen_active());
    lv_obj_t *view=lv_obj_get_child(lv_screen_active(),4);
    lv_obj_t *world=lv_obj_get_child(view,0),*panel=lv_obj_get_child(view,1);
    lv_obj_t *hud=lv_obj_get_child(lv_screen_active(),0),*buttons=lv_obj_get_child(lv_screen_active(),5);
    lv_obj_t *orb=lv_obj_get_child(world,-1);
    assert(lv_obj_get_width(view)==350 && lv_obj_get_height(view)==300);
    screenshot("scroll-ready.ppm");
    unsigned children=lv_obj_get_child_count(world);int panel_x=lv_obj_get_x(panel),panel_y=lv_obj_get_y(panel);
    maze_event_t event;click("开始");assert(maze_ui_take_trial_event(&app,&event) && event==MAZE_EVENT_START);
    app.state=MAZE_APP_CALIBRATING;app.generation++;maze_ui_render_trial(&app);lv_refr_now(display);
    app.state=MAZE_APP_PLAYING;maze_ui_render_trial(&app);lv_refr_now(display);
    unsigned full_ball=ball_pixels();assert(full_ball>80);
    trace_ball=true;
    /* 不滚镜头时跨过一个球直径，旧/新区域必须合并，不能先擦掉再显示。 */
    for(unsigned i=1;i<=10;i++)
    {
        app.model.x=i%2?80:25;now+=50;lv_tick_inc(50);
        maze_ui_render_trial(&app);lv_refr_now(display);
        assert(ball_pixels()==full_ball);
    }
    printf("stationary-camera intermediate ball min=%u full=%u parts=%u\n",minimum_ball,full_ball,trace_parts);
    assert(minimum_ball>=full_ball);
    uint16_t fixed_top[390*30],fixed_bottom[390*65];
    memcpy(fixed_top,frame,sizeof(fixed_top));
    memcpy(fixed_bottom,frame+390*385,sizeof(fixed_bottom));
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    unsigned calls_before=flush_calls;
    unsigned max_pixels=0;
    for(unsigned i=1;i<=180;i++)
    {
        app.model.x=25+fminf(i*6,650);app.model.y=25+fminf(i*5,550);now+=50;lv_tick_inc(50);
        pixels=0;maze_ui_render_trial(&app);lv_refr_now(display);
        if(pixels>max_pixels) max_pixels=pixels;
        assert(pixels<130000);
        assert(ball_pixels()==full_ball);
        assert(!memcmp(fixed_top,frame,sizeof(fixed_top)));
        assert(!memcmp(fixed_bottom,frame+390*385,sizeof(fixed_bottom)));
        assert(lv_obj_get_x(hud)==20 && lv_obj_get_y(hud)==10 && lv_obj_get_y(buttons)==391);
        assert(lv_obj_get_child_count(world)==children);
        assert(lv_obj_get_x(panel)==panel_x && lv_obj_get_y(panel)==panel_y);
        lv_area_t area;lv_obj_get_coords(orb,&area);
        assert(area.x1>=20 && area.x2<=369 && area.y1>=80 && area.y2<=379);
    }
    trace_ball=false;
    printf("scroll intermediate ball min=%u full=%u parts=%u\n",minimum_ball,full_ball,trace_parts);
    assert(minimum_ball>=full_ball);
    assert(flush_calls>calls_before+20);
    assert(lv_obj_get_x(world)==-350 && lv_obj_get_y(world)==-300);
    assert(frame[330*390+70]==lv_color_to_u16(lv_color_hex(0xef4b53)));
    lv_mem_monitor(&after);
    assert(before.free_size==after.free_size);
    printf("scroll LVGL heap free before=%zu after=%zu\n",before.free_size,after.free_size);
    screenshot("scroll-edge.ppm");
    app.state=MAZE_APP_PAUSED;maze_ui_render_trial(&app);lv_refr_now(display);
    int x=lv_obj_get_x(world),y=lv_obj_get_y(world);now+=100000;lv_tick_inc(100000);
    maze_ui_render_trial(&app);assert(lv_obj_get_x(world)==x && lv_obj_get_y(world)==y);
    click("继续");app.generation++;assert(!maze_ui_take_trial_event(&app,&event));
    app.state=MAZE_APP_CALIBRATING;app.calibration_origin=MAZE_CAL_NEW;app.model.x=25;app.model.y=25;
    maze_ui_render_trial(&app);lv_refr_now(display);
    assert(lv_obj_get_x(world)==0 && lv_obj_get_y(world)==0);
    printf("PASS: real LVGL scroll clipping, fixed HUD/panel, stable scene, stale click, pause/reset; max_pixels=%u\n",max_pixels);
    check_labels(lv_screen_active());maze_ui_clear();
    return 0;
}
