/*
This file contains the source code to implement communication with a SD Card
The file system is tested with a fat32 format
The communication is on the SPI bus.

*/
#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"

#define SDC_BUFFER_SIZE 512

typedef struct fatfs_write_buffer_t_ {
    uint16_t length;
    char data[SDC_BUFFER_SIZE];
} fatfs_write_buffer_t;

extern fatfs_write_buffer_t fatfs_write_buffer;

void fatfs_example(void);
void fatfs_init(void);
void fatfs_write(void);