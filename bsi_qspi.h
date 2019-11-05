extern bool m_finished; // used in the QSPI event
static volatile uint8_t m_buffer_tx[16];
static volatile uint8_t m_buffer_rx[16];
extern uint16_t pressCount;
extern bool lwrite_qspi;
extern bool lread_qspi;
static volatile uint32_t LastKnownAddr = 0; // Attempt to track the most recent written memory block

typedef struct Header {
      char BSI_Name[16];
      uint8_t StartTime[7]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
}Header;

typedef struct Ad_gPacket {
    char BSI_Name[16];
    uint8_t StartTime[7]; //7 bytes (5+ 2byte year) YYYY/MM/DD/HH/MM/SS
    //....
}Ad_gPacket;

void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);

void configure_memory(void);

void write_qspi(void);

void read_qspi(void);