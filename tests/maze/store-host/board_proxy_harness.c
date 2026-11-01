/* 运行真实项目适配层，验证硬件地址、单副本保护及驱动错误不被吞掉。 */
#include <assert.h>
#include <stdio.h>
#include <fal.h>
extern const struct fal_flash_dev maze_save_flash;
void maze_store_flash_allow(int bank);
static uint32_t address;
static int calls,failure;
int rt_flash_read(uint32_t a,uint8_t *b,int n) { (void)b;address=a;calls++;return failure?0:n; }
int rt_flash_write(uint32_t a,const uint8_t *b,int n) { (void)b;address=a;calls++;return failure?0:n; }
int rt_flash_erase(uint32_t a,int n) { (void)n;address=a;calls++;return failure?-1:0; }
int main(void)
{
    uint8_t b[4]={0};
    assert(maze_save_flash.blk_size==8192);
    maze_store_flash_allow(-1);
    assert(maze_save_flash.ops.write(0,b,4)==-1 && maze_save_flash.ops.erase(0,8192)==-1 && calls==0);
    assert(maze_save_flash.ops.read(32768,b,4)==4 && address==0x12db8000);
    maze_store_flash_allow(0);
    assert(maze_save_flash.ops.write(0,b,4)==4 && address==0x12db0000);
    assert(maze_save_flash.ops.write(32768,b,4)==-1);
    assert(maze_save_flash.ops.write(32766,b,4)==-1);
    assert(maze_save_flash.ops.erase(0,8192)==8192);
    assert(maze_save_flash.ops.erase(0,4096)==-1);
    maze_store_flash_allow(1);
    assert(maze_save_flash.ops.erase(32768,32768)==32768 && address==0x12db8000);
    assert(maze_save_flash.ops.write(-1,b,4)==-1 && maze_save_flash.ops.read(65535,b,4)==-1);
    failure=1;
    assert(maze_save_flash.ops.write(32768,b,4)==-1 && maze_save_flash.ops.read(0,b,4)==-1);
    assert(maze_save_flash.ops.erase(32768,8192)==-1);
    puts("PASS: actual board proxy physical address, read-only/single-bank bounds, 8KiB erase and error propagation");
    return 0;
}
