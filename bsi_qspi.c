#include "nrf_drv_qspi.h"
#include "bsi_qspi.h"
#include "bsp_btn_ble.h"
#include <string.h>

#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99

#define WAIT_FOR_PERIPH() do { \
        while (!m_finished) {} \
        m_finished = false;    \
    } while (0)

// a "memory page" is 1Byte (each address represents this minimum value)
// however, our "QSPI PAGE" is the size of each sensor reading data set (much larger than 1 byte)
// a memory sector is 4kB
// a memory block is 64kB (thus there are 16 sectors per block!)

//Used to find start address for sector block 1
const uint32_t SectorB1[] = {0,4096,8192,12288,16384,20480,24576,28672,32768,36864,40960,45056,49152,53248,57344,61440,65536};
static uint8_t currentSector = 1; //sector 1 starts at address 4096 
//uint16_t pressCount = 0;
bool lwrite_qspi = false;
bool lread_qspi = false;
bool m_finished = false; // used in the QSPI event
volatile uint8_t OpCode = 9; // Used to specify type of read/write operation (9 is no operation)
static volatile uint32_t LastKnownAddr = 4096; //start at beginning of 1st sector (do not use sector 0)

BSI_Header Header = { // Entire first sector is reserved for header (prevent erasing)
    .BSI_Name = {'T','E','S','T','\0'},
    .StartTime = {20,19,11,4,19,9,0}, //   YY/YY/MM/DD/HH/MM/SS;
    };

// Empty header to read TestHeader back out of memory
BSI_Header ReadHeader = {
    };

QSPI_Page CurrentPage = {
    .countMin = 0, //minutes since Header StartTime[] (last Local Listener connection)
    .sensorCh = 0, //which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    .sensorValue = 0, //What reading did sensor take? (10b ADC or pulse)
    };

QSPI_Page ReadPage = { // Only used for checking an single, specific sensor reading
    };

QSPI_Sector ReadSector = {
    .Page = {0},  // set every page to all zeros (clear array)
    .xTransmitted = false,
    };




Ad_gPacket Packet = {
    };

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
}


// All Code seen below is for reading and writing from QSPI,
// flag was? set in button event, method called in main

// Operation 0 is replace header,
// Operation 1 is write to address,
// Operation 2 is erase a 4kB sector at address
void write_qspi(uint8_t OpCode, uint32_t Address) 
{
    ret_code_t err_code;

    //bsp_board_leds_off();
    
    //memset(&m_buffer_tx, 0, sizeof(m_buffer_tx)); //try this to nuke the array.    

     //err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, 0);
     //APP_ERROR_CHECK(err_code);
     //nrf_drv_qspi_chip_erase();
     //WAIT_FOR_PERIPH();

     switch(OpCode)
     {
        case 0: // replace header
          nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, 0); //Erase 4kB from &0
          err_code = nrf_drv_qspi_write(&Header, sizeof(Header), 0);
          APP_ERROR_CHECK(err_code);
          WAIT_FOR_PERIPH();
          if(err_code == NRF_SUCCESS) { //If write was success...
            // If header update was success... do something?
          }
          break;

        case 1: // write to address
           err_code = nrf_drv_qspi_write(&CurrentPage, sizeof(CurrentPage), LastKnownAddr);
           APP_ERROR_CHECK(err_code);
           WAIT_FOR_PERIPH();

          if(err_code == NRF_SUCCESS) { //If write was success, then increment address
            LastKnownAddr += (sizeof(CurrentPage));
            // if sector pages == 77 then ...
            // is current sector == 16? if no then increment current sector
            //                          if yes then current sector = 2 (roll over)
            //
            // and increment lastknownaddress to 4096*current sector (the new number)
            // before writing to new sector we should check if the sector was transmitted.....
            //    if it isnt Tx'd then we have a BIG problem
          }
          break;

        case 2: // erase 4kb sector from Address
          nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, Address);
          break;
     }
     
     lwrite_qspi = false; 
     //bsp_board_leds_on();
}


// eg. to read sector 1 (4096-8191) call "read_qspi(1,1)". Then ReadHeader is populated with that memory.
void read_qspi(uint8_t OpCode, uint32_t Address)
{
  
    ret_code_t err_code;
    switch(OpCode)
    {
      case 0: // read header
          memset(&ReadHeader, 0, sizeof(ReadHeader)); //Clear the read header first
          err_code = nrf_drv_qspi_read(&ReadHeader, sizeof(ReadHeader), 0);
          APP_ERROR_CHECK(err_code);
          WAIT_FOR_PERIPH();
          break;

       case 1: // Read from address
          memset(&ReadPage, 0, sizeof(ReadPage)); //Clear the Read Page first
          err_code = nrf_drv_qspi_read(&ReadHeader, sizeof(ReadHeader), Address);
          APP_ERROR_CHECK(err_code);
          WAIT_FOR_PERIPH();
          break;

      case 2: // Read 4kB Sector
         memset(&ReadSector, 0, sizeof(ReadSector)); //Clear the Read Sector first
         err_code = nrf_drv_qspi_read(&ReadSector, sizeof(ReadSector)/*<4096*/, SectorB1[Address]);
         break;
   }
        lread_qspi = false;
}

// write function to generate lots of data to populate the entire sector
// then write another function to read it out and test its accuracy (and report errors)