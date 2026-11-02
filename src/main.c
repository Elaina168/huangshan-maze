/*
 * course_starter 原始入口：串口打印 + 黄山派板载 RGB 灯颜色闪烁。
 *
 * 基于 SiFli-SDK 官方例程（Apache-2.0）：
 *   example/get-started/hello_world/rtt   —— 启动打印
 *   example/rt_device/rgb_led             —— RGB 灯驱动用法（SK6812MINI-HS，PA32，PWMT2 通道1）
 * 板型：sf32lb52-lchspi-ulp（立创·黄山派）。
 *
 * 原RGB个人修改及实测证据保留在Git与evidence；当前运行体感迷宫，启动后关灯。
 *
 * 新增 .c 文件直接放进 src/ 即可，src/SConscript 会自动收集。
 */
#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "drivers/rt_drv_pwm.h"
#include "maze_ui.h"
#include "maze_map.h"
#include "maze_imu.h"
#ifdef MAZE_SCROLL_TRIAL
#include "maze_scroll_trial.h"
#else
#include "maze_store.h"
#endif
#include "finsh.h"

#define APP_NAME        "huangshan_maze"
#ifdef MAZE_SCROLL_TRIAL
#define APP_VERSION     "0.11.0-scroll-trial"
#else
#define APP_VERSION     "0.11.0-basic-mechanisms"
#endif

#define RGBLED_NAME     "rgbled"

static struct rt_device *rgbled_device;

static void rgb_led_init(void)
{
#ifdef SF32LB52X
    HAL_PIN_Set(PAD_PA32, GPTIM2_CH1, PIN_NOPULL, 1);   /* pwmt2_cc1 */
    /* RGB 灯供电 */
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO3_3V3, true, true);
#endif
    rgbled_device = rt_device_find(RGBLED_NAME);
    if (!rgbled_device)
    {
        rt_kprintf("[%s] ERROR: device '%s' not found, check proj.conf RGB options\n", APP_NAME, RGBLED_NAME);
        RT_ASSERT(0);
    }
}

static int maze_rgb_off(void)
{
#ifdef SF32LB52X
    pin_function function;
    PIN_ModeTypeDef mode;
    int pin_result = HAL_PIN_Get(PAD_PA32, &function, &mode, 1);
    if (pin_result < 0)
    {
        rt_kprintf("[maze_rgb] pin read failed result=%d\n",pin_result);
        return -RT_ERROR;
    }
    rt_kprintf("[maze_rgb] pin_func=%d expected=%d mode=%d\n",function,GPTIM2_CH1,mode);
    if (function != GPTIM2_CH1)
    {
        HAL_PIN_Set(PAD_PA32, GPTIM2_CH1, PIN_NOPULL, 1);
        rt_kprintf("[maze_rgb] restored PA32 PWM mapping\n");
    }
#endif
    struct rt_rgbled_configuration configuration = {0};
    int error = rt_device_control(rgbled_device, PWM_CMD_SET_COLOR, &configuration);
    rt_kprintf("[maze_rgb] black request result=%d tick=%u\n",error,rt_tick_get());
    if (error != RT_EOK) return error;
    struct rt_pwm_configuration pwm = {.channel=BSP_USING_RGBLED_CH};
    struct rt_device *pwm_device = rt_device_find(RGB_USING_SK6812MINI_HS_PWM_DEV_NAME);
    error = rt_device_control(pwm_device, PWM_CMD_GET, &pwm);
    rt_kprintf("[maze_rgb] pwm_read=%d period_ns=%u pulse_ns=%u\n",error,pwm.period,pwm.pulse);
    return error;
}
MSH_CMD_EXPORT(maze_rgb_off, Send a black frame and report the RGB driver result);

#ifndef MAZE_SCROLL_TRIAL
static uint32_t physics_max_gap,flush_services;
/* 显示提交间推进纯逻辑，不访问LVGL、触摸或持久化接口。 */
static void service_during_flush(void *context)
{
    maze_navigation_t *nav=context;
    rt_thread_mdelay(1);
    uint32_t now=rt_tick_get();
    maze_imu_sample_t sample;
    if(maze_imu_latest(&sample)) maze_navigation_sample(nav,&sample,now);
    if(nav->page==MAZE_NAV_GAME && nav->app.state==MAZE_APP_PLAYING &&
       now-nav->app.last_update_tick>physics_max_gap)
        physics_max_gap=now-nav->app.last_update_tick;
    maze_navigation_update(nav,now);flush_services++;
    uint32_t generation;bool enabled;
    maze_navigation_request(nav,&generation,&enabled);
    maze_imu_request(generation,enabled);
}
#endif

int main(void)
{
    rt_kprintf("\n[%s] v%s boot ok\n", APP_NAME, APP_VERSION);
    rgb_led_init();
    int error=maze_rgb_off();
    if(error==RT_EOK) error=maze_ui_init();
    if(error==RT_EOK) error=maze_imu_init();
    if(error!=RT_EOK)
    {
        rt_kprintf("[%s] init failed error=%d\n",APP_NAME,error);
        return error;
    }
#ifdef MAZE_SCROLL_TRIAL
    return maze_scroll_trial_run();
#else
    static maze_navigation_t nav;
    maze_navigation_init(&nav,RT_TICK_PER_SECOND);
    static maze_store_t store;
    maze_archive_t loaded;
    rt_tick_t open_start=rt_tick_get();
    error=maze_store_open(&store,&loaded);
    bool store_ready=error==0;
    if(store_ready) maze_progress_init(&nav.progress,&loaded);
    else { nav.page=MAZE_NAV_STORAGE_ERROR;nav.store_error=error; }
    nav.store_repair=store.repair;
    rt_kprintf("[maze_store] open result=%d ticks=%u active=%d repair=%d\n",
               error,rt_tick_get()-open_start,store.active,store.repair);
    uint64_t attempted_revision=store_ready?store.revision:0;
    maze_ui_set_service(service_during_flush,&nav);
    maze_app_t *app=&nav.app;
    maze_app_state_t last_state=MAZE_APP_READY;
    uint32_t last_generation=0,last_page_version=0;
    rt_tick_t last_log=rt_tick_get(),last_ui=last_log,previous_loop=last_log,last_render_cost=0;
    maze_ui_render(&nav);
    while(1)
    {
        rt_tick_t service_start=rt_tick_get();
        maze_ui_process();
        rt_tick_t now=rt_tick_get(),service_cost=now-service_start;
        rt_tick_t loop_gap=now-previous_loop;
        previous_loop=now;
        uint32_t previous_sequence=app->last_sequence,previous_read=app->last_read_tick;
        if(nav.page==MAZE_NAV_GAME) maze_progress_observe(&nav.progress,app);
        bool force_save=false;
        maze_ui_command_t command;
        /* 旧档升级首次写入先于菜单命令；失败只抑制自动重试，不阻止手动保存或游玩。 */
        bool first_upgrade=store_ready && store.upgrade_pending && attempted_revision!=nav.progress.archive.revision;
        if(!first_upgrade && maze_ui_take_command(&nav,&command))
        {
            rt_kprintf("[maze_nav] command=%d index=%u page=%d version=%u tick=%u\n",
                       command.kind,command.index,nav.page,nav.page_version,now);
            if(command.kind==MAZE_UI_GAME)
            {
                rt_kprintf("[maze] event=%d state=%d gen=%u tick=%u\n",
                           command.game_event,app->state,app->generation,now);
                maze_progress_before_event(&nav.progress,app,command.game_event);
                maze_app_event(app,command.game_event,now);
            }
            else if(command.kind==MAZE_UI_TOGGLE) maze_navigation_toggle_direction(&nav);
            else if(command.kind==MAZE_UI_SAVE) force_save=true;
            else if(command.kind==MAZE_UI_SCORES)
            {
                nav.page=MAZE_NAV_SCORES;nav.score_index=0;nav.score_mode=nav.reversed?1:0;nav.page_version++;
            }
            else if(command.kind==MAZE_UI_SCORE_LEVEL) { nav.score_index=(nav.score_index+1)%maze_catalog_count;nav.page_version++; }
            else if(command.kind==MAZE_UI_SCORE_MODE) { nav.score_mode=1-nav.score_mode;nav.page_version++; }
            else
            {
                /* 装载之前关闭生产请求并删除持有旧地图的场景。 */
                maze_imu_request(app->generation,false);
                if(command.kind==MAZE_UI_BACK) maze_ui_clear();
                else maze_ui_loading();
                if(command.kind==MAZE_UI_BACK) maze_navigation_back(&nav);
                else
                {
                    rt_tick_t check_start=rt_tick_get();
                    error=command.kind==MAZE_UI_NEXT?maze_navigation_next(&nav):
                        maze_navigation_select(&nav,command.index);
                    rt_kprintf("[maze_map] result=%d check_ticks=%u workspace=%u\n",
                               error,rt_tick_get()-check_start,(unsigned)maze_map_workspace_size());
                    if(error==0 && nav.page==MAZE_NAV_GAME)
                        rt_kprintf("[maze_map] id=%s walls=%u traps=%u ice=%u nodes=%u reversed=%d\n",
                                   app->level->id,app->level->wall_count,app->level->trap_count,
                                   app->level->surface_count,nav.map.checked_nodes,nav.reversed);
                }
            }
        }
        maze_imu_sample_t sample;
        bool have_sample=maze_imu_latest(&sample);
        if(have_sample) maze_navigation_sample(&nav,&sample,now);
        if(nav.page==MAZE_NAV_GAME && app->state==MAZE_APP_PLAYING && now-app->last_update_tick>physics_max_gap)
            physics_max_gap=now-app->last_update_tick;
        maze_navigation_update(&nav,now);
        if(force_save && !store_ready)
        {
            rt_tick_t begin=rt_tick_get();
            error=maze_store_open(&store,&loaded);
            rt_kprintf("[maze_store] retry_open result=%d ticks=%u\n",error,rt_tick_get()-begin);
            if(!error)
            {
                store_ready=true;maze_progress_init(&nav.progress,&loaded);
                attempted_revision=store.revision;nav.page=MAZE_NAV_SELECT;
            }
            nav.store_error=error;nav.store_repair=store.repair;nav.page_version++;
            force_save=false;
        }
        if(nav.progress.revision_exhausted && nav.store_error!=MAZE_STORE_STALE)
        {
            nav.store_error=MAZE_STORE_STALE;nav.page_version++;
        }
        bool dirty=store_ready && nav.progress.archive.revision!=store.revision;
        if(store_ready && !nav.progress.revision_exhausted && maze_navigation_can_save(&nav) &&
           (force_save || (dirty && attempted_revision!=nav.progress.archive.revision)))
        {
            /* 结果/菜单才写，显式停止IMU；读取失败不伪装保存成功。 */
            maze_imu_request(app->generation,false);
            nav.save_pending=true;nav.page_version++;
            maze_ui_render(&nav);maze_ui_process();
            rt_tick_t begin=rt_tick_get();
            error=maze_store_save(&store,&nav.progress.archive);
            rt_kprintf("[maze_store] save result=%d ticks=%u revision=%lu active=%d\n",
                       error,rt_tick_get()-begin,(unsigned long)nav.progress.archive.revision,store.active);
            attempted_revision=nav.progress.archive.revision;
            nav.save_pending=false;nav.store_error=error;nav.store_repair=store.repair;nav.page_version++;
        }
        bool enabled;
        uint32_t request_generation;
        maze_navigation_request(&nav,&request_generation,&enabled);
        maze_imu_request(request_generation,enabled);
        bool changed=nav.page_version!=last_page_version || app->state!=last_state || app->generation!=last_generation;
        if(changed)
        {
            rt_kprintf("[maze_nav] page=%d version=%u index=%u reversed=%d error=%d\n",
                       nav.page,nav.page_version,nav.index,nav.reversed,nav.error);
            if(nav.page==MAZE_NAV_GAME)
            {
                if(app->state==MAZE_APP_DEVICE_ERROR)
                {
                    rt_kprintf("[maze_timing] loop_tick=%u service_tick=%u render_tick=%u previous_seq=%u previous_read=%u\n",
                               loop_gap,service_cost,last_render_cost,previous_sequence,previous_read);
                    if(have_sample)
                        rt_kprintf("[maze_timing] sample_seq=%u read_tick=%u age_tick=%u consumed_gap_tick=%u producer_gap_tick=%u producer_gap_count=%u\n",
                                   sample.sequence,sample.read_start_tick,now-sample.read_start_tick,
                                   sample.read_start_tick-previous_read,sample.read_gap_tick,sample.read_gap_count);
                }
                rt_kprintf("[maze] gen=%u state=%d error=%d count=%u\n",
                           app->generation,app->state,app->error,app->window_count);
                if(last_state==MAZE_APP_CALIBRATING && app->state!=MAZE_APP_CALIBRATING && app->has_reference && !app->error)
                    rt_kprintf("[maze] calibrated ref=%d,%d,%d\n",app->reference[0],app->reference[1],app->reference[2]);
            }
            last_state=app->state;
            last_generation=app->generation;
            last_page_version=nav.page_version;
        }
        if(changed || (rt_tick_t)(now-last_log)>=rt_tick_from_millisecond(500))
        {
            if(nav.page==MAZE_NAV_GAME && (enabled || changed))
            {
                rt_kprintf("[maze] gen=%u state=%d tick=%u ms=%lu pos=%ld,%ld input=%ld,%ld error=%d vel_centi=%ld,%ld surface=%d physics_gap_tick=%u flush_services=%u keys=%u switches=%u broken=%u checkpoint=%u\n",
                           app->generation,app->state,now,(unsigned long)app->elapsed_ms,
                           (long)app->model.x,(long)app->model.y,(long)app->input.x,(long)app->input.y,app->error,
                           (long)(app->model.vx*100),(long)(app->model.vy*100),
                           maze_level_surface_at(app->level,app->model.x,app->model.y),physics_max_gap,flush_services,
                           (unsigned)app->model.keys,(unsigned)app->model.switches,(unsigned)app->model.broken,app->model.checkpoint);
                physics_max_gap=flush_services=0;
            }
            last_log=now;
        }
        if(changed || (rt_tick_t)(now-last_ui)>=rt_tick_from_millisecond(50))
        {
            rt_tick_t render_start=rt_tick_get();
            maze_ui_render(&nav);
            last_render_cost=rt_tick_get()-render_start;
            last_ui=now;
        }
    }
    return 0;
#endif
}
