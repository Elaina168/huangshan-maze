/* 外部存档边界：损坏数据不能部分修改调用方，迁移不会弱化校验。 */
#include "maze_archive.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static maze_archive_t a,b,previous;
static uint8_t bytes[MAZE_ARCHIVE_BYTES+1],valid[MAZE_ARCHIVE_BYTES];
static void crc(void){uint32_t c=maze_archive_crc32(bytes,MAZE_ARCHIVE_BYTES-4);for(unsigned i=0;i<4;i++)bytes[MAZE_ARCHIVE_BYTES-4+i]=(uint8_t)(c>>(8*i));}
static void bad(size_t size,int error){previous=b;assert(maze_archive_decode(bytes,size,&b)==error);assert(!memcmp(&previous,&b,sizeof(b)));memcpy(bytes,valid,sizeof(valid));}
int main(void){maze_archive_default(&a);a.revision=71;maze_archive_encode(&a,valid);memcpy(bytes,valid,sizeof(valid));b=a;b.revision=99;
 assert(maze_archive_crc32((const uint8_t*)"123456789",9)==0xcbf43926u);
 bad(MAZE_ARCHIVE_BYTES-1,MAZE_ARCHIVE_FORMAT);bad(MAZE_ARCHIVE_BYTES+1,MAZE_ARCHIVE_FORMAT);
 bytes[40]^=1;bad(sizeof(valid),MAZE_ARCHIVE_CRC);
 bytes[4]=8;bad(sizeof(valid),MAZE_ARCHIVE_VERSION);
 bytes[16]=2;bad(sizeof(valid),MAZE_ARCHIVE_VERSION);
 bytes[20]=4;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 memcpy(bytes+129,bytes+21,32);crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 memset(bytes+21,'x',32);crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[63]=6;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[63]=2;bytes[64]=2;bytes[77]=1;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[63]=1;bytes[76]=4;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[76]=1;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[348]=1;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 bytes[345]=2;crc();bad(sizeof(valid),MAZE_ARCHIVE_FORMAT);
 const unsigned permutations[][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 for(unsigned p=0;p<6;p++){for(unsigned i=0;i<3;i++)memcpy(bytes+21+i*108,valid+21+permutations[p][i]*108,108);crc();assert(!maze_archive_decode(bytes,sizeof(valid),&b));assert(!memcmp(&a,&b,sizeof(a)));}
 puts("PASS: CRC32 vector, truncated/oversized/future/rules/duplicate/unknown/count/sort/random identity rejected without output changes; six stable-ID orders canonicalize");
}
