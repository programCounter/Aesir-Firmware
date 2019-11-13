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

void fatfs_write()
{
    uint32_t bytes_written;
    FRESULT ff_result;
    char file_name[20];
    strcpy(file_name, "HELLO2.txt");

    #ifdef DEBUG
    strcpy(fatfs_write_buffer.data, "HELLO2");
    fatfs_write_buffer.length = 6;  //length of "HELLO" as int
    #endif

    //NRF_LOG_INFO("Writing to file " FILE_NAME "...");
    #ifdef DEBUG
    printf("Writing to file hello...");
    #endif
    ff_result = f_open(&file, file_name, FA_READ | FA_WRITE | FA_OPEN_APPEND);
    if (ff_result != FR_OK)
    {
      //NRF_LOG_INFO("Unable to open or create file: " FILE_NAME ".");
      #ifdef DEBUG
      printf("Unable to open or create file: HELLO.");
      #endif
      
    }

    ff_result = f_write(&file, fatfs_write_buffer.data, fatfs_write_buffer.length, (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
      //NRF_LOG_INFO("Write failed\r\n.");
      #ifdef DEBUG
      printf("Write failed\r\n.");
      #endif
    }
   else
   {
     //NRF_LOG_INFO("%d bytes written.", bytes_written);
      #ifdef DEBUG
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
        #ifdef DEBUG
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
        #ifdef DEBUG
        printf("Mount failed.");
        #endif
        return;
    }

    //NRF_LOG_INFO("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
        //NRF_LOG_INFO("Directory listing failed!");
        #ifdef DEBUG
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
            #ifdef DEBUG
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