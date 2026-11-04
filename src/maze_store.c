/* 两个独立FlashDB只交替修改非当前副本，数据库算法由SDK提供。 */
#include "maze_store.h"
#include <string.h>

static const char *const names[]={"maze_a","maze_b"};

static int blank(const struct fal_partition *part)
{
    uint8_t buf[256];bool empty=true;
    for(size_t off=0;off<part->len;off+=sizeof(buf))
    {
        if(fal_partition_read(part,(uint32_t)off,buf,sizeof(buf))!=(int)sizeof(buf)) return MAZE_STORE_IO;
        for(unsigned i=0;i<sizeof(buf);i++) if(buf[i]!=255) empty=false;
    }
    return empty?1:0;
}

static int mount_bank(maze_store_t *s,unsigned bank,bool protect)
{
    if(s->mounted[bank]) fdb_kvdb_deinit(&s->db[bank]);
    memset(&s->db[bank],0,sizeof(s->db[bank]));
    s->mounted[bank]=false;
    uint32_t sector=8192;
    fdb_kvdb_control(&s->db[bank],FDB_KVDB_CTRL_SET_SEC_SIZE,&sector);
    fdb_kvdb_control(&s->db[bank],FDB_KVDB_CTRL_SET_NOT_FORMAT,&protect);
    fdb_err_t error=fdb_kvdb_init(&s->db[bank],names[bank],names[bank],NULL,NULL);
    if(error) return -3200-(int)error;
    s->mounted[bank]=true;
    return 0;
}

static int read_archive(maze_store_t *s,unsigned bank,uint8_t *bytes,maze_archive_t *out,bool *legacy)
{
    struct fdb_blob blob;
    size_t read=fdb_kv_get_blob(&s->db[bank],"archive",fdb_blob_make(&blob,bytes,MAZE_ARCHIVE_MAX_BYTES));
    /* 未来格式的长度可能变化，先识别同一魔数的版本，禁止当坏副本清除。 */
    if(read>=6 && !memcmp(bytes,"HMZ1",4) && (bytes[4]<1 || bytes[4]>7 || bytes[5]!=0)) return MAZE_ARCHIVE_VERSION;
    if(read>=20 && !memcmp(bytes,"HMZ1",4) &&
       (bytes[16]!=1 || bytes[17] || bytes[18] || bytes[19])) return MAZE_ARCHIVE_VERSION;
    if((read!=MAZE_ARCHIVE_V1_BYTES && read!=MAZE_ARCHIVE_V2_BYTES && read!=MAZE_ARCHIVE_V3_BYTES && read!=MAZE_ARCHIVE_V4_BYTES && read!=MAZE_ARCHIVE_V5_BYTES && read!=MAZE_ARCHIVE_V6_BYTES && read!=MAZE_ARCHIVE_BYTES) || blob.saved.len!=read)
        return MAZE_ARCHIVE_FORMAT;
    int error=maze_archive_decode(bytes,read,out);
    if(!error) *legacy=bytes[4]<7;
    return error;
}

int maze_store_open(maze_store_t *s,maze_archive_t *out)
{
    memset(s,0,sizeof(*s));s->active=-1;
    maze_store_flash_allow(-1);
    if(fal_init()<0) return MAZE_STORE_IO;
    /* 仅主线程非重入打开；双镜像工作区移出main的8192字节栈。 */
    static uint8_t images[2][MAZE_ARCHIVE_MAX_BYTES];
    static maze_archive_t records[2];
    bool legacy[2]={false,false};
    for(unsigned i=0;i<2;i++)
    {
        s->parts[i]=fal_partition_find(names[i]);
        if(!s->parts[i] || s->parts[i]->len!=32768) return MAZE_STORE_PARTITION;
        const struct fal_flash_dev *dev=fal_flash_device_find(s->parts[i]->flash_name);
        if(!dev || dev->blk_size!=8192) return MAZE_STORE_PARTITION;
        int empty=blank(s->parts[i]);
        if(empty<0) return empty;
        if(empty) { s->status[i]=1;continue; }
        s->status[i]=mount_bank(s,i,true);
        if(!s->status[i]) s->status[i]=read_archive(s,i,images[i],&records[i],&legacy[i]);
        /* 比较同修订快照前，消除v1/v2/v3长度和合法ID顺序差异。 */
        if(!s->status[i]) maze_archive_encode(&records[i],images[i]);
        if(s->status[i]==MAZE_ARCHIVE_VERSION) return s->status[i];
    }
    for(unsigned i=0;i<2;i++) if(s->status[i]==0)
    {
        if(s->active<0 || records[i].revision>records[s->active].revision) s->active=(int)i;
        else if(records[i].revision==records[s->active].revision)
        {
            if(memcmp(images[i],images[s->active],MAZE_ARCHIVE_BYTES)) return MAZE_STORE_CONFLICT;
            if(legacy[s->active] && !legacy[i]) s->active=(int)i;
        }
    }
    if(s->active<0)
    {
        if(s->status[0]!=1 || s->status[1]!=1) return MAZE_STORE_NO_ARCHIVE;
        maze_archive_default(out);
        return maze_store_save(s,out);
    }
    s->revision=records[s->active].revision;
    memcpy(s->current,images[s->active],MAZE_ARCHIVE_BYTES);
    s->repair=s->status[1-s->active]<0;
    s->upgrade_pending=legacy[s->active];
    if(s->upgrade_pending && s->revision==UINT64_MAX) return MAZE_STORE_STALE;
    *out=records[s->active];
    /* 只准备RAM事务，成功提交前current/revision仍代表完整旧档。 */
    if(s->upgrade_pending) out->revision++;
    return 0;
}

int maze_store_save(maze_store_t *s,const maze_archive_t *a)
{
    uint8_t bytes[MAZE_ARCHIVE_BYTES],verify[MAZE_ARCHIVE_MAX_BYTES];maze_archive_t check;
    bool legacy=false;
    maze_archive_encode(a,bytes);
    if(s->active>=0)
    {
        if(a->revision<s->revision) return MAZE_STORE_STALE;
        if(a->revision==s->revision)
        {
            if(memcmp(bytes,s->current,sizeof(bytes))) return MAZE_STORE_CONFLICT;
            if(!s->repair) return 0;
        }
    }
    unsigned target=s->active<0?0:1u-(unsigned)s->active;
    int error=0;
    if(!s->mounted[target])
    {
        int empty=blank(s->parts[target]);if(empty<0) return empty;
        maze_store_flash_allow(empty?(int)target:-1);
        error=mount_bank(s,target,empty==0);
        maze_store_flash_allow(-1);
        if(!error && !empty)
        {
            int status=read_archive(s,target,verify,&check,&legacy);
            if(status==MAZE_ARCHIVE_VERSION) return status;
        }
        if(error && s->active>=0)
        {
            /* 另一份有效档案已在RAM确认，本次只重建损坏的非当前副本。 */
            maze_store_flash_allow((int)target);
            if(fal_partition_erase(s->parts[target],0,32768)!=32768)
            { maze_store_flash_allow(-1);return MAZE_STORE_IO; }
            error=mount_bank(s,target,false);
            maze_store_flash_allow(-1);
        }
        if(error) return error;
    }
    struct fdb_blob blob;
    maze_store_flash_allow((int)target);
    fdb_err_t result=fdb_kv_set_blob(&s->db[target],"archive",fdb_blob_make(&blob,bytes,sizeof(bytes)));
    maze_store_flash_allow(-1);
    if(result) error=-3200-(int)result;
    else
    {
        error=read_archive(s,target,verify,&check,&legacy);
        if(!error && (legacy || memcmp(bytes,verify,sizeof(bytes)))) error=MAZE_STORE_IO;
    }
    if(error) { s->mounted[target]=false;return error; }
    s->active=(int)target;s->revision=a->revision;s->status[target]=0;s->repair=false;
    s->upgrade_pending=false;
    memcpy(s->current,bytes,sizeof(bytes));
    return 0;
}
