/* 从改前真实v6夹具检查新线格式与合并前5，不依赖新版插入器作预期。 */
#include "maze_archive.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t old[982],bytes[982];static maze_archive_t a;
static uint64_t number(unsigned at,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)bytes[at+i]<<(8*i);return v;}
int main(void){assert(MAZE_ARCHIVE_BYTES==356);FILE*f=fopen("tests/maze/fixtures/archive-v6.bin","rb");assert(f);size_t n=fread(old,1,sizeof(old),f);fclose(f);assert(n==524&&!maze_archive_decode(old,n,&a));assert(!maze_archive_encode(&a,bytes));
 assert(bytes[4]==7&&number(6,2)==356&&bytes[20]==3&&bytes[54]==5&&number(55,8)==19&&bytes[63]==5);
 const unsigned times[]={1000,1000,1001,1002,1002},modes[]={2,0,2,2,0},mistakes[]={1,1,2,3,2};
 for(unsigned i=0;i<5;i++){unsigned at=64+i*13;assert(number(at,8)==times[i]&&number(at+8,4)==mistakes[i]&&bytes[at+12]==modes[i]);}
 assert(bytes[345]==1&&number(346,2)==1&&number(348,4)==3);
 puts("PASS: original v6 two top5 merge into exactly five v7 rows, time/mode/mistakes order, completion mapping and random seed preserved");}
