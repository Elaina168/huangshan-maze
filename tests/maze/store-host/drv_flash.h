#ifndef STORE_HOST_DRV_FLASH_H
#define STORE_HOST_DRV_FLASH_H
#include <stdint.h>
int rt_flash_read(uint32_t addr,uint8_t *buf,int size);
int rt_flash_write(uint32_t addr,const uint8_t *buf,int size);
int rt_flash_erase(uint32_t addr,int size);
#endif
