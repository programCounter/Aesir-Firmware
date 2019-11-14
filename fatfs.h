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

typedef struct BSI_Att_t_ {
      char BSI_Name[16];
      uint8_t StartTime[8]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
}BSI_Att_t;

typedef struct BSI_Data_t_ {
      uint16_t CountMin;
      uint16_t SensorValue;
      uint8_t SensorCh;
}BSI_Data_t;

extern BSI_Data_t BSI_Data; //should be defined in main of program when ported ove to LOLI
extern BSI_Att_t BSI_Attribute;
extern fatfs_write_buffer_t fatfs_write_buffer;

void fatfs_example(void);
void fatfs_init(void);
void fatfs_write(BSI_Data_t * BSI_Value);
void data_ch_decode(BSI_Data_t * tempValue);