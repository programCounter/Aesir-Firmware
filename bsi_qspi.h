extern bool m_finished; // used in the QSPI event
static volatile uint8_t m_buffer_tx[72];
static volatile uint8_t m_buffer_rx[72];
extern uint16_t pressCount;
extern bool lwrite_qspi;
extern bool lread_qspi;
static volatile uint32_t LastKnownAddr = 0; // Attempt to track the most recent written memory block



void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);

void configure_memory(void);

void write_qspi(void);

void read_qspi(void);