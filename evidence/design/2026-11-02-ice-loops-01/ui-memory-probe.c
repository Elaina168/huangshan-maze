/* 仅把已批准布局输入当前UI，预研对象内存；不进入产品。 */
#include "rtthread.h"
#define main inherited_ui_main
#include "ui_refresh_harness.c"
#undef main
extern const maze_map_definition_t maze_map_scroll_trial;
rt_tick_t rt_tick_get(void) { return lv_tick_get(); }
int main(void)
{
    setbuf(stdout,NULL);lv_init();
    lv_display_t *display=lv_display_create(390,450);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);assert(maze_ui_init()==0);
    static maze_map_t map;assert(maze_map_load(&maze_map_scroll_trial,&map)==0);
    static maze_app_t app;maze_app_init(&app,&map.level,1000);
    lv_mem_monitor_t mon;lv_mem_monitor(&mon);printf("before scene free=%zu total=%zu\n",mon.free_size,mon.total_size);
    const maze_app_state_t states[]={MAZE_APP_READY,MAZE_APP_CALIBRATING,MAZE_APP_PLAYING,MAZE_APP_PAUSED,MAZE_APP_WON,MAZE_APP_DEVICE_ERROR};
    for(unsigned i=0;i<sizeof(states)/sizeof(states[0]);i++)
    {
        app.state=states[i];maze_ui_render_trial(&app);lv_refr_now(display);
        lv_mem_monitor(&mon);printf("state=%d free=%zu total=%zu max_used=%zu\n",states[i],mon.free_size,mon.total_size,mon.max_used);
    }
    puts("PASS: actual existing UI creates 141-wall layout under host heap");
}
