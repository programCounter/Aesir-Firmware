/*
//example used in debugging/testing
            strcpy(BSI_Attribute.BSI_Name, "BSI_TEST.txt");
            for(uint8_t inc; inc < 13; inc++)
            {
              BSI_Data.SensorValue = 0X0ff1;
              BSI_Data.CountMin = inc*20;       
              fatfs_write(&BSI_Data);
            }
This file contains the functions to pull data from input data stream, format it and store it to an SD card
Currently the variable that must be written to are:
-BSI_Attribute.BSI_Name
-BSI_Attribute.StartTime
-BSI_Data.SensorValue
-BSI_Data.CountMin
*/
#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "fatfs.h"


#define FILE_NAME   "NORDIC.TXT"
#define TEST_STRING "SD card example. NO CONFIG"
#define SDC_BUFFER_SIZE 512

#define SDC_SCK_PIN     NRF_GPIO_PIN_MAP(1,9)  ///< SDC serial clock (SCK) pin.
#define SDC_MOSI_PIN    NRF_GPIO_PIN_MAP(1,8)  ///< SDC serial data in (DI) pin.
#define SDC_MISO_PIN    NRF_GPIO_PIN_MAP(0,4)  ///< SDC serial data out (DO) pin.
#define SDC_CS_PIN      NRF_GPIO_PIN_MAP(0,26)  ///< SDC chip select (CS) pin.


static FATFS fs;
static FIL file;

fatfs_write_buffer_t fatfs_write_buffer = {};      //redefine the extern struct in fatfs.h
BSI_Att_t BSI_Attribute = {
          .BSI_Name = {},
          .StartTime = {20, 19, 12, 31, 23, 59},
          };
BSI_Data_t BSI_Data = {};

NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
        NRF_BLOCK_DEV_SDC_CONFIG(
                SDC_SECTOR_SIZE,
                APP_SDCARD_CONFIG(SDC_MOSI_PIN, SDC_MISO_PIN, SDC_SCK_PIN, SDC_CS_PIN)
         ),
         NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00")
);

// Initialize FATFS disk I/O interface by providing the block device.
static diskio_blkdev_t drives[] =
{
            DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
};

void data_ch_decode(BSI_Data_t * tempValue)
{
  tempValue->SensorCh = tempValue->SensorValue & 0x000F;
  tempValue->SensorValue = (tempValue->SensorValue >> 4);
}

//function to calculate the time that a measurement occured based on the minutes received
void current_time(uint16_t minutes, uint8_t *CurrentTime)
{
    uint8_t days_in_month, days, hours, mins;
    
    days = minutes/1440;            //this will ensure that time added to array is not greater the one increment for rollover
    hours = (minutes - (1440*days))/60;
    mins = (minutes - (1440*days) - (60*hours));


    //uint8_t StartTime[8]; //7 bytes (5+ 2byte year) YY/YY/MM/DD/HH/MM/SS
    //                                               [0][1][2][3][4][5][6]

    CurrentTime[5] += mins;
    CurrentTime[4] += hours;
    CurrentTime[3] += days;
    
    if(CurrentTime[5] >= 60) //rollover minutes & increment hours
    {
      CurrentTime[4]++;
      CurrentTime[5] %= 60;  //assign the remainder to the array
    }
    if(CurrentTime[4] >= 24) //rollover hours & increment days
    {
      CurrentTime[3]++;
      CurrentTime[4] %= 24;  //assign the remainder to the array
    }
    if(CurrentTime[2] == 1 ||
         CurrentTime[2] == 3 ||
         CurrentTime[2] == 5 ||
         CurrentTime[2] == 7 ||
         CurrentTime[2] == 8 ||
         CurrentTime[2] == 10 ||
         CurrentTime[2] == 12) //if the month has 31 days
    {
      days_in_month = 31;
    }
    else if(CurrentTime[2] == 2) //if it is feb
    {
      if((CurrentTime[1] % 4) == 0) //if it is a leap year
      {
        days_in_month = 29;
      }
      else
      {
        days_in_month = 28;
      }
    }
    else
    {
      days_in_month = 30;
    }
    if(CurrentTime[3] > days_in_month) //rollover days & increment months
    {
      
      CurrentTime[2]++;
      CurrentTime[3] %= days_in_month;  //assign the remainder to the array
    }
    if(CurrentTime[2] > 12) //rollover months & increment year
    {
      CurrentTime[1]++;
      CurrentTime[2] = 1;  //reset months
    }
    if(CurrentTime[1] > 99) //rollover year & increment centuries
    {
      
      CurrentTime[0]++;
      CurrentTime[1] = 0;  //reset
    }

}

void fatfs_write(BSI_Data_t * BSI_Value)
{
    uint32_t bytes_written;
    FRESULT ff_result;
    char SensorValTemp[5];
    uint8_t CurrentTimeTemp[8];
    char CurrentTimeStr[20];
    char TempStr[2];

    memset(CurrentTimeStr, 0, 20);
    strcpy(CurrentTimeTemp, BSI_Attribute.StartTime);
    data_ch_decode(BSI_Value);
    current_time(BSI_Value->CountMin, CurrentTimeTemp);
    
    for(uint8_t i = 0; i <= 5; i++) //for loop will contruct a char array of the time-date format form the int array passed
    {
      utoa(CurrentTimeTemp[i], TempStr, 10);
      //uint8_t StartTime[8]; //7 bytes (5+ 2byte year) YY/YY/MM/DD/HH/MM/SS
      //                                               [0][1][2][3][4][5][6]
      if(CurrentTimeTemp[i] < 10)
      {
        TempStr[1] = TempStr[0];
        TempStr[0] = '0';
      }
      if(i >= 2 && i <= 4)
      {
        strncat(CurrentTimeStr, "/", 1);
      }
      else if (i > 4)
      {
        strncat(CurrentTimeStr, ":", 1);
      }
      strncat(CurrentTimeStr, TempStr, 2);
    }
    //uint16_t fake_adc_val = 512;
    //strcpy(file_name, "HELLO2.txt");

    utoa(BSI_Value->SensorValue, SensorValTemp, 10); //convert sensor data to buffer data
    utoa(BSI_Value->SensorCh, TempStr, 10);
    memset(fatfs_write_buffer.data, 0, strlen(fatfs_write_buffer.data));
     //convert format to [time,ch,data]
    strncat(fatfs_write_buffer.data, CurrentTimeStr, strlen(CurrentTimeStr)); 
    strncat(fatfs_write_buffer.data, ",", 1);
    strncat(fatfs_write_buffer.data, TempStr, 1);
    strncat(fatfs_write_buffer.data, ",", 1);
    strncat(fatfs_write_buffer.data, SensorValTemp, 4);
    strncat(fatfs_write_buffer.data, "\n", 1);
    
    fatfs_write_buffer.length = strlen(fatfs_write_buffer.data);  
    
    //NRF_LOG_INFO("Writing to file " FILE_NAME "...");
    #ifdef DEMO_WRITE
    printf("Writing to file TEST...");
    #endif
    ff_result = f_open(&file, BSI_Attribute.BSI_Name, FA_READ | FA_WRITE | FA_OPEN_APPEND);
    if (ff_result != FR_OK)
    {
      //NRF_LOG_INFO("Unable to open or create file: " FILE_NAME ".");
      #ifdef DEMO_WRITE
      printf("Unable to open or create file: TEST.");
      #endif
      
    }

    ff_result = f_write(&file, fatfs_write_buffer.data, fatfs_write_buffer.length, (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
      //NRF_LOG_INFO("Write failed\r\n.");
      #ifdef DEMO_WRITE
      printf("Write failed\r\n.");
      #endif
    }
   else
   {
     //NRF_LOG_INFO("%d bytes written.", bytes_written);
      #ifdef DEMO_WRITE
      printf("%d bytes written.", bytes_written);
      #endif
      fatfs_write_buffer.length = 0;
   }

   (void) f_close(&file);
}

/**
 * @brief Function for demonstrating FAFTS usage.
 */
void fatfs_init()
{
    static DIR dir;
    static FILINFO fno;
    
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

    //NRF_LOG_INFO("Initializing disk 0 (SDC)...");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
        //NRF_LOG_INFO("Disk initialization failed.");
        #ifdef DEMO_WRITE
        printf("Disk initialization failed.");
        #endif
        return;
    }

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    //NRF_LOG_INFO("Capacity: %d MB", capacity);

    //NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        //NRF_LOG_INFO("Mount failed.");
        #ifdef DEMO_WRITE
        printf("Mount failed.");
        #endif
        return;
    }

    //NRF_LOG_INFO("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
        //NRF_LOG_INFO("Directory listing failed!");
        #ifdef DEMO_WRITE
        printf("Directory listing failed!");
        #endif
        return;
    }

    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
            //NRF_LOG_INFO("Directory read failed.");
            #ifdef DEMO_WRITE
            printf("Directory read failed.");
            #endif
            return;
        }

        if (fno.fname[0])
        {
            if (fno.fattrib & AM_DIR)
            {
                //NRF_LOG_RAW_INFO("   <DIR>   %s",(uint32_t)fno.fname);
            }
            else
            {
                //NRF_LOG_RAW_INFO("%9lu  %s\r\n", fno.fsize, NRF_LOG_PUSH(fno.fname));
            }
        }
    }
    while (fno.fname[0]);
    //NRF_LOG_RAW_INFO("");
    return;
}

void fatfs_example()
{
    static FATFS fs;
    static DIR dir;
    static FILINFO fno;
    static FIL file;

    uint32_t bytes_written;
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

//    NRF_LOG_INFO("Initializing disk 0 (SDC)...");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
//        NRF_LOG_INFO("Disk initialization failed.");
        return;
    }

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
//    NRF_LOG_INFO("Capacity: %d MB", capacity);

//    NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
//        NRF_LOG_INFO("Mount failed.");
        return;
    }

//    NRF_LOG_INFO("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
//        NRF_LOG_INFO("Directory listing failed!");
        return;
    }

    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
//            NRF_LOG_INFO("Directory read failed.");
            return;
        }

        if (fno.fname[0])
        {
            if (fno.fattrib & AM_DIR)
            {
//                NRF_LOG_RAW_INFO("   <DIR>   %s",(uint32_t)fno.fname);
            }
            else
            {
//                NRF_LOG_RAW_INFO("%9lu  %s", fno.fsize, (uint32_t)fno.fname);
            }
        }
    }
    while (fno.fname[0]);
//    NRF_LOG_RAW_INFO("");

//    NRF_LOG_INFO("Writing to file " FILE_NAME "...");
    ff_result = f_open(&file, FILE_NAME, FA_READ | FA_WRITE | FA_OPEN_APPEND);
    if (ff_result != FR_OK)
    {
//        NRF_LOG_INFO("Unable to open or create file: " FILE_NAME ".");
        return;
    }

    ff_result = f_write(&file, TEST_STRING, sizeof(TEST_STRING) - 1, (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
//        NRF_LOG_INFO("Write failed\r\n.");
    }
    else
    {
//        NRF_LOG_INFO("%d bytes written.", bytes_written);
    }

    (void) f_close(&file);
    return;
}