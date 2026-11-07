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
static void refresh(lv_display_t*d){maze_ui_render(&navigation);lv_refr_now(d);check_labels(lv_screen_active());}
int main(void){lv_init();lv_display_t*d=lv_display_create(390,450);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);lv_display_set_buffers(d,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);assert(!maze_ui_init());maze_navigation_init(&navigation,1000);
 refresh(d);assert(caption(lv_screen_active(),"冰环迷宫")&&!caption(lv_screen_active(),"冰岔迷宫")&&!caption(lv_screen_active(),"冰弯迷宫"));screenshot("selection.ppm");
 click("成绩");maze_ui_command_t cmd;assert(maze_ui_take_command(&navigation,&cmd)&&cmd.kind==MAZE_UI_SCORES);
 navigation.page=MAZE_NAV_SCORES;navigation.page_version++;refresh(d);assert(caption(lv_screen_active(),"返回选关"));click("返回选关");assert(maze_ui_take_command(&navigation,&cmd)&&cmd.kind==MAZE_UI_BACK);maze_navigation_back(&navigation);
 assert(!maze_navigation_random_open(&navigation,3));refresh(d);assert(caption(lv_screen_active(),"取消生成"));int result;do{result=maze_navigation_random_step(&navigation,0,NULL);}while(result==MAZE_RANDOM_PENDING);assert(result==MAZE_RANDOM_READY);maze_ui_clear();assert(!maze_navigation_random_accept(&navigation,navigation.random_job_id));refresh(d);
 assert(caption(lv_screen_active(),"换一张地图")&&!caption(lv_screen_active(),"本图成绩"));
 assert(caption(lv_screen_active(),"重玩保持同一地图\n黄灯预警，红色激光危险\n绿点存检查点\n随时换一张地图"));char random_title[64];lv_snprintf(random_title,sizeof(random_title),"随机 %08lx | 正常",(unsigned long)navigation.progress.archive.random.seed);assert(caption(lv_screen_active(),random_title));screenshot("casual-ready.ppm");
 const maze_app_state_t states[]={MAZE_APP_CALIBRATING,MAZE_APP_PLAYING,MAZE_APP_PAUSED,MAZE_APP_LOST,MAZE_APP_WON};
 for(unsigned i=0;i<5;i++){navigation.app.state=states[i];refresh(d);assert(caption(lv_screen_active(),"娱乐模式 | 不记录成绩"));assert(!caption(lv_screen_active(),"本图成绩"));}
 screenshot("casual-won.ppm");maze_ui_clear();export_font();printf("PASS: four menu entries, three fixed scores and back, random ready/calibration/play/pause/loss/win has no score entry or timer; glyph/bounds checked pixels=%u\n",pixels);return 0;}
