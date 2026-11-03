/* 固定小端字节格式，禁止直接把含填充的C结构写入Flash。 */
#include "maze_archive.h"
#include <string.h>

uint32_t maze_archive_crc32(const uint8_t *bytes,size_t size)
{
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<size;i++)
    {
        crc^=bytes[i];
        for(unsigned bit=0;bit<8;bit++) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return ~crc;
}

static void put(uint8_t **p,uint64_t value,unsigned bytes)
{
    for(unsigned i=0;i<bytes;i++) *(*p)++=(uint8_t)(value>>(i*8));
}
static uint64_t get(const uint8_t **p,unsigned bytes)
{
    uint64_t value=0;
    for(unsigned i=0;i<bytes;i++) value|=(uint64_t)*(*p)++<<(i*8);
    return value;
}

static const char *const level_ids[]={"ice_loops","mechanism_trial","dynamic_trial"};
/* 仅旧档读取识别旧ID，不保留取消的地图或游戏入口。 */
static const char *const legacy_ids[]={"frost_fork","ice_bends","ice_loops","mechanism_trial","dynamic_trial"};

void maze_archive_default(maze_archive_t *a)
{
    memset(a,0,sizeof(*a));
    for(unsigned i=0;i<MAZE_ARCHIVE_LEVELS;i++) strcpy(a->levels[i].id,level_ids[i]);
    a->levels[0].unlocked=true;
}

static void encode_mode(uint8_t **p,const maze_mode_record_t *m)
{
 put(p,m->losses,4);put(p,m->count,1);
 for(unsigned i=0;i<5;i++){put(p,m->scores[i].time_cs,8);put(p,m->scores[i].mistakes,4);}
}
static int decode_mode(const uint8_t **p,maze_mode_record_t *m)
{
 m->losses=(uint32_t)get(p,4);m->count=(unsigned)get(p,1);if(m->count>5)return MAZE_ARCHIVE_FORMAT;
 for(unsigned i=0;i<5;i++){
  m->scores[i].time_cs=get(p,8);m->scores[i].mistakes=(uint32_t)get(p,4);
  if(i&&i<m->count){maze_score_t prev=m->scores[i-1],cur=m->scores[i];if(cur.time_cs<prev.time_cs||(cur.time_cs==prev.time_cs&&cur.mistakes<prev.mistakes))return MAZE_ARCHIVE_FORMAT;}
 }
 return 0;
}
int maze_archive_encode(const maze_archive_t*a,uint8_t out[MAZE_ARCHIVE_BYTES])
{
 uint8_t*p=out;memcpy(p,"HMZ1",4);p+=4;put(&p,6,2);put(&p,MAZE_ARCHIVE_BYTES,2);put(&p,a->revision,8);put(&p,1,4);put(&p,3,1);
 for(unsigned i=0;i<3;i++){
  const maze_level_record_t*l=&a->levels[i];memcpy(p,l->id,32);p+=32;put(&p,l->unlocked,1);put(&p,l->completed,1);
  for(unsigned m=0;m<2;m++)encode_mode(&p,&l->modes[m]);
 }
 put(&p,a->random.valid,1);put(&p,a->random.generator_version,2);put(&p,a->random.seed,4);
 put(&p,maze_archive_crc32(out,MAZE_ARCHIVE_BYTES-4),4);return 0;
}
int maze_archive_decode(const uint8_t*bytes,size_t size,maze_archive_t*out)
{
 if(size<6||memcmp(bytes,"HMZ1",4))return MAZE_ARCHIVE_FORMAT;
 const uint8_t*p=bytes+4;unsigned version=(unsigned)get(&p,2);
 if(version<1||version>6)return MAZE_ARCHIVE_VERSION;
 size_t expected=version==1?353:version==2?517:version==3?681:version==4?845:version==5?982:524;
 unsigned levels=version==1?2:version==2?3:version==3?4:version<6?5:3;
 if(size>=20&&(bytes[16]!=1||bytes[17]||bytes[18]||bytes[19]))return MAZE_ARCHIVE_VERSION;
 if(size!=expected||get(&p,2)!=expected)return MAZE_ARCHIVE_FORMAT;
 maze_archive_t a;maze_archive_default(&a);a.revision=get(&p,8);
 if(get(&p,4)!=1)return MAZE_ARCHIVE_VERSION;
 if(get(&p,1)!=levels)return MAZE_ARCHIVE_FORMAT;
 const uint8_t*crc=bytes+size-4;if(get(&crc,4)!=maze_archive_crc32(bytes,size-4))return MAZE_ARCHIVE_CRC;
 unsigned seen=0;
 for(unsigned i=0;i<levels;i++){
  maze_level_record_t l={0};memcpy(l.id,p,32);p+=32;if(!memchr(l.id,0,32))return MAZE_ARCHIVE_FORMAT;
  const char *const *ids=version<6?legacy_ids:level_ids;
  unsigned index=0;while(index<levels&&strcmp(l.id,ids[index]))index++;
  if(index==levels||(seen&(1u<<index)))return MAZE_ARCHIVE_FORMAT;
  seen|=1u<<index;
  unsigned unlocked=(unsigned)get(&p,1);l.completed=(unsigned)get(&p,1);if(unlocked>1||l.completed>3)return MAZE_ARCHIVE_FORMAT;l.unlocked=unlocked!=0;
  for(unsigned m=0;m<2;m++){int error=decode_mode(&p,&l.modes[m]);if(error)return error;}
  if(version==6)a.levels[index]=l;
  else if(index>=2)a.levels[index-2]=l;
 }
 if(version<6)a.levels[0].unlocked=true;
 if(version<3)a.levels[1].unlocked=a.levels[0].completed!=0;
 if(version<4)a.levels[2].unlocked=a.levels[1].completed!=0;
 if(version>=5){
  const uint8_t*block=p;unsigned valid=(unsigned)get(&p,1);if(valid>1)return MAZE_ARCHIVE_FORMAT;
  a.random.valid=valid!=0;a.random.generator_version=(uint16_t)get(&p,2);a.random.seed=(uint32_t)get(&p,4);
  if(version==5)for(unsigned m=0;m<2;m++){maze_mode_record_t discarded;int error=decode_mode(&p,&discarded);if(error)return error;}
  if(valid&&!a.random.generator_version)return MAZE_ARCHIVE_FORMAT;
  if(!valid)for(unsigned i=0;i<(version==5?137u:7u);i++)if(block[i])return MAZE_ARCHIVE_FORMAT;
 }
 *out=a;return 0;
}
