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

void maze_archive_default(maze_archive_t *a)
{
    memset(a,0,sizeof(*a));
    strcpy(a->levels[0].id,"frost_fork");
    strcpy(a->levels[1].id,"ice_bends");
    a->levels[0].unlocked=true;
}

int maze_archive_encode(const maze_archive_t *a,uint8_t out[MAZE_ARCHIVE_BYTES])
{
    uint8_t *p=out;
    memcpy(p,"HMZ1",4);p+=4;
    put(&p,1,2);put(&p,MAZE_ARCHIVE_BYTES,2);put(&p,a->revision,8);put(&p,1,4);put(&p,2,1);
    for(unsigned l=0;l<2;l++)
    {
        const maze_level_record_t *level=&a->levels[l];
        memcpy(p,level->id,32);p+=32;
        put(&p,level->unlocked,1);put(&p,level->completed,1);
        for(unsigned m=0;m<2;m++)
        {
            const maze_mode_record_t *mode=&level->modes[m];
            put(&p,mode->losses,4);put(&p,mode->count,1);
            for(unsigned i=0;i<5;i++)
            {
                put(&p,mode->scores[i].time_cs,8);put(&p,mode->scores[i].mistakes,4);
            }
        }
    }
    put(&p,maze_archive_crc32(out,MAZE_ARCHIVE_BYTES-4),4);
    return 0;
}

int maze_archive_decode(const uint8_t *bytes,size_t size,maze_archive_t *out)
{
    if(size!=MAZE_ARCHIVE_BYTES || memcmp(bytes,"HMZ1",4)) return MAZE_ARCHIVE_FORMAT;
    const uint8_t *p=bytes+4;
    if(get(&p,2)!=1) return MAZE_ARCHIVE_VERSION;
    if(get(&p,2)!=MAZE_ARCHIVE_BYTES) return MAZE_ARCHIVE_FORMAT;
    maze_archive_t a={0}; a.revision=get(&p,8);
    if(get(&p,4)!=1) return MAZE_ARCHIVE_VERSION;
    if(get(&p,1)!=2) return MAZE_ARCHIVE_FORMAT;
    const uint8_t *crc=bytes+size-4;
    if(get(&crc,4)!=maze_archive_crc32(bytes,size-4)) return MAZE_ARCHIVE_CRC;
    for(unsigned l=0;l<2;l++)
    {
        maze_level_record_t *level=&a.levels[l];
        memcpy(level->id,p,32);p+=32;
        if(!memchr(level->id,0,32) ||
           (strcmp(level->id,"frost_fork") && strcmp(level->id,"ice_bends"))) return MAZE_ARCHIVE_FORMAT;
        unsigned unlocked=(unsigned)get(&p,1); level->completed=(unsigned)get(&p,1);
        if(unlocked>1 || level->completed>3) return MAZE_ARCHIVE_FORMAT;
        level->unlocked=unlocked!=0;
        for(unsigned m=0;m<2;m++)
        {
            maze_mode_record_t *mode=&level->modes[m];
            mode->losses=(uint32_t)get(&p,4);mode->count=(unsigned)get(&p,1);
            if(mode->count>5) return MAZE_ARCHIVE_FORMAT;
            for(unsigned i=0;i<5;i++)
            {
                mode->scores[i].time_cs=get(&p,8);mode->scores[i].mistakes=(uint32_t)get(&p,4);
                if(i && i<mode->count)
                {
                    maze_score_t prev=mode->scores[i-1],cur=mode->scores[i];
                    if(cur.time_cs<prev.time_cs ||
                       (cur.time_cs==prev.time_cs && cur.mistakes<prev.mistakes)) return MAZE_ARCHIVE_FORMAT;
                }
            }
        }
    }
    if(!strcmp(a.levels[0].id,a.levels[1].id)) return MAZE_ARCHIVE_FORMAT;
    if(strcmp(a.levels[0].id,"frost_fork"))
    {
        maze_level_record_t temp=a.levels[0];a.levels[0]=a.levels[1];a.levels[1]=temp;
    }
    *out=a;
    return 0;
}
