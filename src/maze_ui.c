/* 显示与触摸预研页，不包含迷宫玩法。 */
#include "maze_ui.h"
#include "rtthread.h"
#include "lvgl.h"
#include "littlevgl2rtt.h"

static lv_obj_t *position_label;
static lv_obj_t *count_label;
static lv_obj_t *sampling_label;
static lv_obj_t *input_label;
static bool sample_toggle;
static unsigned total_clicks;

static struct
{
    const char *name;
    lv_align_t align;
    int32_t x;
    int32_t y;
    uint32_t color;
} const targets[] =
{
    {"TL", LV_ALIGN_TOP_LEFT, 40, 40, 0xa83240},
    {"TR", LV_ALIGN_TOP_RIGHT, -40, 40, 0x237547},
    {"BL", LV_ALIGN_BOTTOM_LEFT, 40, -40, 0x285ba8},
    {"BR", LV_ALIGN_BOTTOM_RIGHT, -40, -40, 0x956018},
    {"CENTER", LV_ALIGN_CENTER, 0, 0, 0x6840a0},
};

static void touch_event(lv_event_t *event)
{
    const char *name = lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED)
    {
        lv_point_t point;
        lv_indev_get_point(lv_event_get_indev(event), &point);
        lv_label_set_text_fmt(position_label, "%s: x=%ld y=%ld", name,
                              (long)point.x, (long)point.y);
        rt_kprintf("[display_probe] press target=%s x=%ld y=%ld\n",
                   name, (long)point.x, (long)point.y);
    }
    else if (code == LV_EVENT_CLICKED)
    {
        total_clicks++;
        if (name == targets[4].name) sample_toggle = !sample_toggle;
        lv_label_set_text_fmt(count_label, "Clicks: %u", total_clicks);
        rt_kprintf("[display_probe] click target=%s count=%u\n", name, total_clicks);
    }
}

int maze_ui_init(void)
{
    int error = littlevgl2rtt_init("lcd");
    if (error != RT_EOK)
        return error;

    lv_display_t *display = lv_display_get_default();
    int32_t width = lv_display_get_horizontal_resolution(display);
    int32_t height = lv_display_get_vertical_resolution(display);
    lv_obj_t *screen = lv_screen_active();
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);

    /* 边框与四角目标内缩，便于观察圆角屏上的可见边界。 */
    lv_obj_t *border = lv_obj_create(screen);
    lv_obj_set_size(border, width - 20, height - 20);
    lv_obj_center(border);
    lv_obj_set_style_bg_opa(border, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(border, lv_color_hex(0x607080), 0);
    lv_obj_set_style_border_width(border, 1, 0);
    lv_obj_set_style_radius(border, 20, 0);
    lv_obj_remove_flag(border, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "IMU INPUT PROBE");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -104);

    lv_obj_t *resolution = lv_label_create(screen);
    lv_label_set_text_fmt(resolution, "%ld x %ld | USB RIGHT", (long)width, (long)height);
    lv_obj_align(resolution, LV_ALIGN_CENTER, 0, -76);

    position_label = lv_label_create(screen);
    lv_label_set_text(position_label, "Touch a target");
    lv_obj_align(position_label, LV_ALIGN_CENTER, 0, 60);
    count_label = lv_label_create(screen);
    lv_label_set_text(count_label, "Clicks: 0");
    lv_obj_align(count_label, LV_ALIGN_CENTER, 0, 88);
    sampling_label = lv_label_create(screen);
    lv_label_set_text(sampling_label, "Center: calibrate");
    lv_obj_align(sampling_label, LV_ALIGN_CENTER, 0, 116);
    input_label = lv_label_create(screen);
    lv_label_set_text(input_label, "x=0 y=0");
    lv_obj_align(input_label, LV_ALIGN_CENTER, 0, -48);

    /* 五个目标共用事件处理，按下记录坐标，完整点击增加一次计数。 */
    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); i++)
    {
        lv_obj_t *button = lv_button_create(screen);
        lv_obj_set_size(button, i == 4 ? 130 : 72, i == 4 ? 58 : 48);
        lv_obj_align(button, targets[i].align, targets[i].x, targets[i].y);
        lv_obj_set_style_bg_color(button, lv_color_hex(targets[i].color), 0);
        lv_obj_add_event_cb(button, touch_event, LV_EVENT_PRESSED, (void *)targets[i].name);
        lv_obj_add_event_cb(button, touch_event, LV_EVENT_CLICKED, (void *)targets[i].name);
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, targets[i].name);
        lv_obj_center(label);
    }

    rt_kprintf("[display_probe] ready width=%ld height=%ld tick_hz=%d\n",
               (long)width, (long)height, RT_TICK_PER_SECOND);
    return RT_EOK;
}

void maze_ui_process(void)
{
    uint32_t delay = lv_timer_handler();
    if (delay < 2) delay = 2;
    if (delay > 20) delay = 20;
    rt_thread_mdelay(delay);
}

bool maze_ui_take_sample_toggle(void)
{
    bool pending = sample_toggle;
    sample_toggle = false;
    return pending;
}

void maze_ui_show_input(const maze_app_t *app)
{
    lv_label_set_text_fmt(input_label,"x=%ld y=%ld",(long)app->input.x,(long)app->input.y);
    switch (app->state)
    {
    case MAZE_APP_READY:
        lv_label_set_text(sampling_label,"Center: calibrate"); break;
    case MAZE_APP_CALIBRATING:
        lv_label_set_text_fmt(sampling_label,"Calibrating: %u/20",app->window_count); break;
    case MAZE_APP_INPUT_READY:
        lv_label_set_text(sampling_label,"Input ready | center: stop"); break;
    case MAZE_APP_CALIBRATION_FAILED:
    case MAZE_APP_INPUT_ERROR:
        lv_label_set_text_fmt(sampling_label,"Error %d | center: retry",app->error); break;
    }
}
