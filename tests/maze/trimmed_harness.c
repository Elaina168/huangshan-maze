/* 当前三关、旧档迁移和娱乐模式的真实产品逻辑回归。 */
#include "maze_navigation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static maze_archive_t a,b,before;
static maze_navigation_t n;
static uint8_t input[982],encoded[524];
int main(void){
 assert(MAZE_ARCHIVE_LEVELS==3&&MAZE_ARCHIVE_BYTES==524&&maze_catalog_count==3);
 const char*ids[]={"ice_loops","mechanism_trial","dynamic_trial"};
 for(unsigned i=0;i<3;i++)assert(!strcmp(maze_catalog[i].definition->id,ids[i]));
 for(unsigned v=1;v<=5;v++){
  char name[80];snprintf(name,sizeof(name),"tests/maze/fixtures/archive-v%u.bin",v);
  FILE*f=fopen(name,"rb");assert(f);size_t bytes=fread(input,1,sizeof(input),f);fclose(f);
  assert(!maze_archive_decode(input,bytes,&a));assert(a.levels[0].unlocked);
  for(unsigned i=0;i<3;i++)assert(!strcmp(a.levels[i].id,ids[i]));
  assert(!maze_archive_encode(&a,encoded));assert(encoded[4]==6&&encoded[20]==3);
  unsigned old_count=input[20];
  for(unsigned i=2;i<old_count;i++){
   unsigned at=21+(i-2)*164,old_at=21+i*164;
   assert(!memcmp(encoded+at,input+old_at,32));
   assert(!memcmp(encoded+at+33,input+old_at+33,131));
  }
  assert(!maze_archive_decode(encoded,sizeof(encoded),&b));assert(!memcmp(&a,&b,sizeof(a)));
  if(v==5)assert(a.random.valid&&a.random.seed==3&&a.random.generator_version==1);
 }
 maze_archive_default(&a);maze_progress_t p;maze_progress_init(&p,&a);
 maze_app_t app;maze_level_t level={.id=MAZE_RANDOM_ID,.width=100,.height=100,.ball_radius=7,.start={25,25},.goal={75,75,10}};
 maze_app_init(&app,&level,1000);before=p.archive;
 maze_progress_before_event(&p,&app,MAZE_EVENT_START);
 app.state=MAZE_APP_LOST;app.elapsed_ms=200;maze_progress_observe(&p,&app);
 assert(!memcmp(&p.archive,&before,sizeof(before))&&p.mistakes==0&&p.rank==0);
 maze_progress_before_event(&p,&app,MAZE_EVENT_REPLAY);app.state=MAZE_APP_WON;maze_progress_observe(&p,&app);
 assert(!memcmp(&p.archive,&before,sizeof(before))&&p.rank==0);
 maze_navigation_init(&n,1000);assert(!maze_navigation_select(&n,0));assert(!strcmp(n.app.level->id,"ice_loops"));
 maze_progress_before_event(&n.progress,&n.app,MAZE_EVENT_START);n.app.state=MAZE_APP_WON;n.app.elapsed_ms=1230;maze_progress_observe(&n.progress,&n.app);
 assert(n.progress.archive.levels[0].modes[0].count==1&&n.progress.archive.levels[1].unlocked);
 assert(!maze_navigation_next(&n)&&n.index==1&&!strcmp(n.app.level->id,"mechanism_trial"));
 puts("PASS: three fixed IDs/first unlocked/next-level, v1-v5 migration retains kept scores, 524B v6 roundtrip; random win/loss never scores or changes archive");
}
