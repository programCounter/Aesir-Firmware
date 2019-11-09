#define SECTOR_SIZE 1024 //1kB

#include "nrf_drv_qspi.h"

extern bool m_finished; // used in the QSPI event
static volatile uint8_t m_buffer_tx[16];
static volatile uint8_t m_buffer_rx[16];
extern uint16_t pressCount;
static uint8_t currentSector;
extern bool lwrite_qspi;
extern bool lread_qspi;
extern bool lerase_sector;

// "In the C programming language, static is used with global variables and functions to set their scope to the containing file."
//static volatile uint32_t LastKnownAddr; // Attempt to track the most recent written memory block

typedef struct BSI_Header {
      char BSI_Name[16];
      uint8_t StartTime[8]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
}BSI_Header;


typedef struct QSPI_Page_ 
{ // each memory page is 1byte, but ours are the size of each sensor reading
    uint16_t countMin; // minutes since Header StartTime[] (last Local Listener connection)
    uint16_t sensorCh; // which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    uint16_t sensorValue; // the 16b reading from the sensor (10-bit ADC for An# or Bool for Pulse)    
    //uint8_t dataSpace; //maybe data needs spaces too :)
}QSPI_Page;

extern QSPI_Page CurrentPage;
extern QSPI_Page ReadPage;

typedef struct QSPI_Sector { // if a QSPI_page is 5 bytes and a sector is 1024 Bytes...
    struct QSPI_Page_ Page[(SECTOR_SIZE/(sizeof(QSPI_Page)))];    // ... then there should be 204 QSPI pages in this array
    //bool xTransmitted; // *** HMM ERASED MEMORY IS STORED AS ALL 111111 soooo
    // sector related features go here?
}QSPI_Sector;


typedef struct Advertisement_General_Packet {
    struct BSI_Header Header;
    struct QSPI_Sector Sector;
}Ad_gPacket;


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