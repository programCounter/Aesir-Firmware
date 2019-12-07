//#define SECTOR_SIZE 4096 //4kB //already defined in .h?

#include "nrf_drv_qspi.h"
#include "bsi_qspi.h"
#include "bsp_btn_ble.h"
#include <string.h>
#include "bsi_config.h"
#include <time.h>
#include "bsi_measure.h"

#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99

#define WAIT_FOR_PERIPH() do { \
        while (!m_finished) {} \
        m_finished = false;    \
    } while (0)


// *********************************************************************************************
//                            S o m e    T e r m i n o l o g y
// *********************************************************************************************
// a "memory page" (for our chip) is 1 Byte (each address represents this minimum value)
// ...however, our "QSPI PAGE" is the size of each sensor reading data set "QSPI_PAGE_" (4 Bytes)
// a memory sector is 4kB (this is the minimum amount of data we can delete)
// a memory block is 64kB (thus there are 16 sectors per block!)


// *********************************************************************************************
//                   D e c l a r a t i o n s / I n i t i a l i z a t i o n s
// *********************************************************************************************
// Below is a "look-up table" used to find start address for sector block 1
const uint32_t SectorB1[] = {0,4096,8192,12288,16384,20480,24576,28672,32768,36864,40960,45056,49152,53248,57344,61440,65536};


// *** DEPRECATED VARIABLES THAT MAYBE WE SHOULD COMMENT OUT ***
uint16_t pressCount = 0;
//static volatile uint32_t LastKnownAddr = 4096; //start at beginning of 1st sector (do not use sector 0)
//static uint8_t currentSector;  //sector 1 starts at address 4096
// ************************************************************* 
bool lwrite_qspi = false;
bool lread_qspi = false;
bool lerase_sector = false;
bool m_finished = false; // used in the QSPI event
volatile time_t RawTime = 1574731926;
struct tm *info;
uint32_t ticksTUpload;

// Entire first sector is reserved for header (prevent erasing)
// Temp header to be used to change header in sector zero...
// ... populate Header then call "qspi_write_header()"
BSI_Header Header = { 
    .BSI_Name = {},
    .StartTime = {}, //   YY/YY/MM/DD/HH/MM
    };

// Empty header to read TestHeader back out of memory
BSI_Header ReadHeader = {
    };

// The temporary struct to inject with data before calling "write_qspi_page()"
QSPI_Page CurrentPage = {
    .countMin = 0, //minutes since Header StartTime[] (last Local Listener connection)
    //.sensorCh = 1, //which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    .sensorValue = 420, //What reading did sensor take? (10b ADC or pulse)
    };

// Only used for checking an single, specific sensor reading
QSPI_Page ReadPage = { 
    };

// The temporary struct filled when calling "read_qspi_sector(uint8_t Sector)"
QSPI_Sector ReadSector = {
    .Page = {0},  // set every page to all zeros (clear array)
    //.xTransmitted = false,
    };

// The temporary general packet populated with a sector when calling "qspi_prepare_packet(uint8_t Sector)"
Ad_gPacket gPacket = {
    .imSendingYouData = {'#','#','#'},
    };


// *********************************************************************************************
//                                    F u n c t i o n s
// *********************************************************************************************

//Event called when the QSPI completes an action
void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context)
{
    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(p_context);
    m_finished = true;// used in the "While QSPI Busy" function.
}

void configure_memory()
{
    //Qspi init
    uint8_t temporary = 0x40;
    uint32_t err_code;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = QSPI_STD_CMD_RSTEN,
        .length    = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = true,
        .wren      = true
    };

    // Send reset enable
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Send reset command
    cinstr_cfg.opcode = QSPI_STD_CMD_RST;
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Switch to qspi mode
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, &temporary, NULL);
    APP_ERROR_CHECK(err_code);

    // Landon added code past here in configure_memory()
    //currentSector = (bsi_config.qspi_currentSector); // Which sector are we in after waking up?
}

#ifdef DEBUG_QSPI //The header is no longer in the QSPI
// ***********************************************************************************
// used to ERASE the old header and UPDATE with new header!
// I need to add another function for just updating start time (or just do it manually?)...
// ...but one thing at a time...
void write_qspi_header()
{
          /*ret_code_t err_code;

          nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, 0); //Erase 4kB from &0 (erase sector 0)
          WAIT_FOR_PERIPH();
          //APP_ERROR_CHECK(err_code);
          
          memcpy(&Header.BSI_Name, &bsi_config.BSI_Name, sizeof(Header.BSI_Name)); 
          err_code = nrf_drv_qspi_write(&Header, sizeof(Header), 0);

          APP_ERROR_CHECK(err_code);
          WAIT_FOR_PERIPH();

          if(err_code == NRF_SUCCESS) { //If write was success...
            //bsp_board_led_on(3);// If header update was success... do something?
          }
          */
          lwrite_qspi = false; 
}
#endif


// ***********************************************************************************
// Used to write pages to the memory using last known address (see page struct for more info)
// populate CurrentPage with data, then call write_qspi_page to write it!
// *** This should be used to write each sensor reading to memory ***
void write_qspi_page()
{
          
       ret_code_t err_code;

       err_code = nrf_drv_qspi_write(&CurrentPage, sizeof(CurrentPage), bsi_config.lastKnownAddr);

       APP_ERROR_CHECK(err_code);
       WAIT_FOR_PERIPH();


        //If write was success, then increment lastKnownAddr...
        // ...and write the last known address to the config so it survives power cycles.
        if(err_code == NRF_SUCCESS) 
        { 
          bsi_config.lastKnownAddr += (sizeof(CurrentPage));
          bsi_config.configChanged = true;
          
          // if we have reached a new sector...
          if((bsi_config.lastKnownAddr) >= (SectorB1[bsi_config.qspi_currentSector+1]))
          {
            if(bsi_config.qspi_currentSector == 15)
            {
               bsi_config.qspi_currentSector = 0; //needs to erase sectors :(
               bsi_config.lastKnownAddr = 0;   // = SectorB1[bsi_config.qspi_currentSector];
            }
            else
            {
               bsi_config.qspi_currentSector+=1;
            }
            erase_qspi_sector(bsi_config.qspi_currentSector); //erase this sector so we can reuse it
          }//end of if new sector
                   
        }//end if(err_code == NRF_SUCCESS)
      lwrite_qspi = false; 

}//end write_qspi_page()


// ***********************************************************************************
// Used to erase an entire sector ( 1 - 15 , but hopefully not 0 which is the header )
// Should only be called interally when end of memory is reached and a new empty sector is needed
void erase_qspi_sector(uint8_t Sector)
{

      ret_code_t err_code;
           
      err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, SectorB1[Sector]);
      WAIT_FOR_PERIPH();
      
      //debugging
      //bsi_config.lastKnownAddr = 4096; // point back to start of sector 1
      //bsi_config.configChanged = true;

      //nrf_drv_qspi_chip_erase(); // only to be used to erase entire chip
      //printf("QSPI sector %u erased\n", Sector);
      lerase_sector = false;      
}

#ifdef DEBUG_QSPI
// ***********************************************************************************
// becoming "legacy" or deprecated
// Used to write anything sizeof(m_buffer_tx) to a specific address
void write_qspi(uint32_t Address) 
{
      ret_code_t err_code; 

      err_code = nrf_drv_qspi_write(&Header, sizeof(Header), Address);
      APP_ERROR_CHECK(err_code);
      WAIT_FOR_PERIPH();

      memset(&m_buffer_tx, 0, sizeof(m_buffer_tx)); //clean out tx buffer AFTER tx'ing 
      lwrite_qspi = false; 
}
#endif

#ifdef DEBUG_QSPI
// ***********************************************************************************
// becoming "legacy" or deprecated
// Used to read anything sizeof(m_buffer_rx) from a specific address
void read_qspi(uint32_t Address)
{
    ret_code_t err_code;
    memset(&m_buffer_rx, 0, sizeof(m_buffer_rx)); //clear m_buffer_rx for next read

    err_code = nrf_drv_qspi_read(m_buffer_rx, sizeof(m_buffer_rx), (Address));

    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    lread_qspi = false;
}
#endif


// ***********************************************************************************
// used ONLY to read the header from the FDS (used to be on QSPI, now it is not)
void read_qspi_header(){
    //uint8_t testTimeArray[] = {20,19,11,26,16,22};
    //ret_code_t err_code;
    memset(&ReadHeader, 0, sizeof(ReadHeader)); //Clear the read header first
    //strcpy(ReadHeader.BSI_Name, "BSITEST");
    //memcpy(ReadHeader.StartTime, testTimeArray, 6);
    memcpy(ReadHeader.BSI_Name, bsi_config.BSI_Name, strlen(bsi_config.BSI_Name));
    //memcpy(ReadHeader.StartTime, bsi_config., sizeof(bsi_config.BSI_Name));

    //err_code = nrf_drv_qspi_read(&ReadHeader, sizeof(ReadHeader), 0); //Header is always kept at &0
    //APP_ERROR_CHECK(err_code);
    //WAIT_FOR_PERIPH();
    ReadHeader.StartTime[0] = 20; // Can't wait to see the Y21K people freak about this
    ReadHeader.StartTime[1] = (info->tm_year)%100; //6 bytes (5+ 2byte year) YY/YY/MM/DD/HH/MM
    ReadHeader.StartTime[2] = (info->tm_mon)+1;
    ReadHeader.StartTime[3] = (info->tm_mday); //mday is day of the month (ie Dec 31st)
    ReadHeader.StartTime[4] = (info->tm_hour); //24 hour format
    ReadHeader.StartTime[5] = (info->tm_min);
    lread_qspi = false;
}

#ifdef DEBUG_QSPI
// ***********************************************************************************
// used to read out a page (see page struct) from any specified address
// be cautious, passing an address that is not the start of a page will result in overlap
void read_qspi_page(uint32_t Address){
          
    ret_code_t err_code;
    memset(&ReadPage, 0, sizeof(ReadPage)); //Clear ReadPage first

    err_code = nrf_drv_qspi_read(&ReadPage, sizeof(ReadPage), Address);

    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    if(err_code == NRF_SUCCESS) 
    {
      printf("QSPI page read successfully...\n");
      printf("Page at &%u :\n",Address);
      printf("  countMin: %u\n", ReadPage.countMin);
      printf("  sensorCh: %u\n", (ReadPage.sensorValue & 0x000F) );
      printf("  sensorValue: %u\n", (ReadPage.sensorValue & 0xFFF0) );
      printf("*******************\n\n");
      
    }

    lread_qspi = false;
} 
#endif


// ***********************************************************************************
// Used to read an entire sector (4kB) of the QSPI using addresses in the sector table.
// This will populate the struct ReadSector (which is less than 4kB) with all Pages...
// ...read in the specified sector
// You can select sector 0-15 
// *** This fxn should be used for populating the advertisement ***
void read_qspi_sector(uint8_t Sector){
    
     ret_code_t err_code;
     memset(&ReadSector, 0, sizeof(ReadSector)); //Clear the Read Sector first

     err_code = nrf_drv_qspi_read(&ReadSector, sizeof(ReadSector), SectorB1[Sector]);

     APP_ERROR_CHECK(err_code);
     WAIT_FOR_PERIPH();
     lread_qspi = false;
}


// ***********************************************************************************
// This will automatically populate the extern gPacket (general packet) with the current
// header (from FDS) and the specified sector of data from QSPI. Good luck?
void qspi_prepare_packet(uint8_t Sector)
{
      uint16_t data_length_calc = 0;
      #ifdef DEMO_SENSORS
      data_length_calc += (ticksTUpload/pulseInterval);
      data_length_calc += (ticksTUpload/analogInterval);
      data_length_calc += (ticksTUpload/analogInterval);
      #else
      if(bsi_config.sensor1_config.sensorEnabled == true)
      {data_length_calc += (ticksTUpload/pulseInterval);}
      if(bsi_config.sensor2_config.sensorEnabled == true)
      {data_length_calc += (ticksTUpload/bsi_config.sensor2_config.measInterval);}
      if(bsi_config.sensor3_config.sensorEnabled == true)
      {data_length_calc += (ticksTUpload/bsi_config.sensor3_config.measInterval);}
      #endif
      gPacket.datalength = data_length_calc * 4;
      
      qspi_update_time(); // Update the current START TIME in the header to current time
      read_qspi_header();
      gPacket.Header = ReadHeader;
      read_qspi_sector(Sector);
      gPacket.Sector = ReadSector;
      gPacket.datalength += sizeof(gPacket.Header);

      // Because we sent data (hopefully), we should start writing at the next sector
      // Determine if we should move to the next sector or roll back to start and erase
//      if(bsi_config.qspi_currentSector == 15) //Sector 15 is last in block 1
//      {
//         bsi_config.qspi_currentSector = 0; // Set sector back to 0
//         bsi_config.lastKnownAddr = 0;  // set next write address to start of sector 0
//      }
//      else
//      {
//         bsi_config.qspi_currentSector+=1; //increment to next sector 
//         bsi_config.lastKnownAddr = SectorB1[bsi_config.qspi_currentSector]; //set next write address to start of next sector
//      }
      //qspi_increment_sector(); // if we don't send the data, we just write to the same sector forever... call this from main when we hit our upload thresh-hold
      bsi_config.configChanged = true; //update the fds
      erase_qspi_sector(bsi_config.qspi_currentSector); //erase this sector so we can reuse it
      
}

void qspi_increment_sector(void)
{
      // Because we sent data (hopefully), we should start writing at the next sector
      // Determine if we should move to the next sector or roll back to start and erase
      if(bsi_config.qspi_currentSector == 15) //Sector 15 is last in block 1
      {
         bsi_config.qspi_currentSector = 0; // Set sector back to 0
         bsi_config.lastKnownAddr = 0;  // set next write address to start of sector 0
      }
      else
      {
         bsi_config.qspi_currentSector+=1; //increment to next sector 
         bsi_config.lastKnownAddr = SectorB1[bsi_config.qspi_currentSector]; //set next write address to start of next sector
      }
      bsi_config.configChanged = true; //update the fds
}

/************************************************************************************ 
** Update the stored "StartTime" in bsi_conifg FDS (which is in minutes)
** Called when generating a packet (meaning new start time for next packet)
** Or called when connecting to Android App and characteristics are updated 
*/
void qspi_update_time()
{
      RawTime = ((bsi_config.UTC_Minutes + ticksTUpload)*60);// Raw time has to be in seconds
      bsi_config.UTC_Minutes = (RawTime/60); //When we are done fiddling with this we gotta put it back in minutes
      ticksTUpload = 0;
      info = gmtime(&RawTime);
      #ifdef DEMO_WRITE
      printf("GMT Time : %2d:%02d\n", (info->tm_hour)%24, info->tm_min);
      #endif
}