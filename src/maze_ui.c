/* 游戏页面及触摸事件，只由主线程调用LVGL。 */
#include "maze_ui.h"
#include "rtthread.h"
#include "lvgl.h"
#include "maze_font_zh_16.h"
#include "maze_map.h"
#include "littlevgl2rtt.h"
#include <string.h>

static lv_obj_t *ball, *time_label, *info_label;
static lv_obj_t *heading_label, *info_panel, *controls;
static bool has_page, event_pending;
static maze_app_state_t page_state, event_state;
static uint32_t page_generation, event_generation;
static maze_ui_command_t pending_command, button_commands[8];
static unsigned button_count;
static bool navigation_known;
static maze_navigation_page_t navigation_page;
static uint32_t navigation_version, event_version;
static maze_navigation_page_t event_page;
static int shown_error;
/* 暂停重校准的结果与持续更新的传感器错误分开保存，避免健康采样抹掉提示。 */
static int paused_calibration_error;
static maze_surface_t shown_surface;

static int32_t pixel(float value) { return (int32_t)(value+0.5f); }

static lv_obj_t *box(lv_obj_t *parent, int32_t x, int32_t y,
                     int32_t width, int32_t height, uint32_t color)
{
    lv_obj_t *obj=lv_obj_create(parent);
    /* 去除主题边框、内距与阴影，模型坐标就是父容器局部坐标。 */
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj,x,y);
    lv_obj_set_size(obj,width,height);
    lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);
    lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int32_t x, int32_t y, int32_t width)
{
    lv_obj_t *obj=lv_label_create(parent);
    lv_obj_set_width(obj,width);
    lv_obj_set_pos(obj,x,y);
    lv_obj_set_style_text_align(obj,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_text(obj,text);
    return obj;
}

static lv_obj_t *circle(lv_obj_t *parent, maze_circle_t c, uint32_t color)
{
    lv_obj_t *obj=box(parent,pixel(c.x-c.radius),pixel(c.y-c.radius),
                      pixel(2*c.radius),pixel(2*c.radius),color);
    lv_obj_set_style_radius(obj,pixel(c.radius),0);
    return obj;
}

static void button_clicked(lv_event_t *event)
{
    if (event_pending) return;
    pending_command=*(const maze_ui_command_t *)lv_event_get_user_data(event);
    event_state=page_state;
    event_generation=page_generation;
    event_version=navigation_version;
    event_page=navigation_page;
    event_pending=true;
}

static lv_obj_t *command_button(lv_obj_t *screen, const char *text, maze_ui_command_t command, int32_t x, int32_t width)
{
    lv_obj_t *obj=lv_button_create(screen);
    lv_obj_set_pos(obj,x-20,4);
    lv_obj_set_size(obj,width,38);
    lv_obj_set_style_pad_all(obj,4,0);
    button_commands[button_count]=command;
    lv_obj_add_event_cb(obj,button_clicked,LV_EVENT_CLICKED,&button_commands[button_count++]);
    lv_obj_t *caption=lv_label_create(obj);
    lv_obj_set_style_text_font(caption,&maze_font_zh_16,0);
    lv_label_set_text(caption,text);
    lv_obj_center(caption);
    return obj;
}

static void button(lv_obj_t *screen,const char *text,maze_event_t event,int32_t x,int32_t width)
{
    command_button(screen,text,(maze_ui_command_t){.kind=MAZE_UI_GAME,.game_event=event},x,width);
}

static void back_button(lv_obj_t *screen,int32_t x,int32_t width)
{
    command_button(screen,"选关",(maze_ui_command_t){.kind=MAZE_UI_BACK},x,width);
}

static void make_scene(const maze_navigation_t *nav)
{
    const maze_app_t *app=&nav->app;
    lv_obj_t *screen=lv_screen_active();
    lv_obj_t *title=label(screen,"",20,10,350);
    lv_label_set_text_fmt(title,"%s | %s%s",maze_catalog[nav->index].title,app->reversed?"反转":"正常",
                         nav->store_error?" | 未保存":"");
    heading_label=label(screen,"",20,35,350);
    time_label=label(screen,"用时: 0.00 秒",95,58,200);
    const maze_level_t *level=app->level;
    box(screen,19,79,pixel(level->width)+2,pixel(level->height)+2,0x526b80);
    lv_obj_t *map=box(screen,20,80,pixel(level->width),pixel(level->height),0x182b38);
    for (unsigned i=0;i<level->surface_count;i++)
    {
        maze_surface_area_t s=level->surfaces[i];
        box(map,pixel(s.area.x),pixel(s.area.y),pixel(s.area.width),pixel(s.area.height),
            s.surface==MAZE_SURFACE_ICE?0x294f68:0x182b38);
    }
    for (unsigned i=0;i<level->wall_count;i++)
    {
        maze_rect_t w=level->walls[i];
        box(map,pixel(w.x),pixel(w.y),pixel(w.width),pixel(w.height),0x708596);
    }
    for (unsigned i=0;i<level->trap_count;i++) circle(map,level->traps[i],0xef4b53);
    circle(map,level->goal,0x40c98d);
    ball=circle(map,(maze_circle_t){app->model.x,app->model.y,level->ball_radius},0x7dd3fc);
    info_panel=box(map,20,90,310,120,0x243e50);
    info_label=label(info_panel,"",12,12,286);
    controls=box(screen,20,391,350,45,0x101c27);
}

static void make_page(const maze_navigation_t *nav)
{
    const maze_app_t *app=&nav->app;
    if (!has_page) make_scene(nav);
    /* 保留地图和球，只使提示区域及按钮区域失效，避免切页整屏刷新。 */
    lv_obj_clean(controls);
    button_count=0;
    /* 活动态都不遮盖地图；校准期间就完成大面板的隐藏，继续时无需擦图。 */
    if (app->state==MAZE_APP_CALIBRATING || app->state==MAZE_APP_PLAYING || app->state==MAZE_APP_PAUSED)
        lv_obj_add_flag(info_panel,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(info_panel,LV_OBJ_FLAG_HIDDEN);
    const char *heading="";
    switch (app->state)
    {
    case MAZE_APP_READY:
        heading="准备就绪 | USB在右侧";
        lv_label_set_text(info_label,"轻倾加速，反向制动\n红色陷阱请绕行\n蓝色冰面更滑\n到达绿色终点获胜");
        button(controls,"开始",MAZE_EVENT_START,70,110);
        back_button(controls,210,110); break;
    case MAZE_APP_CALIBRATING:
        heading="校准中";
        button(controls,"取消",MAZE_EVENT_EXIT,130,130); break;
    case MAZE_APP_PLAYING:
        heading="游玩中 | 普通地面";
        button(controls,"暂停",MAZE_EVENT_PAUSE,130,130); break;
    case MAZE_APP_PAUSED:
        heading="已暂停";
        button(controls,"继续",MAZE_EVENT_RESUME,24,80);
        button(controls,"校准",MAZE_EVENT_RECALIBRATE,111,80);
        button(controls,"重玩",MAZE_EVENT_REPLAY,198,80);
        back_button(controls,285,80); break;
    case MAZE_APP_WON:
    case MAZE_APP_LOST:
        heading=app->state==MAZE_APP_WON?
            (nav->index+1==maze_catalog_count?"全部完成":"成功到达终点"):"小球落入陷阱";
        {
            char outcome[80],feedback[80];
            if(app->state==MAZE_APP_LOST) lv_snprintf(outcome,sizeof(outcome),"重玩继续累计用时");
            else if(nav->progress.rank) lv_snprintf(outcome,sizeof(outcome),"本次第%u名",nav->progress.rank);
            else lv_snprintf(outcome,sizeof(outcome),"本次未进前5");
            if(nav->save_pending) lv_snprintf(feedback,sizeof(feedback),"正在保存");
            else if(nav->store_error) lv_snprintf(feedback,sizeof(feedback),"未保存 (%d)\n点击保存重试",nav->store_error);
            else lv_snprintf(feedback,sizeof(feedback),"成绩已保存");
            lv_label_set_text_fmt(info_label,"本次失误:%u\n%s\n%s",nav->progress.mistakes,outcome,feedback);
        }
        if(nav->store_error || nav->store_repair)
        {
            command_button(controls,"保存",(maze_ui_command_t){.kind=MAZE_UI_SAVE},24,80);
            button(controls,"重玩",MAZE_EVENT_REPLAY,111,80);
            if(app->state==MAZE_APP_WON && nav->index+1<maze_catalog_count)
                command_button(controls,"下一关",(maze_ui_command_t){.kind=MAZE_UI_NEXT},198,80);
            back_button(controls,285,80);
        }
        else if(app->state==MAZE_APP_WON && nav->index+1<maze_catalog_count)
        {
            button(controls,"重玩",MAZE_EVENT_REPLAY,24,106);
            command_button(controls,"下一关",(maze_ui_command_t){.kind=MAZE_UI_NEXT},142,106);
            back_button(controls,260,106);
        }
        else
        {
            button(controls,"重玩",MAZE_EVENT_REPLAY,70,110);
            back_button(controls,210,110);
        }
        break;
    case MAZE_APP_CALIBRATION_FAILED:
    case MAZE_APP_DEVICE_ERROR:
        heading=app->state==MAZE_APP_CALIBRATION_FAILED?"校准失败":"设备异常";
        button(controls,"重试",MAZE_EVENT_RETRY,70,110);
        back_button(controls,210,110); break;
    }
    lv_label_set_text(heading_label,heading);
}

int maze_ui_init(void)
{
    int error=littlevgl2rtt_init("lcd");
    if (error!=RT_EOK) return error;
    lv_obj_t *screen=lv_screen_active();
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen,lv_color_hex(0x101c27),0);
    lv_obj_set_style_text_color(screen,lv_color_white(),0);
    lv_obj_set_style_text_font(screen,&maze_font_zh_16,0);
    lv_display_t *display=lv_display_get_default();
    rt_kprintf("[maze_ui] ready width=%ld height=%ld USB_RIGHT tick_hz=%d\n",
        (long)lv_display_get_horizontal_resolution(display),
        (long)lv_display_get_vertical_resolution(display),RT_TICK_PER_SECOND);
    return RT_EOK;
}

void maze_ui_process(void)
{
    uint32_t delay=lv_timer_handler();
    if (delay<2) delay=2;
    if (delay>20) delay=20;
    rt_thread_mdelay(delay);
}

void maze_ui_clear(void)
{
    lv_obj_clean(lv_screen_active());
    ball=time_label=info_label=heading_label=info_panel=controls=NULL;
    has_page=event_pending=navigation_known=false;
    button_count=0;
    shown_error=paused_calibration_error=0;
    shown_surface=MAZE_SURFACE_NORMAL;
    page_generation=0;
    page_state=MAZE_APP_READY;
}

static void map_error(int error)
{
    lv_obj_t *screen=lv_screen_active();
    const char *reason="地图数据错误";
    switch (error)
    {
    case MAZE_MAP_FORMAT: reason="地图格式错误";break;
    case MAZE_MAP_OBJECT: reason="起点终点或陷阱数量错误";break;
    case MAZE_MAP_GEOMETRY: reason="地图对象位置或大小错误";break;
    case MAZE_MAP_NO_ROUTE: reason="未找到安全路线";break;
    }
    label(screen,"地图载入失败",20,140,350);
    label(screen,reason,20,178,350);
    lv_obj_t *code=label(screen,"",20,206,350);
    lv_label_set_text_fmt(code,"错误码: %d",error);
    label(screen,"返回选关后重试",20,238,350);
    controls=box(screen,20,391,350,45,0x101c27);
    back_button(controls,130,130);
}

bool maze_ui_take_command(const maze_navigation_t *nav, maze_ui_command_t *command)
{
    bool valid=event_pending && event_page==nav->page && event_version==nav->page_version;
    if(nav->page==MAZE_NAV_GAME)
        valid=valid && event_state==nav->app.state && event_generation==nav->app.generation;
    event_pending=false;
    if (valid) *command=pending_command;
    return valid;
}

void maze_ui_render(const maze_navigation_t *nav)
{
    bool new_navigation=!navigation_known || navigation_page!=nav->page || navigation_version!=nav->page_version;
    if(new_navigation)
    {
        maze_ui_clear();
        navigation_known=true;
        navigation_page=nav->page;
        navigation_version=nav->page_version;
        if(nav->page==MAZE_NAV_MAP_ERROR) map_error(nav->error);
        else if(nav->page==MAZE_NAV_STORAGE_ERROR)
        {
            lv_obj_t *screen=lv_screen_active();
            label(screen,"存档不可用",20,140,350);
            label(screen,"未覆盖原档案",20,178,350);
            lv_obj_t *code=label(screen,"",20,206,350);
            lv_label_set_text_fmt(code,"错误码: %d",nav->store_error);
            label(screen,"请重试；未知数据会保留",20,238,350);
            controls=box(screen,20,391,350,45,0x101c27);
            command_button(controls,"重试",(maze_ui_command_t){.kind=MAZE_UI_SAVE},130,130);
        }
        else if(nav->page==MAZE_NAV_SCORES)
        {
            lv_obj_t *screen=lv_screen_active();
            const maze_level_record_t *level=&nav->progress.archive.levels[nav->score_index];
            const maze_mode_record_t *mode=&level->modes[nav->score_mode];
            lv_obj_t *title=label(screen,"",20,30,350);
            lv_label_set_text_fmt(title,"成绩 | %s",maze_catalog[nav->score_index].title);
            label(screen,nav->score_mode?"反转模式":"正常模式",20,64,350);
            lv_obj_t *totals=label(screen,"",20,96,350);
            lv_label_set_text_fmt(totals,"%s | 累计失误:%lu",
                level->completed&(1u<<nav->score_mode)?"已通关":"未通关",(unsigned long)mode->losses);
            if(!mode->count) label(screen,"暂无通关记录",20,150,350);
            for(unsigned i=0;i<mode->count;i++)
            {
                lv_obj_t *row=label(screen,"",20,140+(int)i*34,350);
                maze_score_t score=mode->scores[i];
                lv_label_set_text_fmt(row,"%u. %llu.%02llu 秒  失误:%lu",i+1,
                    (unsigned long long)(score.time_cs/100),(unsigned long long)(score.time_cs%100),
                    (unsigned long)score.mistakes);
            }
            lv_obj_t *row=box(screen,20,320,350,45,0x101c27);
            command_button(row,"切换关卡",(maze_ui_command_t){.kind=MAZE_UI_SCORE_LEVEL},45,145);
            command_button(row,"切换模式",(maze_ui_command_t){.kind=MAZE_UI_SCORE_MODE},200,145);
            controls=box(screen,20,391,350,45,0x101c27);back_button(controls,130,130);
        }
        else if(nav->page==MAZE_NAV_SELECT)
        {
            lv_obj_t *screen=lv_screen_active();
            label(screen,"体感迷宫 | 选择关卡",20,32,350);
            label(screen,"USB在右侧 | 轻倾加速",20,66,350);
            for(unsigned i=0;i<maze_catalog_count;i++)
            {
                lv_obj_t *row=box(screen,20,120+(int)i*70,350,50,0x101c27);
                bool unlocked=maze_progress_unlocked(&nav->progress,maze_catalog[i].definition->id);
                char title[80];lv_snprintf(title,sizeof(title),unlocked?"%s":"%s [未解锁]",maze_catalog[i].title);
                lv_obj_t *choice=command_button(row,title,
                    (maze_ui_command_t){.kind=MAZE_UI_SELECT,.index=i},45,300);
                if(!unlocked) lv_obj_add_state(choice,LV_STATE_DISABLED);
            }
            lv_obj_t *scores=box(screen,20,260,350,50,0x101c27);
            command_button(scores,"成绩",(maze_ui_command_t){.kind=MAZE_UI_SCORES},45,300);
            lv_obj_t *row=box(screen,20,320,350,50,0x101c27);
            command_button(row,nav->reversed?"控制方向：反转":"控制方向：正常",
                (maze_ui_command_t){.kind=MAZE_UI_TOGGLE},45,300);
            if(nav->store_error || nav->store_repair || nav->save_pending)
            {
                lv_obj_t *status=label(screen,"",20,94,350);
                if(nav->save_pending) lv_label_set_text(status,"正在保存");
                else if(nav->store_error) lv_label_set_text_fmt(status,"未保存 (%d)",nav->store_error);
                else lv_label_set_text(status,"副本待修复");
                controls=box(screen,20,391,350,45,0x101c27);
                command_button(controls,"保存",(maze_ui_command_t){.kind=MAZE_UI_SAVE},130,130);
            }
            else label(screen,"选关后点击开始并放稳校准",20,397,350);
        }
    }
    if(nav->page!=MAZE_NAV_GAME) return;
    const maze_app_t *app=&nav->app;
    bool changed=!has_page || page_state!=app->state;
    if (changed || page_generation!=app->generation) event_pending=false;
    if (changed)
    {
        paused_calibration_error=app->state==MAZE_APP_PAUSED && page_state==MAZE_APP_CALIBRATING?
            app->error:0;
        make_page(nav);
    }
    has_page=true;
    page_state=app->state;
    page_generation=app->generation;
    lv_obj_set_pos(ball,pixel(app->model.x-app->level->ball_radius),
                        pixel(app->model.y-app->level->ball_radius));
    uint64_t cs=maze_progress_time_cs(&nav->progress,app);
    lv_label_set_text_fmt(time_label,"挑战: %llu.%02llu 秒",(unsigned long long)(cs/100),(unsigned long long)(cs%100));
    if (app->state==MAZE_APP_CALIBRATING)
        lv_label_set_text_fmt(heading_label,"请放稳: %u/%u | 失误:%u",
                              app->window_count,MAZE_CALIBRATION_COUNT,nav->progress.mistakes);
    if (app->state==MAZE_APP_PLAYING)
    {
        maze_surface_t surface=maze_level_surface_at(app->level,app->model.x,app->model.y);
        if (changed || shown_surface!=surface)
            lv_label_set_text_fmt(heading_label,"游玩中 | %s | 失误:%u",
                surface==MAZE_SURFACE_ICE?"冰面":"普通地面",nav->progress.mistakes);
        shown_surface=surface;
    }
    int feedback_error=app->error;
    if (app->state==MAZE_APP_PAUSED && !feedback_error) feedback_error=paused_calibration_error;
    if (changed || shown_error!=feedback_error)
    {
        if (app->state==MAZE_APP_PAUSED && feedback_error)
            lv_label_set_text_fmt(heading_label,"已暂停 | 失误:%u (%d)",nav->progress.mistakes,feedback_error);
        else if (app->state==MAZE_APP_CALIBRATION_FAILED || app->state==MAZE_APP_DEVICE_ERROR)
            lv_label_set_text_fmt(info_label,app->state==MAZE_APP_CALIBRATION_FAILED?
                "校准未完成 (%d)\n请放稳后重试。":
                "传感器或时序异常 (%d)\n请放稳后重试。\n恢复后返回暂停。",app->error);
        else if (app->state==MAZE_APP_PAUSED)
            lv_label_set_text_fmt(heading_label,"已暂停 | 失误:%u",nav->progress.mistakes);
    }
    shown_error=feedback_error;
}
