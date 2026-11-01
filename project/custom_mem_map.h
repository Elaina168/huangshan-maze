/* 基于本SDK同板hcpu/custom_mem_map.h保留原区域；新增游戏专用FAL设备。 */
#ifndef __CUSTOM_MEM_MAP__
#define __CUSTOM_MEM_MAP__
#ifdef USING_PARTITION_TABLE
#include "ptab.h"
#endif
/* 此头也会被链接脚本预处理，C声明仅供实际FAL编译单元使用。 */
#ifdef _FAL_CFG_H_
struct fal_flash_dev;
extern const struct fal_flash_dev maze_save_flash;
#endif
/* 本板QSPI1为PSRAM，QSPI2为NOR；保留原NOR设备，另加游戏写保护设备。 */
#define FAL_FLASH_DEV_TABLE { &nor_flash2, &maze_save_flash }
#define FAL_PART_TABLE \
{ \
 {FAL_PART_MAGIC_WORD,"dfu",NOR_FLASH2_DEV_NAME,KVDB_DFU_REGION_OFFSET,KVDB_DFU_REGION_SIZE,0}, \
 {FAL_PART_MAGIC_WORD,"ble",NOR_FLASH2_DEV_NAME,KVDB_BLE_REGION_OFFSET,KVDB_BLE_REGION_SIZE,0}, \
 {FAL_PART_MAGIC_WORD,"maze_a","maze_flash",0,MAZE_SAVE_A_REGION_SIZE,0}, \
 {FAL_PART_MAGIC_WORD,"maze_b","maze_flash",MAZE_SAVE_B_REGION_OFFSET-MAZE_SAVE_A_REGION_OFFSET,MAZE_SAVE_B_REGION_SIZE,0}, \
}
#endif
