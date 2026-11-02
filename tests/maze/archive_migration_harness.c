/* v1/v2外部golden、v3规范顺序与错误不发布输出；不依赖存储或UI。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "maze_archive.h"
static void crc(uint8_t *b,size_t size)
{
    uint32_t c=maze_archive_crc32(b,size-4);
    for(unsigned i=0;i<4;i++) b[size-4+i]=(uint8_t)(c>>(8*i));
}
static void rejected(const uint8_t *b,size_t size,int error)
{
    maze_archive_t out;uint8_t before[sizeof(out)];
    memset(&out,0x5a,sizeof(out));memcpy(before,&out,sizeof(out));
    assert(maze_archive_decode(b,size,&out)==error);
    assert(!memcmp(before,&out,sizeof(out)));
}
int main(int argc,char **argv)
{
    assert(argc==3);
    assert(MAZE_ARCHIVE_BYTES==681 && MAZE_ARCHIVE_LEVELS==4);
    uint8_t golden[353],legacy[353],raw[681],canonical[681],mutated[682];
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(golden,1,sizeof(golden),f)==sizeof(golden) && fgetc(f)==EOF);assert(!fclose(f));
    maze_archive_t a,b;
    for(unsigned completed=0;completed<4;completed++)
    {
        memcpy(legacy,golden,353);legacy[218]=(uint8_t)completed;crc(legacy,353);
        assert(maze_archive_decode(legacy,353,&a)==0);
        assert(a.revision==17 && a.levels[0].completed==3 && a.levels[1].completed==completed);
        assert(a.levels[0].unlocked && a.levels[1].unlocked);
        assert(!strcmp(a.levels[2].id,"ice_loops") && a.levels[2].unlocked==(completed!=0));
        assert(a.levels[2].completed==0 && !a.levels[3].unlocked && !a.levels[3].completed);
        for(unsigned l=0;l<2;l++) for(unsigned m=0;m<2;m++)
        {
            maze_mode_record_t *r=&a.levels[l].modes[m];
            assert(r->losses==(l+1)*10+m && r->count==2);
            assert(r->scores[0].time_cs==1000+l*100+m*10 && r->scores[0].mistakes==2);
            assert(r->scores[1].time_cs==r->scores[0].time_cs && r->scores[1].mistakes==3);
            assert(r->scores[4].time_cs==50000+l*10+m && r->scores[4].mistakes==99);
        }
        for(unsigned m=0;m<2;m++)
        {
            assert(a.levels[2].modes[m].losses==0 && a.levels[2].modes[m].count==0);
            for(unsigned i=0;i<5;i++) assert(a.levels[2].modes[m].scores[i].time_cs==0 && a.levels[2].modes[m].scores[i].mistakes==0);
        }
        assert(maze_archive_encode(&a,raw)==0 && raw[4]==3 && raw[6]==169 && raw[7]==2 && raw[20]==4);
        assert(!memcmp(raw+21,legacy+21,328));
    }
    puts("PASS: v1 all legacy fields preserved including unused slots; completed 0/1/2/3 controls third unlock without revision bump");
    memcpy(legacy,golden,353);
    memcpy(legacy+21,golden+185,164);memcpy(legacy+185,golden+21,164);crc(legacy,353);
    assert(maze_archive_decode(legacy,353,&b)==0);
    assert(!strcmp(b.levels[0].id,"frost_fork") && b.levels[0].modes[0].losses==10 && b.levels[1].modes[0].losses==20);
    a.levels[2].completed=2;a.levels[2].modes[1].losses=7;a.levels[2].modes[1].count=1;
    a.levels[2].modes[1].scores[0]=(maze_score_t){9876,4};
    assert(maze_archive_encode(&a,canonical)==0);
    const unsigned permutations[][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    for(unsigned k=0;k<6;k++)
    {
        memcpy(raw,canonical,681);
        for(unsigned i=0;i<3;i++) memcpy(raw+21+i*164,canonical+21+permutations[k][i]*164,164);
        crc(raw,681);assert(maze_archive_decode(raw,681,&b)==0);
        assert(b.levels[2].completed==2 && b.levels[2].modes[1].scores[0].time_cs==9876);
        assert(maze_archive_encode(&b,raw)==0 && !memcmp(raw,canonical,681));
    }
    puts("PASS: legacy swapped order and all six v3 first-three ID permutations canonicalize without losing third data");
    for(size_t size=0;size<353;size++) rejected(golden,size,MAZE_ARCHIVE_FORMAT);
    for(size_t size=0;size<681;size++) rejected(canonical,size,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[681]=0;rejected(mutated,682,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,golden,353);memset(mutated+353,0,328);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[4]=4;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_VERSION);
    rejected(mutated,6,MAZE_ARCHIVE_VERSION);
    memcpy(mutated,canonical,681);mutated[16]=2;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_VERSION);
    memcpy(mutated,canonical,681);mutated[6]=4;rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[20]=2;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[8]^=1;rejected(mutated,681,MAZE_ARCHIVE_CRC);
    memcpy(mutated,golden,353);mutated[8]^=1;rejected(mutated,353,MAZE_ARCHIVE_CRC);
    memcpy(mutated,canonical,681);memset(mutated+21,'X',32);crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);memset(mutated+21,0,32);memcpy(mutated+21,"unknown",7);crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);memcpy(mutated+349,mutated+21,32);crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,golden,353);memcpy(mutated+185,mutated+21,32);crc(mutated,353);rejected(mutated,353,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,golden,353);memset(mutated+185,0,32);memcpy(mutated+185,"ice_loops",9);crc(mutated,353);rejected(mutated,353,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[53]=2;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[54]=4;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[59]=6;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,canonical,681);mutated[60]++;crc(mutated,681);rejected(mutated,681,MAZE_ARCHIVE_FORMAT);
    puts("PASS: truncation/oversize/format/rules/CRC/ID/flags/count/sort rejected with caller output untouched");
    /* 独立旧v2 golden保留全部三关字段与未使用成绩槽。 */
    uint8_t v2[MAZE_ARCHIVE_V2_BYTES];
    f=fopen(argv[2],"rb");assert(f);
    assert(fread(v2,1,sizeof(v2),f)==sizeof(v2) && fgetc(f)==EOF);assert(!fclose(f));
    for(unsigned completed=0;completed<4;completed++)
    {
        v2[382]=(uint8_t)completed;crc(v2,sizeof(v2));
        assert(!maze_archive_decode(v2,sizeof(v2),&b));
        assert(b.revision==29 && b.levels[3].unlocked==(completed!=0));
        assert(!strcmp(b.levels[3].id,"mechanism_trial") && !b.levels[3].completed);
        assert(!maze_archive_encode(&b,raw) && !memcmp(raw+21,v2+21,492));
        for(unsigned m=0;m<2;m++)
        {
            assert(!b.levels[3].modes[m].losses && !b.levels[3].modes[m].count);
            for(unsigned i=0;i<5;i++) assert(!b.levels[3].modes[m].scores[i].time_cs && !b.levels[3].modes[m].scores[i].mistakes);
        }
    }
    assert(!maze_archive_encode(&b,canonical));
    for(unsigned k=0;k<6;k++)
    {
        memcpy(raw,v2,sizeof(v2));
        for(unsigned i=0;i<3;i++) memcpy(raw+21+i*164,v2+21+permutations[k][i]*164,164);
        crc(raw,sizeof(v2));assert(!maze_archive_decode(raw,sizeof(v2),&b));
        assert(!maze_archive_encode(&b,raw) && !memcmp(raw,canonical,sizeof(raw)));
    }
    /* 24种四关顺序均归位；第四关有效/未使用成绩均参与往返。 */
    b.levels[3].completed=3;b.levels[3].modes[1].scores[4]=(maze_score_t){987654321,77};
    assert(!maze_archive_encode(&b,canonical));
    unsigned permutations4=0;
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<4;j++) for(unsigned k=0;k<4;k++) for(unsigned l=0;l<4;l++)
    {
        if(i==j || i==k || i==l || j==k || j==l || k==l) continue;
        unsigned order[]={i,j,k,l};memcpy(raw,canonical,sizeof(raw));
        for(unsigned n=0;n<4;n++) memcpy(raw+21+n*164,canonical+21+order[n]*164,164);
        crc(raw,sizeof(raw));assert(!maze_archive_decode(raw,sizeof(raw),&b));
        assert(!maze_archive_encode(&b,raw) && !memcmp(raw,canonical,sizeof(raw)));permutations4++;
    }
    assert(permutations4==24);
    for(size_t size=0;size<sizeof(v2);size++) rejected(v2,size,MAZE_ARCHIVE_FORMAT);
    memcpy(mutated,v2,sizeof(v2));mutated[16]=2;rejected(mutated,20,MAZE_ARCHIVE_VERSION);
    puts("PASS: independent v2 golden all 492 prior bytes preserved; fourth unlock 0/1/2/3; empty new records; six v2 and 24 v3 permutations");
    return 0;
}
