/* 使用SDK真实LVGL离屏刷新统计，不连接显示、触摸或串口。 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "maze_ui.h"
#include "rtthread.h"
#include "maze_map.h"
#include "maze_navigation.h"
#include "lvgl.h"
#include "maze_font_zh_16.h"
#include "src/misc/lv_text_private.h"

static uint16_t buffer[390*40];
static unsigned pixels;
static uint16_t frame[390*450];
/* 按实际生成的字形经LVGL读取后导出，供独立像素检查。 */
static void export_font(void)
{
    FILE *out=fopen("font-raw.txt","w");assert(out);
    for (uint32_t cp=128;cp<=65535;cp++)
    {
        lv_font_glyph_dsc_t g={0};
        if (!lv_font_get_glyph_dsc(&maze_font_zh_16,&g,cp,0) || g.is_placeholder ||
            g.resolved_font!=&maze_font_zh_16) continue;
        lv_draw_buf_t *decoded=lv_draw_buf_create(g.box_w,g.box_h,LV_COLOR_FORMAT_A8,LV_STRIDE_AUTO);
        assert(decoded && lv_font_get_glyph_bitmap(&g,decoded)==decoded);
        fprintf(out,"%u %u %u %d %d %u ",cp,g.box_w,g.box_h,g.ofs_x,g.ofs_y,g.adv_w);
        for (unsigned y=0;y<g.box_h;y++)
            for (unsigned x=0;x<g.box_w;x++) fprintf(out,"%02x",decoded->data[y*decoded->header.stride+x]);
        lv_draw_buf_destroy(decoded);
        fputc('\n',out);
    }
    assert(fclose(out)==0);
}
static void screenshot(const char *name)
{
    FILE *out=fopen(name,"wb");assert(out);fprintf(out,"P6\n390 450\n255\n");
    for (unsigned i=0;i<390*450;i++)
    {
        unsigned v=frame[i];
        uint8_t rgb[]={(uint8_t)(((v>>11)&31)*255/31),
                       (uint8_t)(((v>>5)&63)*255/63),(uint8_t)((v&31)*255/31)};
        assert(fwrite(rgb,1,3,out)==3);
    }
    assert(fclose(out)==0);
}
static void check_labels(lv_obj_t *obj)
{
    if (lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN)) return;
    if (lv_obj_check_type(obj,&lv_label_class))
    {
        const lv_font_t *font=lv_obj_get_style_text_font(obj,LV_PART_MAIN);
        assert(font==&maze_font_zh_16);
        const char *text=lv_label_get_text(obj);
        uint32_t index=0,cp;
        while ((cp=lv_text_encoded_next(text,&index)))
        {
            if (cp=='\n') continue;
            lv_font_glyph_dsc_t g={0};
            assert(lv_font_get_glyph_dsc(font,&g,cp,0) && !g.is_placeholder);
            assert(g.resolved_font==(cp>127?&maze_font_zh_16:&lv_font_montserrat_16));
            assert(font->line_height-font->base_line-g.box_h-g.ofs_y>=0);
            assert(font->line_height-font->base_line-g.ofs_y<=font->line_height);
        }
        lv_point_t size;
        lv_text_get_size(&size,text,font,0,lv_obj_get_style_text_line_space(obj,0),32767,LV_TEXT_FLAG_NONE);
        assert(size.x<=lv_obj_get_width(obj) && size.y<=lv_obj_get_height(obj));
        lv_area_t a,b;lv_obj_get_coords(obj,&a);lv_obj_get_coords(lv_obj_get_parent(obj),&b);
        assert(a.x1>=b.x1 && a.x2<=b.x2 && a.y1>=b.y1 && a.y2<=b.y2);
    }
    for (unsigned i=0;i<lv_obj_get_child_count(obj);i++) check_labels(lv_obj_get_child(obj,i));
}
int littlevgl2rtt_init(const char *name) { (void)name; return 0; }
int rt_kprintf(const char *fmt, ...) { (void)fmt; return 0; }
rt_tick_t rt_tick_get(void) { return lv_tick_get(); }
void rt_thread_mdelay(int ms) { lv_tick_inc((uint32_t)ms); }
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data)
{
    const uint16_t *src=(const uint16_t *)data;
    unsigned width=(unsigned)(area->x2-area->x1+1);
    for (int y=area->y1;y<=area->y2;y++)
        memcpy(&frame[y*390+area->x1],&src[(y-area->y1)*width],width*2);
    pixels+=(unsigned)(area->x2-area->x1+1)*(unsigned)(area->y2-area->y1+1);
    lv_display_flush_ready(display);
}
static maze_navigation_t navigation;
static lv_obj_t *caption(lv_obj_t *root,const char *text)
{
    if(lv_obj_check_type(root,&lv_label_class) && !strcmp(lv_label_get_text(root),text)) return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);i++)
    {
        lv_obj_t *found=caption(lv_obj_get_child(root,i),text);
        if(found) return found;
    }
    return NULL;
}
static void click(const char *text)
{
    lv_obj_t *label=caption(lv_screen_active(),text); assert(label);
    lv_obj_t *button=lv_obj_get_parent(label);
    assert(lv_obj_check_type(button,&lv_button_class));
    lv_obj_send_event(button,LV_EVENT_CLICKED,NULL);
}
static void navigation_ui(lv_display_t *display)
{
    maze_ui_command_t command;
    maze_ui_clear(); maze_navigation_init(&navigation,1000);
    navigation.progress.archive.levels[1].unlocked=true;
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"冰岔迷宫") && caption(lv_screen_active(),"冰弯迷宫"));
    screenshot("selection.ppm");
    click("冰岔迷宫");
    assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_SELECT && command.index==0);
    click("冰岔迷宫");
    maze_navigation_toggle_direction(&navigation);
    assert(!maze_ui_take_command(&navigation,&command));
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"控制方向：反转"));
    maze_ui_clear(); assert(maze_navigation_select(&navigation,0)==0);
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"冰岔迷宫 | 反转"));
    click("开始");
    navigation.app.generation++;
    assert(!maze_ui_take_command(&navigation,&command));
    navigation.app.state=MAZE_APP_WON;
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    click("下一关");
    assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_NEXT);
    maze_ui_clear(); assert(maze_navigation_next(&navigation)==0);
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    assert(navigation.app.state==MAZE_APP_READY && navigation.app.reversed);
    assert(caption(lv_screen_active(),"冰弯迷宫 | 反转"));
    screenshot("second-ready.ppm");
    navigation.app.state=MAZE_APP_WON;
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"下一关"));
    navigation.progress.archive.levels[2].unlocked=true;
    maze_ui_clear();assert(maze_navigation_next(&navigation)==0);
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    navigation.app.state=MAZE_APP_WON;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"下一关"));
    navigation.progress.archive.levels[3].unlocked=true;
    maze_ui_clear();assert(maze_navigation_next(&navigation)==0);
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    navigation.app.state=MAZE_APP_WON;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"全部完成") && !caption(lv_screen_active(),"下一关"));
    screenshot("all-complete.ppm");
    navigation.app.state=MAZE_APP_CALIBRATING;
    maze_ui_render(&navigation);
    navigation.app.state=MAZE_APP_PAUSED; navigation.app.error=-1003;
    maze_ui_render(&navigation);
    maze_ui_clear(); maze_navigation_back(&navigation);
    assert(maze_navigation_select(&navigation,0)==0);
    navigation.app.state=MAZE_APP_PAUSED;
    maze_ui_render(&navigation); lv_refr_now(display);
    assert(caption(lv_screen_active(),"已暂停 | 失误:0"));
    navigation.page=MAZE_NAV_MAP_ERROR; navigation.page_version++; navigation.error=MAZE_MAP_NO_ROUTE;
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    click("选关"); assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_BACK);
    maze_ui_clear(); maze_navigation_back(&navigation);
    maze_ui_render(&navigation); lv_refr_now(display); check_labels(lv_screen_active());
    click("冰岔迷宫"); maze_navigation_toggle_direction(&navigation); maze_navigation_toggle_direction(&navigation);
    assert(!maze_ui_take_command(&navigation,&command));
    puts("PASS: actual navigation UI, two titles, mode, next/final, stale generation/page clicks and cleared pause-error cache");
}
static void progress_ui(lv_display_t *display)
{
    maze_ui_command_t command;
    maze_ui_clear();maze_navigation_init(&navigation,1000);
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"冰弯迷宫 [未解锁]"));
    screenshot("locked-selection.ppm");
    assert(maze_navigation_select(&navigation,1)==MAZE_NAV_LOCKED);
    click("成绩");assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_SCORES);
    navigation.page=MAZE_NAV_SCORES;navigation.page_version++;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"暂无通关记录"));
    screenshot("scores-empty.ppm");
    navigation.progress.archive.levels[0].modes[0].count=5;
    navigation.progress.archive.levels[0].completed=1;
    navigation.progress.archive.levels[0].modes[0].losses=20;
    for(unsigned i=0;i<5;i++) navigation.progress.archive.levels[0].modes[0].scores[i]=(maze_score_t){123456+i,2+i};
    navigation.page_version++;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());screenshot("scores-full.ppm");
    navigation.page=MAZE_NAV_SELECT;navigation.page_version++;navigation.store_error=-3102;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    click("保存");assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_SAVE);
    navigation.page=MAZE_NAV_STORAGE_ERROR;navigation.page_version++;
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
    assert(caption(lv_screen_active(),"存档不可用"));screenshot("storage-error.ppm");
    puts("PASS: locked level, score list empty/top5, unsaved retry and startup storage error UI");
}
static unsigned render(lv_display_t *display, maze_app_t *app, maze_app_state_t state)
{
    app->state=state;
    pixels=0;
    navigation.page=MAZE_NAV_GAME;
    navigation.app=*app;
    navigation.progress.active=true;
    navigation.progress.accounted=false;
    navigation.progress.accrued_ms=0;
    maze_ui_render(&navigation);
    lv_refr_now(display);
    check_labels(lv_screen_active());
    return pixels;
}
int main(int argc, char **argv)
{
    setbuf(stdout,NULL);
    lv_init();
    lv_display_t *display=lv_display_create(390,450);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    assert(maze_ui_init()==0);
    maze_navigation_init(&navigation,1000);
    if (argc>1)
    {
        assert(!strcmp(argv[1],"--map-error"));
        const int codes[]={MAZE_MAP_FORMAT,MAZE_MAP_OBJECT,MAZE_MAP_GEOMETRY,MAZE_MAP_NO_ROUTE};
        for (unsigned i=0;i<4;i++)
        {
            navigation.page=MAZE_NAV_MAP_ERROR;
            navigation.page_version++;
            navigation.error=codes[i];
            maze_ui_render(&navigation);
            lv_refr_now(display);check_labels(lv_screen_active());
            assert(lv_obj_get_child_count(lv_screen_active())==5);
            assert(!strcmp(lv_label_get_text(lv_obj_get_child(lv_screen_active(),0)),"地图载入失败"));
            char code[20];snprintf(code,sizeof(code),"%d",codes[i]);
            assert(strstr(lv_label_get_text(lv_obj_get_child(lv_screen_active(),2)),code));
            maze_ui_process();
        }
        screenshot("map-error.ppm");export_font();
        puts("PASS: all map-load error pages show reason/code and return button, fit and service UI");
        return 0;
    }
    static maze_map_t map_data;
    assert(maze_map_load(&maze_map_frost_fork,&map_data)==MAZE_MAP_OK);
    /* 独立几何视图有相同ID但不同地址，验证UI不再靠实例指针识别。 */
    maze_level_t alias=map_data.level;
    maze_app_t app;
    maze_app_init(&app,&alias,1000);
    render(display,&app,MAZE_APP_READY);
    assert(!strcmp(lv_label_get_text(lv_obj_get_child(lv_screen_active(),0)),"冰岔迷宫 | 正常"));
    screenshot("ready.ppm");
    lv_obj_t *view=lv_obj_get_child(lv_screen_active(),4);
    lv_obj_t *map=lv_obj_get_child(view,0);
    assert(lv_obj_get_child_count(map)==36);
    for (unsigned i=0;i<app.level->surface_count+app.level->wall_count;i++)
    {
        maze_rect_t r=i<app.level->surface_count?app.level->surfaces[i].area:
            app.level->walls[i-app.level->surface_count];
        lv_obj_t *obj=lv_obj_get_child(map,i);
        assert(lv_obj_get_x(obj)==(int)r.x && lv_obj_get_y(obj)==(int)r.y);
        assert(lv_obj_get_width(obj)==(int)r.width && lv_obj_get_height(obj)==(int)r.height);
    }
    puts("PASS: actual LVGL maze title, object count, walls and ice geometry");
    render(display,&app,MAZE_APP_CALIBRATING);
    unsigned start_pixels=render(display,&app,MAZE_APP_PLAYING);
    printf("calibration->playing flushed pixels=%u (full screen=175500)\n",start_pixels);
    assert(start_pixels<40000);
    screenshot("playing.ppm");
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
    maze_app_init(&app,&map_data.level,1000);
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
    screenshot("pause-error.ppm");
    maze_imu_sample_t good={.generation=app.generation,.sequence=1,
        .read_start_tick=5000,.raw={0,0,-16000}};
    maze_app_sample(&app,&good,5000);
    assert(app.error==0 && app.state==MAZE_APP_PAUSED);
    render(display,&app,app.state);
    assert(strstr(lv_label_get_text(heading),"-1003"));
    assert(app.model.x==25 && app.model.y==25 && app.elapsed_ms==0);
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
    assert(strstr(lv_label_get_text(heading),"已暂停") && strstr(lv_label_get_text(heading),"失误"));
    puts("PASS: paused calibration failure remains visible after healthy samples; Resume and successful recalibration clear it");
    render(display,&app,MAZE_APP_PLAYING);
    assert(strstr(lv_label_get_text(heading),"普通地面"));
    app.model.x=125;app.model.y=225;
    render(display,&app,MAZE_APP_PLAYING);
    assert(strstr(lv_label_get_text(heading),"冰面"));
    app.model.x=25;app.model.y=25;
    render(display,&app,MAZE_APP_PLAYING);
    assert(strstr(lv_label_get_text(heading),"普通地面"));
    puts("PASS: maze NORMAL/ICE heading follows the shared ball-center query");
    for (int state=MAZE_APP_READY;state<=MAZE_APP_LOST;state++)
    {
        app.error=(state==MAZE_APP_CALIBRATION_FAILED)?-1003:
                  (state==MAZE_APP_DEVICE_ERROR)?-1004:0;
        render(display,&app,(maze_app_state_t)state);
        if (state==MAZE_APP_DEVICE_ERROR) screenshot("device-error.ppm");
        if (state==MAZE_APP_WON) screenshot("won.ppm");
    }
    app.window_count=20;render(display,&app,MAZE_APP_CALIBRATING);
    assert(strstr(lv_label_get_text(heading),"请放稳: 20/20"));
    app.elapsed_ms=1234567;render(display,&app,MAZE_APP_PLAYING);
    assert(!strcmp(lv_label_get_text(lv_obj_get_child(lv_screen_active(),2)),"挑战: 1234.56 秒"));
    puts("PASS: all states Chinese glyph coverage, ASCII fallback, font baseline, label/button bounds, progress and timer");
    navigation_ui(display);
    progress_ui(display);
    export_font();
    puts("PASS: real LVGL partial invalidation; target flush time still needs hardware");
    return 0;
}
