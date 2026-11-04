/* 真实LVGL，异步LCD替身延迟读取源像素；不冒充物理DMA或板端观测。 */
#include "rtthread.h"
#include <math.h>
#include "src/display/lv_display_private.h"
#define main original_ui_main
#include "ui_refresh_harness.c"
#undef main
static const uint8_t *pending_pixels;
static lv_area_t pending_area;
static uint32_t pending_crc;
static bool pending,late_tail;
static unsigned transfers,overwrites,waits,services;
static unsigned minimum_ball=100000,black_pixels;
static uint32_t checksum(const uint8_t *bytes,const lv_area_t *area)
{
    size_t n=(size_t)(area->x2-area->x1+1)*(size_t)(area->y2-area->y1+1)*2;
    return maze_archive_crc32(bytes,n);
}
static unsigned ball_pixels(void)
{
    unsigned n=0;uint16_t cyan=lv_color_to_u16(lv_color_hex(0x7dd3fc));
    for(unsigned y=80;y<380;y++) for(unsigned x=20;x<370;x++) n+=frame[y*390+x]==cyan;
    return n;
}
static void consume(lv_display_t *d)
{
    if(!pending) return;
    if(pending_crc!=checksum(pending_pixels,&pending_area)) overwrites++;
    flush(d,&pending_area,(uint8_t *)pending_pixels);
    pending=false;transfers++;
    if(late_tail)
    {
        unsigned n=ball_pixels();if(n<minimum_ball) minimum_ball=n;
        for(unsigned y=80;y<380;y++) for(unsigned x=20;x<370;x++) black_pixels+=frame[y*390+x]==0;
    }
}
static void async_flush(lv_display_t *d,const lv_area_t *a,uint8_t *bytes)
{
    /* 前笔未完成又提交时，按LCD任务顺序才读取前笔源：可发现提前覆写。 */
    consume(d);
    pending_pixels=bytes;pending_area=*a;pending_crc=checksum(bytes,a);pending=true;
}
static void wait_complete(lv_display_t *d)
{
    waits++;
    consume(d);
}
static void service(void *context)
{
    lv_display_t *d=context;services++;
    /* 模拟旧LCD完成回调从sem_release后恢复，晚清当前新传输的flushing。 */
    if(late_tail && pending) d->flushing=0;
}
int main(void)
{
    lv_init();lv_display_t *d=lv_display_create(390,450);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(d,async_flush);lv_display_set_flush_wait_cb(d,wait_complete);
    assert(maze_ui_init()==0);maze_ui_set_service(service,d);
    maze_navigation_init(&navigation,1000);
    navigation.progress.archive.levels[0].unlocked=true;
    assert(maze_navigation_select(&navigation,0)==0);
    navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);consume(d);
    unsigned full_ball=ball_pixels();assert(full_ball==128);
    late_tail=true;
    for(unsigned i=0;i<200;i++)
    {
        if(i<40) { navigation.app.model.x=i%2?80:25;navigation.app.model.y=25; }
        else { navigation.app.model.x=25+fminf((i-40)*6,650);navigation.app.model.y=25+fminf((i-40)*5,550); }
        lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
    }
    consume(d);
    printf("async transfers=%u waits=%u services=%u overwritten_sources=%u ball=%u min_ball=%u black_pixels=%u\n",
           transfers,waits,services,overwrites,ball_pixels(),minimum_ball,black_pixels);fflush(stdout);
    assert(overwrites==0);
    assert(!pending && ball_pixels()==full_ball);
    assert(minimum_ball==full_ball && black_pixels==0);
    for(unsigned y=80;y<380;y++) for(unsigned x=20;x<370;x++) assert(frame[y*390+x]!=0);
    assert(services>=200 && waits>=200);
    screenshot("async-edge.ppm");maze_ui_clear();
    /* 原第三关交错路径保留，再覆盖第四关开门/破墙/快照回滚引起的重绘。 */
    late_tail=false;
    navigation.progress.archive.levels[1].unlocked=true;
    assert(maze_navigation_select(&navigation,1)==0);
    navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);consume(d);
    assert(ball_pixels()==full_ball);
    late_tail=true;
    for(unsigned i=0;i<100;i++)
    {
        navigation.app.model.x=25+fminf(i*7,navigation.app.level->width-50);
        navigation.app.model.y=25+fminf(i*6,navigation.app.level->height-50);
        navigation.app.model.keys=i%2?255:0;
        navigation.app.model.switches=i%2?255:0;
        navigation.app.model.broken=i%2?UINT32_MAX:0;
        lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
    }
    consume(d);
    printf("stage4 async transfers=%u waits=%u services=%u overwritten_sources=%u ball=%u min_ball=%u black_pixels=%u\n",
           transfers,waits,services,overwrites,ball_pixels(),minimum_ball,black_pixels);
    assert(overwrites==0 && !pending && ball_pixels()==full_ball && minimum_ball==full_ball && black_pixels==0);
    screenshot("stage4-async-edge.ppm");
    late_tail=false;navigation.progress.archive.levels[2].unlocked=true;
    assert(!maze_navigation_select(&navigation,2));navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);consume(d);late_tail=true;
    for(unsigned i=0;i<140;i++)
    {
        navigation.app.model.dynamic_us=i*50000;
        navigation.app.model.x=25+fminf(i*6,navigation.app.level->width-50);
        navigation.app.model.y=25+fminf(i*4,navigation.app.level->height-50);
        lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
    }
    consume(d);
    printf("stage5 async transfers=%u overwritten=%u min_ball=%u black=%u\n",transfers,overwrites,minimum_ball,black_pixels);
    assert(!overwrites && minimum_ball==full_ball && !black_pixels);
    screenshot("stage5-async-edge.ppm");maze_ui_clear();
    /* 使用实际生成并发布的地图，覆盖滚动和机关变化的延迟传输。 */
    late_tail=false;maze_navigation_back(&navigation);
    assert(!maze_navigation_random_open(&navigation,0));
    int result;
    do { result=maze_navigation_random_step(&navigation,0,NULL); } while(result==MAZE_RANDOM_PENDING);
    assert(result==MAZE_RANDOM_READY);
    assert(!maze_navigation_random_accept(&navigation,navigation.random_job_id));
    navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);consume(d);late_tail=true;
    for(unsigned i=0;i<140;i++)
    {
        navigation.app.model.dynamic_us=i*50000;
        navigation.app.model.x=25+fminf(i*6,navigation.app.level->width-50);
        navigation.app.model.y=25+fminf(i*4,navigation.app.level->height-50);
        navigation.app.model.keys=i%2?255:0;
        navigation.app.model.switches=i%2?255:0;
        lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
    }
    consume(d);
    printf("random async transfers=%u overwritten=%u min_ball=%u black=%u\n",transfers,overwrites,minimum_ball,black_pixels);
    assert(!overwrites && minimum_ball==full_ball && !black_pixels);
    screenshot("random-async-edge.ppm");maze_ui_clear();
    for(unsigned level=0;level<3;level++)
    {
        late_tail=false;maze_navigation_back(&navigation);navigation.dark=true;
        assert(!maze_navigation_select(&navigation,level));navigation.app.state=MAZE_APP_PLAYING;
        maze_ui_render(&navigation);lv_refr_now(d);consume(d);late_tail=true;
        for(unsigned i=0;i<80;i++){
            navigation.app.model.dynamic_us=i*50000;
            navigation.app.model.x=25+fminf(i*9,navigation.app.level->width-50);
            navigation.app.model.y=25+fminf(i*8,navigation.app.level->height-50);
            lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);
        }
        consume(d);assert(!overwrites&&minimum_ball==full_ball&&!black_pixels);maze_ui_clear();
    }
    late_tail=false;maze_navigation_back(&navigation);assert(!maze_navigation_random_open(&navigation,3));
    do{result=maze_navigation_random_step(&navigation,0,NULL);}while(result==MAZE_RANDOM_PENDING);
    assert(result==MAZE_RANDOM_READY);assert(!maze_navigation_random_accept(&navigation,navigation.random_job_id));navigation.app.state=MAZE_APP_PLAYING;
    maze_ui_render(&navigation);lv_refr_now(d);consume(d);late_tail=true;
    for(unsigned i=0;i<80;i++){navigation.app.model.x=25+fminf(i*9,navigation.app.level->width-50);navigation.app.model.y=25+fminf(i*8,navigation.app.level->height-50);lv_tick_inc(50);maze_ui_render(&navigation);lv_refr_now(d);}
    consume(d);assert(!overwrites&&minimum_ball==full_ball&&!black_pixels);
    printf("dark fixed/random async transfers=%u overwritten=%u min_ball=%u black=%u\n",transfers,overwrites,minimum_ball,black_pixels);
    screenshot("stage7-dark-async.ppm");maze_ui_clear();
    puts("PASS: fixed/random async delayed consumption, source stable, no black holes and complete ball");
}
