extern bool m_finished; // used in the QSPI event
static volatile uint8_t m_buffer_tx[16];
static volatile uint8_t m_buffer_rx[16];
extern uint16_t pressCount;

static uint8_t currentSector;
extern bool lwrite_qspi;
extern bool lread_qspi;
// "In the C programming language, static is used with global variables and functions to set their scope to the containing file."
static volatile uint32_t LastKnownAddr; // Attempt to track the most recent written memory block
extern volatile uint8_t OpCode; //Used to specify type of read/write operation (9 is no operation

typedef struct BSI_Header {
      char BSI_Name[16];
      uint8_t StartTime[8]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
}BSI_Header;

typedef struct QSPI_Page { // each memory page is 1byte, but ours are the size of each sensor reading
    uint32_t countMin; // minutes since Header StartTime[] (last Local Listener connection)
    uint8_t sensorCh; // which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    uint16_t sensorValue; // the 16b reading from the sensor (10-bit ADC for An# or Bool for Pulse)    
}QSPI_Page;

typedef struct QSPI_Sector { // if a QSPI_page is 53 bytes and a sector is 4096 Bytes...
    struct QSPI_Page Page[77];    // ... then there should be 77.28 QSPI pages in this array
    bool xTransmitted; // if F, 4kB sector has never been transmitted. otherwise, T
    // sector related features go here?
}QSPI_Sector;



typedef struct Advertisement_General_Packet {
    struct BSI_Header Header;
    struct QSPI_Sector Sector;
}Ad_gPacket;

void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);

void configure_memory(void);

void write_qspi(uint8_t Opcode, uint32_t Address); //Address to write to

void read_qspi(uint8_t Opcode, uint32_t Address); //Address to read FROM