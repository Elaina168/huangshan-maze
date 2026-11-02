/* 显式格式与不可信Flash输入边界测试。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "maze_archive.h"

static void crc(uint8_t *b)
{
    uint32_t c=maze_archive_crc32(b,MAZE_ARCHIVE_BYTES-4);
    for(unsigned i=0;i<4;i++) b[MAZE_ARCHIVE_BYTES-4+i]=(uint8_t)(c>>(8*i));
}
int main(void)
{
    maze_archive_t a,b;
    uint8_t raw[MAZE_ARCHIVE_BYTES],copy[MAZE_ARCHIVE_BYTES];
    assert(MAZE_ARCHIVE_BYTES==681);
    assert(maze_archive_crc32((const uint8_t *)"123456789",9)==0xcbf43926);
    maze_archive_default(&a);
    assert(a.levels[0].unlocked && !a.levels[1].unlocked && !a.levels[2].unlocked && !a.levels[3].unlocked);
    assert(!strcmp(a.levels[0].id,"frost_fork") && !strcmp(a.levels[1].id,"ice_bends"));
    a.revision=0x0102030405060708ULL;
    a.levels[0].completed=1;
    a.levels[0].modes[0].losses=12;
    a.levels[0].modes[0].count=2;
    a.levels[0].modes[0].scores[0]=(maze_score_t){1234,2};
    a.levels[0].modes[0].scores[1]=(maze_score_t){1234,3};
    assert(maze_archive_encode(&a,raw)==0);
    assert(!memcmp(raw,"HMZ1",4) && raw[4]==3 && raw[6]==169 && raw[7]==2);
    assert(raw[8]==8 && raw[15]==1 && raw[20]==4);
    assert(maze_archive_decode(raw,sizeof(raw),&b)==0);
    assert(b.revision==a.revision && b.levels[0].modes[0].scores[1].mistakes==3);
    assert(maze_archive_encode(&b,copy)==0 && !memcmp(raw,copy,sizeof(raw)));
    assert(maze_archive_decode(raw,sizeof(raw)-1,&b)==MAZE_ARCHIVE_FORMAT);
    memcpy(copy,raw,sizeof(raw));copy[8]^=1;
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_CRC);
    memcpy(copy,raw,sizeof(raw));copy[4]=4;crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_VERSION);
    memcpy(copy,raw,sizeof(raw));copy[16]=2;crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_VERSION);
    memcpy(copy,raw,sizeof(raw));copy[20]=2;crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_FORMAT);
    memcpy(copy,raw,sizeof(raw));memset(copy+21,'x',32);crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_FORMAT);
    memcpy(copy,raw,sizeof(raw));copy[59]=6;crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_FORMAT);
    memcpy(copy,raw,sizeof(raw));copy[60]++;crc(copy);
    assert(maze_archive_decode(copy,sizeof(copy),&b)==MAZE_ARCHIVE_FORMAT);
    puts("PASS: 681-byte v3 LE archive, CRC32 vector, roundtrip, ID/count/version/truncation/sort/CRC boundaries");
    return 0;
}
