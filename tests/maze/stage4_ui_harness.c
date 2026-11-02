/* 第四阶段真实LVGL对象/快照显示验证；直接置状态不冒充物理或板端。 */
#include "rtthread.h"
#define main original_ui_main
#include "ui_refresh_harness.c"
#undef main

static void refresh(lv_display_t *display)
{
    maze_ui_render(&navigation);lv_refr_now(display);check_labels(lv_screen_active());
}
int main(void)
{
    lv_init();lv_display_t *display=lv_display_create(390,450);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display,flush);assert(maze_ui_init()==0);
    assert(lv_display_get_buf_active(display)->data_size==234000);
    assert(!lv_display_is_double_buffered(display));
    const maze_mechanism_t mechanisms[]={
        {MAZE_MECH_KEY,{60,30,20,20},0},
        {MAZE_MECH_SWITCH,{90,30,20,20},1},
        {MAZE_MECH_KEY_DOOR,{130,20,20,60},0},
        {MAZE_MECH_SWITCH_DOOR,{170,20,20,60},1},
        {MAZE_MECH_BREAKABLE,{210,20,20,60},0},
        {MAZE_MECH_CHECKPOINT,{250,30,20,20},0}};
    const maze_surface_area_t rough={.area={20,100,280,60},.surface=MAZE_SURFACE_ROUGH};
    const maze_level_t level={.width=350,.height=300,.ball_radius=6,.start={25,25},
        .goal={320,270,10},.surfaces=&rough,.surface_count=1,
        .id=MAZE_LEVEL_ID_FROST_FORK,.mechanisms=mechanisms,.mechanism_count=6};
    maze_navigation_init(&navigation,1000);
    navigation.page=MAZE_NAV_GAME;
    maze_app_init(&navigation.app,&level,1000);
    refresh(display);
    screenshot("stage4-ready.ppm");
    lv_obj_t *view=lv_obj_get_child(lv_screen_active(),4),*world=lv_obj_get_child(view,0);
    assert(lv_obj_get_width(view)==350 && lv_obj_get_height(view)==300);
    assert(lv_obj_get_child_count(world)==9);
    lv_obj_t *objects[6];for(unsigned i=0;i<6;i++) objects[i]=lv_obj_get_child(world,2+i);
    lv_obj_t *original_ball=lv_obj_get_child(world,8);
    navigation.app.state=MAZE_APP_PLAYING;refresh(display);
    for(unsigned i=0;i<6;i++) assert(!lv_obj_has_flag(objects[i],LV_OBJ_FLAG_HIDDEN));
    navigation.app.model.keys=1;navigation.app.model.switches=2;
    navigation.app.model.broken=1u<<4;navigation.app.model.checkpoint=6;
    refresh(display);
    assert(lv_obj_has_flag(objects[0],LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(objects[2],LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(objects[3],LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(objects[4],LV_OBJ_FLAG_HIDDEN));
    assert(lv_color_eq(lv_obj_get_style_bg_color(objects[1],0),lv_color_hex(0x754782)));
    assert(lv_obj_get_style_border_width(objects[5],0)==2);
    assert(strstr(lv_label_get_text(lv_obj_get_child(lv_screen_active(),1)),"钥匙1 开关1"));
    screenshot("stage4-open.ppm");
    navigation.app.model.x=25;navigation.app.model.y=120;refresh(display);
    assert(strstr(lv_label_get_text(lv_obj_get_child(lv_screen_active(),1)),"粗糙地面"));
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    for(unsigned i=0;i<80;i++)
    {
        navigation.app.model.keys=i%2;navigation.app.model.switches=i%2?2:0;
        navigation.app.model.broken=i%2?1u<<4:0;navigation.app.model.checkpoint=i%2?6:0;
        refresh(display);
        assert(lv_obj_get_child(world,8)==original_ball && lv_obj_get_child_count(world)==9);
    }
    lv_mem_monitor(&after);assert(before.free_size==after.free_size);
    /* 检查点之后机关变化可回滚；UI显示恢复后的本轮状态。 */
    navigation.app.model.checkpoint=6;navigation.app.model.keys=1;
    navigation.app.model.switches=0;navigation.app.model.broken=0;refresh(display);
    assert(lv_obj_has_flag(objects[2],LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(objects[3],LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(objects[4],LV_OBJ_FLAG_HIDDEN));
    navigation.app.state=MAZE_APP_LOST;navigation.store_error=-3102;refresh(display);
    assert(caption(lv_screen_active(),"复活") && caption(lv_screen_active(),"重玩") &&
        caption(lv_screen_active(),"选关") && caption(lv_screen_active(),"保存"));
    maze_ui_command_t command;click("复活");
    assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_GAME && command.game_event==MAZE_EVENT_RESPAWN);
    screenshot("stage4-respawn.ppm");
    navigation.app.model.checkpoint=0;navigation.app.model.keys=0;
    navigation.app.state=MAZE_APP_CALIBRATING;refresh(display);
    for(unsigned i=0;i<6;i++) assert(!lv_obj_has_flag(objects[i],LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_border_width(objects[5],0)==0);
    navigation.app.state=MAZE_APP_LOST;refresh(display);assert(!caption(lv_screen_active(),"复活"));
    maze_ui_clear();maze_navigation_init(&navigation,1000);
    assert(maze_catalog_count==4);
    for(unsigned i=0;i<4;i++) navigation.progress.archive.levels[i].unlocked=true;
    navigation.store_error=-3102;refresh(display);
    for(unsigned i=0;i<4;i++)
    {
        lv_obj_t *c=caption(lv_screen_active(),maze_catalog[i].title);assert(c);
        lv_obj_t *row=lv_obj_get_parent(lv_obj_get_parent(c));
        assert(lv_obj_get_y(row)==112+(int)i*46 && lv_obj_get_height(row)==42);
        click(maze_catalog[i].title);
        assert(maze_ui_take_command(&navigation,&command) && command.kind==MAZE_UI_SELECT && command.index==i);
    }
    assert(caption(lv_screen_active(),"成绩") && caption(lv_screen_active(),"控制方向：正常") && caption(lv_screen_active(),"保存"));
    screenshot("stage4-selection.ppm");
    navigation.store_error=0;
    assert(maze_navigation_select(&navigation,3)==0);refresh(display);
    screenshot("stage4-formal-ready.ppm");
    navigation.app.state=MAZE_APP_PLAYING;refresh(display);
    screenshot("stage4-formal-playing.ppm");
    navigation.app.state=MAZE_APP_WON;refresh(display);
    assert(caption(lv_screen_active(),"全部完成") && !caption(lv_screen_active(),"下一关"));
    navigation.page=MAZE_NAV_SCORES;navigation.score_index=3;navigation.score_mode=1;navigation.page_version++;
    navigation.progress.archive.levels[3].completed=2;
    navigation.progress.archive.levels[3].modes[1].count=1;
    navigation.progress.archive.levels[3].modes[1].scores[0]=(maze_score_t){12345,2};refresh(display);
    assert(caption(lv_screen_active(),"成绩 | 机关试炼") && caption(lv_screen_active(),"反转模式"));
    screenshot("stage4-scores.ppm");
    maze_ui_clear();
    puts("PASS: four menus/save/mode/scores/final, rough/closed/open/reset/snapshot objects, respawn command, fixed viewport/buffer and stable object heap");
}
