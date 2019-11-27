#define SECTOR_SIZE 4096 //4kB
#include "nrf_drv_qspi.h"
#include <time.h>


// *** DEPRECATED VARIABLES THAT MAYBE WE SHOULD COMMENT OUT ***
static volatile uint8_t m_buffer_tx[16];
static volatile uint8_t m_buffer_rx[16];
extern uint16_t pressCount;
//static uint8_t currentSector; //now this is in the FDS
// *************************************************************
extern bool m_finished; // used in the QSPI event
extern bool lwrite_qspi;
extern bool lread_qspi;
extern bool lerase_sector;
//volatile time_t RawTime;

// "In the C programming language, static is used with global variables and functions to set their scope to the containing file."
//static volatile uint32_t LastKnownAddr; // Attempt to track the most recent written memory block

typedef struct BSI_Header {
      char BSI_Name[16];
      uint8_t StartTime[6]; //6 bytes (4 + 2byte year) YYYY/MM/DD/HH/MM
}BSI_Header;

extern BSI_Header Header;

typedef struct QSPI_Page_ 
{ // each QSPI_Page is 4 bytes (but a true page, for our flash chip, is 1 byte)
    uint16_t countMin; // minutes since Header StartTime[] (last Local Listener connection)
    //uint16_t sensorCh; // which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    uint16_t sensorValue; // the 16b reading from the sensor (10-bit ADC for An# or Bool for Pulse)    
}QSPI_Page;

extern QSPI_Page CurrentPage;

typedef struct QSPI_Sector{ // if a QSPI_page is 4 bytes and a sector is 1024 Bytes...
    struct QSPI_Page_ Page[(SECTOR_SIZE/(sizeof(QSPI_Page)))];    // ... then there should be 256 QSPI pages in this array
    // sector related features go here?
}QSPI_Sector;

extern QSPI_Sector ReadSector;

typedef struct Advertisement_General_Packet {
    char imSendingYouData[3];
    uint16_t datalength;
    struct BSI_Header Header;
    struct QSPI_Sector Sector;
}Ad_gPacket;

extern Ad_gPacket gPacket; //General packet used for sending header + a sector of data 
extern uint32_t ticksTUpload; // nUmber of ticks to be compared against upload interval

//************************************************************
//        F U N C T I O N      P R O T O T Y P E S 
//************************************************************
void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);
void configure_memory(void);

void write_qspi(uint32_t Address);
void write_qspi_page();
void write_qspi_header();

void erase_qspi_sector(uint8_t Sector);

void read_qspi(uint32_t Address); 
void read_qspi_header();
void read_qspi_page(uint32_t Address);
void read_qspi_sector(uint8_t Sector);

void qspi_prepare_packet(uint8_t Sector);
void qspi_update_time();