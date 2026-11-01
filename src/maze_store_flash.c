/* 游戏专用NOR边界：探测只读，写擦只开放目标副本，不改SDK。 */
#include <fal.h>
#include "drv_flash.h"
#include "ptab.h"
#include "rtthread.h"
#include <stdbool.h>
static int allowed_bank=-1;
void maze_store_flash_allow(int bank) { allowed_bank=bank; }

static bool in_range(long offset,size_t size)
{
    return offset>=0 && (size_t)offset<=65536 && size<=65536-(size_t)offset;
}
static bool writable(long offset,size_t size)
{
    return in_range(offset,size) && allowed_bank>=0 && offset/32768==allowed_bank &&
        size<=32768-(size_t)offset%32768;
}
static int read_save(long offset,uint8_t *buf,size_t size)
{
    if(!in_range(offset,size)) return -1;
    int result=rt_flash_read(MAZE_SAVE_A_REGION_START_ADDR+(uint32_t)offset,buf,(int)size);
    return result==(int)size?result:-1;
}
static int write_save(long offset,const uint8_t *buf,size_t size)
{
    if(!writable(offset,size)) return -1;
    int result=rt_flash_write(MAZE_SAVE_A_REGION_START_ADDR+(uint32_t)offset,buf,(int)size);
    return result==(int)size?result:-1;
}
static int erase_save(long offset,size_t size)
{
    if(!writable(offset,size) || offset%8192 || size%8192) return -1;
    int result=rt_flash_erase(MAZE_SAVE_A_REGION_START_ADDR+(uint32_t)offset,(int)size);
    return result==RT_EOK?(int)size:-1;
}
const struct fal_flash_dev maze_save_flash={
    .name="maze_flash",.addr=MAZE_SAVE_A_REGION_START_ADDR,.len=65536,
    .blk_size=8192,.sector_size=8192,.ops={NULL,read_save,write_save,erase_save},.write_gran=1
};
