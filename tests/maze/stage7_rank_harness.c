#include "maze_archive.h"
#include "maze_progress.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){maze_archive_t a;maze_archive_default(&a);maze_level_record_t*l=&a.levels[0];
 for(unsigned m=0;m<4;m++)assert(maze_score_insert(l,(maze_score_t){1000,99-m,m})==1);
 assert(maze_score_insert(l,(maze_score_t){900,999,0})==1&&l->count==5);
 for(unsigned j=1;j<5;j++)assert(l->scores[j].mode==4-j);
 assert(maze_score_insert(l,(maze_score_t){1000,1,2})==3&&l->count==5);
 assert(maze_score_insert(l,(maze_score_t){1000,1,2})==4); /* 同分新记录在已有同分之后。 */
 maze_level_record_t before=*l;assert(maze_score_insert(l,(maze_score_t){2000,0,3})==0&&!memcmp(l,&before,sizeof(before)));
 for(unsigned i=0;i<100;i++){maze_score_insert(l,(maze_score_t){2000-i,i,i%4});assert(l->count==5);}
 maze_progress_t p;maze_archive_default(&a);maze_progress_init(&p,&a);maze_level_t level={.id="ice_loops",.width=100,.height=100,.ball_radius=7,.start={25,25},.goal={75,75,14}};
 maze_app_t app;maze_app_init(&app,&level,1000);app.reversed=true;app.dark=true;maze_progress_before_event(&p,&app,MAZE_EVENT_START);app.state=MAZE_APP_WON;app.elapsed_ms=1230;maze_progress_observe(&p,&app);
 assert(p.archive.levels[0].count==1&&p.archive.levels[0].scores[0].mode==3&&p.archive.levels[0].completed==8&&p.archive.levels[1].unlocked);
 uint64_t rev=p.archive.revision;maze_progress_observe(&p,&app);assert(p.archive.revision==rev);
 puts("PASS: time first, four mode priorities, mistakes third, stable ties, exact five/no worse overwrite, four-mode runtime score/unlock once");}
