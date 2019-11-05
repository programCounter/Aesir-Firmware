#include "nrf_drv_qspi.h"
#include "bsi_qspi.h"
#include "bsp_btn_ble.h"

#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99

#define WAIT_FOR_PERIPH() do { \
        while (!m_finished) {} \
        m_finished = false;    \
    } while (0)

uint16_t pressCount = 0;
bool lwrite_qspi = false;
bool lread_qspi = false;
bool m_finished = false; // used in the QSPI event

BSI_Header TestHeader = {
    .BSI_Name = {'T','E','S','T','\0'},
    .StartTime = {20,19,11,4,19,9,0}, //   YY/YY/MM/DD/HH/MM/SS;
    };

// Empty header to read TestHeader back out of memory
BSI_Header ReadHeader = {
    };

QSPI_Page PageTest = {
    .countMin = 1440, //minutes since Header StartTime[] (last Local Listener connection)
    .sensorCh = 1, //which sensor the following value is from (1=An1, 2=An2, or 9 = Pulse)
    .sensorValue = 1023, //What reading did sensor take? (10b ADC or pulse)
    };

Ad_gPacket TestPacket = {
//    .Header = TestHeader,
//    .qPage = PageTest,
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

//Code for reading and writing from QSPI, flag set in button event, method called in main
void write_qspi(uint32_t Address)
{
    ret_code_t err_code;
    //volatile uint16_t nCount = pressCount;
    //volatile uint16_t c = 0;
    // counter for binary array 

    //bsp_board_leds_off();
    //bsp_board_led_on(0);
    //m_buffer_tx[0]=0;
    //m_buffer_tx[1]=0;
    //m_buffer_tx[2]=0;
    //m_buffer_tx[3]=0;
    
    //memset(&m_buffer_tx, 0, sizeof(m_buffer_tx)); //try this to nuke the array.
    
    /*while (nCount > 0) { 
        // storing remainder in binary array  
        m_buffer_tx[c] = nCount & 00000001; 
        nCount = nCount>>1;
        c++; 
     }*/

     //err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, 0);
     //APP_ERROR_CHECK(err_code);
     //nrf_drv_qspi_chip_erase();
     //WAIT_FOR_PERIPH();

     err_code = nrf_drv_qspi_write(&TestHeader, sizeof(TestHeader), LastKnownAddr);
     APP_ERROR_CHECK(err_code);
     WAIT_FOR_PERIPH();

    if(err_code == NRF_SUCCESS) { //If write was success, then increment address (+1 so we start at a new place, no overlap)
      LastKnownAddr += (sizeof(TestHeader));
    }
 
     //bsp_board_leds_on();
     //pressCount = 0;
     lwrite_qspi = false;
}

void read_qspi(uint32_t Address)
{
      //bsp_board_leds_off();
      ret_code_t err_code;
      //volatile uint8_t c = 0;
      //volatile uint8_t digit_2read = 0;
      memset(&ReadHeader, 0, sizeof(ReadHeader)); //try this to nuke the array.

      err_code = nrf_drv_qspi_read(&ReadHeader, sizeof(ReadHeader), Address);
      APP_ERROR_CHECK(err_code);
      WAIT_FOR_PERIPH();

      /*for (c = 0 ; c<4 ; ++c)
       {
          digit_2read = m_buffer_rx[3-c];
          if(digit_2read == 1)
          {
             bsp_board_led_on(c);
          }
       }*/
        
       lread_qspi = false;
}