/* 使用真实UI回调与LVGL事件字段读取，验证单槽事件交接；不渲染、不替代触摸实测。 */
#include <assert.h>
#include <stdio.h>
#include "lvgl.h"
#include "src/misc/lv_event_private.h"
#include "../../src/maze_ui.c"

int main(void)
{
    maze_app_t app={.state=MAZE_APP_READY,.generation=7};
    page_state=app.state; page_generation=app.generation;
    lv_event_t click={.code=LV_EVENT_CLICKED,.user_data=(void *)(uintptr_t)MAZE_EVENT_START};
    maze_event_t out=MAZE_EVENT_EXIT;
    assert(!maze_ui_take_event(&app,&out) && out==MAZE_EVENT_EXIT);
    button_clicked(&click);
    click.user_data=(void *)(uintptr_t)MAZE_EVENT_EXIT;
    button_clicked(&click);
    assert(app.state==MAZE_APP_READY && app.generation==7);
    assert(maze_ui_take_event(&app,&out) && out==MAZE_EVENT_START);
    assert(!maze_ui_take_event(&app,&out));
    puts("PASS: callback does not mutate app, first queued event wins and consumes once");

    button_clicked(&click);
    app.state=MAZE_APP_CALIBRATING;
    assert(!maze_ui_take_event(&app,&out));
    page_state=app.state; page_generation=app.generation;
    button_clicked(&click);
    app.generation++;
    assert(!maze_ui_take_event(&app,&out));
    puts("PASS: stale page state and stale generation events are discarded");

    page_state=app.state; page_generation=app.generation;
    button_clicked(&click);
    assert(maze_ui_take_event(&app,&out) && out==MAZE_EVENT_EXIT);
    puts("PASS: current page event remains usable after stale-event rejection (no rendering)");
    return 0;
}
