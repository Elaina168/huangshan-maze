/* 只替代NOR/RT边界，数据库与恢复算法使用SDK原样实现。 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "maze_store.h"

static uint8_t *flash;
static unsigned operations,erases,cut_operation,cut_prefix,reads,read_cut;
static bool armed,write_failure,read_failure,partial_failure;
static int allowed_bank=-1;
void maze_store_flash_allow(int bank) { allowed_bank=bank; }
static const struct fal_flash_dev device={.name="test_nor",.len=65536,.blk_size=8192,.write_gran=1};
static const struct fal_partition parts[]={
    {.name="maze_a",.flash_name="test_nor",.offset=0,.len=32768},
    {.name="maze_b",.flash_name="test_nor",.offset=32768,.len=32768}
};
int rt_kprintf(const char *format,...) { (void)format;return 0; }
uint32_t rt_tick_get_millisecond(void) { return 0; }
int fal_init(void) { return 2; }
const struct fal_flash_dev *fal_flash_device_find(const char *name)
{ assert(!strcmp(name,"test_nor"));return &device; }
const struct fal_partition *fal_partition_find(const char *name)
{
    for(unsigned i=0;i<2;i++) if(!strcmp(name,parts[i].name)) return &parts[i];
    return NULL;
}
int fal_partition_read(const struct fal_partition *part,uint32_t addr,uint8_t *buf,size_t size)
{
    assert(addr+size<=part->len);
    if(armed) reads++;
    if(read_failure || (read_cut && reads>=read_cut)) return -1;
    memcpy(buf,flash+part->offset+addr,size);return (int)size;
}
static int mutate(const struct fal_partition *part,uint32_t addr,const uint8_t *data,size_t size)
{
    assert(addr+size<=part->len);
    if(!data) assert(addr%8192==0 && size%8192==0);
    if(allowed_bank!=(int)(part->offset/32768)) return -1;
    if(armed) operations++;
    size_t n=size;
    bool cut=armed && operations==cut_operation;
    if(cut)
    {
        const size_t prefixes[]={0,1,size/2,size?size-1:0,size};
        n=prefixes[cut_prefix];
    }
    if(write_failure) return -1;
    if(partial_failure && n>1) n=1;
    for(size_t i=0;i<n;i++)
    {
        uint8_t *p=flash+part->offset+addr+i;
        if(data) { assert((*p&data[i])==data[i]);*p=data[i]; }
        else *p=255;
    }
    if(!data && armed) erases++;
    if(cut) { assert(msync(flash,65536,MS_SYNC)==0);_exit(77); }
    if(partial_failure) return -1;
    return (int)size;
}
int fal_partition_write(const struct fal_partition *part,uint32_t addr,const uint8_t *buf,size_t size)
{ return mutate(part,addr,buf,size); }
int fal_partition_erase(const struct fal_partition *part,uint32_t addr,size_t size)
{ return mutate(part,addr,NULL,size); }

static maze_store_t store;
static maze_archive_t archive;
static void variant(unsigned revision)
{
    archive.revision=revision;
    archive.levels[0].completed=1;archive.levels[1].unlocked=true;
    archive.levels[0].modes[0].losses=revision;
    archive.levels[0].modes[0].count=1;
    archive.levels[0].modes[0].scores[0]=(maze_score_t){1000+revision,revision};
}
int main(int argc,char **argv)
{
    assert(argc>=3);
    int fd=open(argv[2],O_RDWR);assert(fd>=0);
    flash=mmap(NULL,65536,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);assert(flash!=MAP_FAILED);
    if(argc>=6) { cut_operation=(unsigned)atoi(argv[4]);cut_prefix=(unsigned)atoi(argv[5]); }
    armed=!strcmp(argv[1],"init");
    read_failure=!strcmp(argv[1],"readerror");
    int error=maze_store_open(&store,&archive);
    if(error) { printf("ERROR %d\n",error);return 2; }
    if(!strcmp(argv[1],"version") || !strcmp(argv[1],"conflict"))
    {
        unsigned target=1u-(unsigned)store.active;
        maze_store_flash_allow((int)target);
        bool protect=false;uint32_t sector=8192;
        memset(&store.db[target],0,sizeof(store.db[target]));
        fdb_kvdb_control(&store.db[target],FDB_KVDB_CTRL_SET_NOT_FORMAT,&protect);
        fdb_kvdb_control(&store.db[target],FDB_KVDB_CTRL_SET_SEC_SIZE,&sector);
        assert(fdb_kvdb_init(&store.db[target],target?"maze_b":"maze_a",target?"maze_b":"maze_a",NULL,NULL)==0);
        uint8_t bytes[MAZE_ARCHIVE_BYTES];
        if(!strcmp(argv[1],"conflict")) archive.levels[0].modes[0].losses++;
        maze_archive_encode(&archive,bytes);
        if(!strcmp(argv[1],"version")) bytes[4]=2;
        uint32_t crc=maze_archive_crc32(bytes,MAZE_ARCHIVE_BYTES-4);
        for(unsigned i=0;i<4;i++) bytes[MAZE_ARCHIVE_BYTES-4+i]=(uint8_t)(crc>>(8*i));
        struct fdb_blob blob;
        assert(fdb_kv_set_blob(&store.db[target],"archive",fdb_blob_make(&blob,bytes,sizeof(bytes)))==0);
    }
    if(!strcmp(argv[1],"save") || !strcmp(argv[1],"retry") || !strcmp(argv[1],"partialretry") || !strcmp(argv[1],"readback"))
    {
        variant((unsigned)atoi(argv[3]));armed=true;operations=erases=reads=0;
        if(!strcmp(argv[1],"retry") || !strcmp(argv[1],"partialretry") || !strcmp(argv[1],"readback"))
        {
            write_failure=!strcmp(argv[1],"retry");
            partial_failure=!strcmp(argv[1],"partialretry");
            if(!strcmp(argv[1],"readback")) read_cut=(unsigned)atoi(argv[4]);
            assert(maze_store_save(&store,&archive)!=0);
            write_failure=partial_failure=false;read_cut=0;
        }
        error=maze_store_save(&store,&archive);
        if(error) { printf("ERROR %d\n",error);return 2; }
    }
    uint8_t bytes[MAZE_ARCHIVE_BYTES];maze_archive_encode(&archive,bytes);
    printf("STATE ");for(unsigned i=0;i<sizeof(bytes);i++) printf("%02x",bytes[i]);
    printf("\nOPS %u ERASES %u READS %u ACTIVE %d REPAIR %d\n",operations,erases,reads,store.active,store.repair);
    assert(msync(flash,65536,MS_SYNC)==0);assert(munmap(flash,65536)==0);close(fd);
    return 0;
}
