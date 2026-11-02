/* 独立试验运行器：只运行app，不打开存档或调用成绩结算。 */
#include "maze_scroll_trial.h"
#include "maze_ui.h"
#include "rtthread.h"

extern const maze_map_definition_t maze_map_scroll_trial;
static uint32_t physics_max_gap,flush_services;

/* 刷新条带之间推进真正的物理时基；不修改场景，不跳过超时检测。 */
static void service_during_flush(void *context)
{
    maze_app_t *app=context;
    /* 主线程优先级高于IMU生产者，显式让出一tick防止连续绘制饿死采样。 */
    rt_thread_mdelay(1);
    uint32_t now=rt_tick_get();
    maze_imu_sample_t sample;
    if(maze_imu_latest(&sample)) maze_app_sample(app,&sample,now);
    if(app->state==MAZE_APP_PLAYING && now-app->last_update_tick>physics_max_gap)
        physics_max_gap=now-app->last_update_tick;
    maze_app_update(app,now);flush_services++;
    uint32_t generation;bool enabled;
    maze_app_request(app,&generation,&enabled);
    maze_imu_request(generation,enabled);
}

int maze_scroll_trial_run(void)
{
    static maze_map_t map;
    static maze_app_t app;
    rt_tick_t begin=rt_tick_get();
    int result=maze_map_load(&maze_map_scroll_trial,&map);
    rt_kprintf("[scroll_trial] map_result=%d ticks=%u workspace=%u nodes=%u no_archive=1\n",
               result,rt_tick_get()-begin,(unsigned)maze_map_workspace_size(),map.checked_nodes);
    if(result) return result;
    maze_app_init(&app,&map.level,RT_TICK_PER_SECOND);
    rt_tick_t last_ui=rt_tick_get(),last_log=last_ui;
    maze_app_state_t previous=app.state;
    uint32_t old_generation=app.generation;
    maze_ui_set_service(service_during_flush,&app);
    maze_ui_render_trial(&app);
    while(1)
    {
        rt_tick_t loop_start=rt_tick_get();
        maze_ui_process();
        rt_tick_t now=rt_tick_get();
        maze_imu_sample_t sample;
        if(maze_imu_latest(&sample)) maze_app_sample(&app,&sample,now);
        maze_event_t event;
        if(maze_ui_take_trial_event(&app,&event))
        {
            rt_kprintf("[scroll_trial] event=%d state=%d gen=%u\n",event,app.state,app.generation);
            if(event==MAZE_EVENT_EXIT) maze_app_cancel(&app);
            else maze_app_event(&app,event,now);
        }
        if(app.state==MAZE_APP_PLAYING && now-app.last_update_tick>physics_max_gap)
            physics_max_gap=now-app.last_update_tick;
        maze_app_update(&app,now);
        uint32_t generation;bool enabled;
        maze_app_request(&app,&generation,&enabled);
        maze_imu_request(generation,enabled);
        bool changed=previous!=app.state || old_generation!=app.generation;
        if(changed || now-last_ui>=rt_tick_from_millisecond(50))
        {
            rt_tick_t render_start=rt_tick_get();
            maze_ui_render_trial(&app);last_ui=now;
            if(changed || now-last_log>=rt_tick_from_millisecond(500))
            {
                rt_kprintf("[scroll_trial] state=%d gen=%u tick=%u ms=%lu pos=%ld,%ld error=%d loop_tick=%u render_tick=%u physics_gap_tick=%u flush_services=%u\n",
                    app.state,app.generation,now,(unsigned long)app.elapsed_ms,(long)app.model.x,(long)app.model.y,
                    app.error,rt_tick_get()-loop_start,rt_tick_get()-render_start,physics_max_gap,flush_services);
                physics_max_gap=flush_services=0;
                last_log=now;
            }
        }
        previous=app.state;old_generation=app.generation;
    }
}
