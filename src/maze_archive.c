/* 显式小端格式与稳定前5；旧档校验成功后一次发布。 */
#include "maze_archive.h"
#include <string.h>
static const char *const ids[]={"ice_loops","mechanism_trial","dynamic_trial"};
static const char *const legacy_ids[]={"frost_fork","ice_bends","ice_loops","mechanism_trial","dynamic_trial"};
static void put(uint8_t **p,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++)*(*p)++=(uint8_t)(v>>(8*i));}
static uint64_t get(const uint8_t **p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)*(*p)++<<(8*i);return v;}
uint32_t maze_archive_crc32(const uint8_t *b,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=b[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;}
void maze_archive_default(maze_archive_t *a){memset(a,0,sizeof(*a));for(unsigned i=0;i<3;i++)strcpy(a->levels[i].id,ids[i]);a->levels[0].unlocked=true;}
bool maze_score_precedes(maze_score_t a,maze_score_t b)
{
 if(a.time_cs!=b.time_cs)return a.time_cs<b.time_cs;
 if(a.mode!=b.mode)return a.mode>b.mode;
 return a.mistakes<b.mistakes;
}
unsigned maze_score_insert(maze_level_record_t *l,maze_score_t s)
{
 unsigned at=0;while(at<l->count&&!maze_score_precedes(s,l->scores[at]))at++;
 if(at>=MAZE_ARCHIVE_RANKS)return 0;
 if(l->count<MAZE_ARCHIVE_RANKS)l->count++;
 for(unsigned i=l->count-1;i>at;i--)l->scores[i]=l->scores[i-1];
 l->scores[at]=s;return at+1;
}
int maze_archive_encode(const maze_archive_t *a,uint8_t out[MAZE_ARCHIVE_BYTES])
{
 uint8_t*p=out;memcpy(p,"HMZ1",4);p+=4;put(&p,7,2);put(&p,MAZE_ARCHIVE_BYTES,2);put(&p,a->revision,8);put(&p,1,4);put(&p,3,1);
 for(unsigned i=0;i<3;i++){
  const maze_level_record_t*l=&a->levels[i];memcpy(p,l->id,32);p+=32;put(&p,l->unlocked,1);put(&p,l->completed,1);put(&p,l->losses,8);put(&p,l->count,1);
  for(unsigned j=0;j<5;j++){maze_score_t s=j<l->count?l->scores[j]:(maze_score_t){0};put(&p,s.time_cs,8);put(&p,s.mistakes,4);put(&p,s.mode,1);}
 }
 put(&p,a->random.valid,1);put(&p,a->random.generator_version,2);put(&p,a->random.seed,4);put(&p,maze_archive_crc32(out,MAZE_ARCHIVE_BYTES-4),4);return 0;
}
static int legacy_mode(const uint8_t **p,maze_level_record_t *l,unsigned mode)
{
 l->losses+=get(p,4);unsigned count=(unsigned)get(p,1);if(count>5)return MAZE_ARCHIVE_FORMAT;
 maze_score_t previous={0};
 for(unsigned i=0;i<5;i++){
  maze_score_t s={get(p,8),(uint32_t)get(p,4),mode};
  if(i<count){if(i&&maze_score_precedes(s,previous))return MAZE_ARCHIVE_FORMAT;maze_score_insert(l,s);previous=s;}
 }
 return 0;
}
int maze_archive_decode(const uint8_t *bytes,size_t size,maze_archive_t *out)
{
 if(size<6||memcmp(bytes,"HMZ1",4))return MAZE_ARCHIVE_FORMAT;
 const uint8_t*p=bytes+4;unsigned v=(unsigned)get(&p,2);
 if(v<1||v>7)return MAZE_ARCHIVE_VERSION;
 const unsigned lengths[]={0,353,517,681,845,982,524,356};
 if(size>=20&&(bytes[16]!=1||bytes[17]||bytes[18]||bytes[19]))return MAZE_ARCHIVE_VERSION;
 if(size!=lengths[v]||get(&p,2)!=size)return MAZE_ARCHIVE_FORMAT;
 maze_archive_t a;maze_archive_default(&a);a.revision=get(&p,8);if(get(&p,4)!=1)return MAZE_ARCHIVE_VERSION;
 unsigned levels=v==1?2:v==2?3:v==3?4:v<6?5:3;if(get(&p,1)!=levels)return MAZE_ARCHIVE_FORMAT;
 const uint8_t*tail=bytes+size-4;if(get(&tail,4)!=maze_archive_crc32(bytes,size-4))return MAZE_ARCHIVE_CRC;
 unsigned seen=0;
 for(unsigned i=0;i<levels;i++){
  maze_level_record_t l={0};memcpy(l.id,p,32);p+=32;if(!memchr(l.id,0,32))return MAZE_ARCHIVE_FORMAT;
  const char*const*names=v<6?legacy_ids:ids;unsigned index=0;while(index<levels&&strcmp(l.id,names[index]))index++;
  if(index==levels||(seen&(1u<<index)))return MAZE_ARCHIVE_FORMAT;
  seen|=1u<<index;
  unsigned unlocked=(unsigned)get(&p,1),complete=(unsigned)get(&p,1);if(unlocked>1||complete>(v==7?15u:3u))return MAZE_ARCHIVE_FORMAT;
  l.unlocked=unlocked!=0;l.completed=v==7?complete:(complete&1u)|((complete&2u)<<1);
  if(v==7){
   l.losses=get(&p,8);l.count=(unsigned)get(&p,1);if(l.count>5)return MAZE_ARCHIVE_FORMAT;
   for(unsigned j=0;j<5;j++){
    maze_score_t s={get(&p,8),(uint32_t)get(&p,4),(unsigned)get(&p,1)};
    if(s.mode>=MAZE_MODE_COUNT)return MAZE_ARCHIVE_FORMAT;
    if(j<l.count&&j&&maze_score_precedes(s,l.scores[j-1]))return MAZE_ARCHIVE_FORMAT;
    /* 空槽不能隐藏屏幕之外的成绩。 */
    if(j>=l.count&&(s.time_cs||s.mistakes||s.mode))return MAZE_ARCHIVE_FORMAT;
    l.scores[j]=s;
   }
  }else{int e=legacy_mode(&p,&l,0);if(!e)e=legacy_mode(&p,&l,MAZE_MODE_REVERSED);if(e)return e;}
  if(v>=6)a.levels[index]=l;else if(index>=2)a.levels[index-2]=l;
 }
 if(v<6)a.levels[0].unlocked=true;
 if(v<3)a.levels[1].unlocked=a.levels[0].completed!=0;
 if(v<4)a.levels[2].unlocked=a.levels[1].completed!=0;
 if(v>=5){
  const uint8_t*block=p;unsigned valid=(unsigned)get(&p,1);if(valid>1)return MAZE_ARCHIVE_FORMAT;
  a.random.valid=valid!=0;a.random.generator_version=(uint16_t)get(&p,2);a.random.seed=(uint32_t)get(&p,4);
  if(v==5){maze_level_record_t discarded={0};int e=legacy_mode(&p,&discarded,0);if(!e)e=legacy_mode(&p,&discarded,MAZE_MODE_REVERSED);if(e)return e;}
  if(valid&&!a.random.generator_version)return MAZE_ARCHIVE_FORMAT;
  if(!valid)for(unsigned j=0;j<(v==5?137u:7u);j++)if(block[j])return MAZE_ARCHIVE_FORMAT;
 }
 *out=a;return 0;
}
