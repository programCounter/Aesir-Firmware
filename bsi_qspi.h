extern bool m_finished; // used in the QSPI event
static volatile uint8_t m_buffer_tx[16];
static volatile uint8_t m_buffer_rx[16];
extern uint16_t pressCount;
extern bool lwrite_qspi;
extern bool lread_qspi;
static volatile uint32_t LastKnownAddr = 0; // Attempt to track the most recent written memory block

typedef struct BSI_Header {
      char BSI_Name[16];
      uint8_t StartTime[7]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
}BSI_Header;

typedef struct QSPI_Page {
    uint32_t countMin; //minutes since Header StartTime[] (last Local Listener connection)
    uint8_t sensorCh; //which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    uint16_t sensorValue; //the 16b reading from the sensor (10-bit ADC for An# or Bool for Pulse)
}QSPI_Page;

typedef struct Advertisement_General_Packet {
    struct BSI_Header Header;
    struct QSPI_Page qPage[];
}Ad_gPacket;

void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);

void configure_memory(void);

void write_qspi(uint32_t Address); //Address to write to

void read_qspi(uint32_t Address); //Address to read FROM