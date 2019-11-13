/*
This file contains the source code to implement communication with a SD Card
The file system is tested with a fat32 format
The communication is on the SPI bus.

*/
#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"

void fatfs_example(void);