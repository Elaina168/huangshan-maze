/* 使用SDK真实LVGL离屏刷新统计，不连接显示、触摸或串口。 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "maze_ui.h"
#include "lvgl.h"

static uint16_t buffer[390*40];
static unsigned pixels;
int littlevgl2rtt_init(const char *name) { (void)name; return 0; }
int rt_kprintf(const char *fmt, ...) { (void)fmt; return 0; }
void rt_thread_mdelay(int ms) { lv_tick_inc((uint32_t)ms); }
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data)
{
    (void)data;
    pixels+=(unsigned)(area->x2-area->x1+1)*(unsigned)(area->y2-area->y1+1);
    lv_display_flush_ready(display);
}
static unsigned render(lv_display_t *display, maze_app_t *app, maze_app_state_t state)
{
    app->state=state;
    pixels=0;
    maze_ui_render(app);
    lv_refr_now(display);
    return pixels;
}
int main(void)
{
    setbuf(stdout,NULL);
    lv_init();
    lv_display_t *display=lv_display_create(390,450);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    assert(maze_ui_init()==0);
    maze_app_t app;
    maze_app_init(&app,&maze_level_one,1000);
    render(display,&app,MAZE_APP_READY);
    render(display,&app,MAZE_APP_CALIBRATING);
    unsigned start_pixels=render(display,&app,MAZE_APP_PLAYING);
    printf("calibration->playing flushed pixels=%u (full screen=175500)\n",start_pixels);
    assert(start_pixels<40000);
    render(display,&app,MAZE_APP_PAUSED);
    unsigned resume_pixels=render(display,&app,MAZE_APP_PLAYING);
    printf("paused->playing flushed pixels=%u\n",resume_pixels);
    assert(resume_pixels<40000);
    app.model.x+=1;
    app.elapsed_ms=50;
    unsigned play_pixels=render(display,&app,MAZE_APP_PLAYING);
    printf("playing one-pixel move flushed pixels=%u\n",play_pixels);
    assert(play_pixels<10000);
    /* 复现板端：暂停校准超时后正常采样会清实时错误，但失败提示应保留。 */
    /* 上面刷新统计直接设置状态，反馈测试从新的真实校准会话开始。 */
    maze_app_init(&app,&maze_level_one,1000);
    maze_app_event(&app,MAZE_EVENT_START,1000);
    for (unsigned i=0;i<20;i++)
    {
        maze_imu_sample_t s={.generation=app.generation,.sequence=i+1,
            .read_start_tick=1000+i*50,.raw={0,0,-16000}};
        maze_app_sample(&app,&s,s.read_start_tick);
    }
    assert(app.state==MAZE_APP_PLAYING);
    maze_app_event(&app,MAZE_EVENT_PAUSE,1950);
    render(display,&app,app.state);
    maze_app_event(&app,MAZE_EVENT_RECALIBRATE,1950);
    render(display,&app,app.state);
    maze_app_update(&app,4950);
    assert(app.state==MAZE_APP_PAUSED && app.error==MAZE_ERROR_CALIBRATION_TIMEOUT);
    render(display,&app,app.state);
    /* make_scene先创建标题，再创建状态标签，两者都是屏幕的直接子对象。 */
    lv_obj_t *heading=lv_obj_get_child(lv_screen_active(),1);
    assert(strstr(lv_label_get_text(heading),"-1003"));
    maze_imu_sample_t good={.generation=app.generation,.sequence=1,
        .read_start_tick=5000,.raw={0,0,-16000}};
    maze_app_sample(&app,&good,5000);
    assert(app.error==0 && app.state==MAZE_APP_PAUSED);
    render(display,&app,app.state);
    assert(strstr(lv_label_get_text(heading),"-1003"));
    assert(app.model.x==24 && app.model.y==24 && app.elapsed_ms==0);
    maze_app_event(&app,MAZE_EVENT_RESUME,5000);
    assert(app.state==MAZE_APP_PLAYING);
    render(display,&app,app.state);
    assert(!strstr(lv_label_get_text(heading),"-1003"));
    maze_app_event(&app,MAZE_EVENT_PAUSE,5000);
    maze_app_event(&app,MAZE_EVENT_RECALIBRATE,5000);
    render(display,&app,app.state);
    for (unsigned i=0;i<20;i++)
    {
        maze_imu_sample_t s={.generation=app.generation,.sequence=i+1,
            .read_start_tick=5000+i*50,.raw={0,0,-16000}};
        maze_app_sample(&app,&s,s.read_start_tick);
    }
    assert(app.state==MAZE_APP_PAUSED);
    render(display,&app,app.state);
    assert(!strcmp(lv_label_get_text(heading),"PAUSED"));
    puts("PASS: paused calibration failure remains visible after healthy samples; Resume and successful recalibration clear it");
    puts("PASS: real LVGL partial invalidation; target flush time still needs hardware");
    return 0;
}
