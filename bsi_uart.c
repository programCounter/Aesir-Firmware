#include <stdint.h>
#include <string.h>
//#include "nordic_common.h"
//#include "nrf.h"
//#include "ble_hci.h"
//#include "ble_advdata.h"
//#include "ble_advertising.h"
//#include "ble_conn_params.h"
//#include "nrf_sdh.h"
//#include "nrf_sdh_soc.h"
//#include "nrf_sdh_ble.h"
//#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
//#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
//#include "app_util_platform.h"
#include "bsp_btn_ble.h"
//#include "nrf_pwr_mgmt.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "bsi_uart.h"

#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_QWR_DEF(m_qwr);  
//static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static uint16_t   m_ble_nus_max_data_len = NRF_SDH_BLE_GATT_MAX_MTU_SIZE - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
bool comm_started = false;
bool tx_rdy = false;
/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
uint8_t commstartedcount = 0;
uint8_t txrdycount  = 0;
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;

        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

        for (uint32_t i = 0; i < p_evt->params.rx_data.length; i++)
        {
            do
            {
                err_code = app_uart_put(p_evt->params.rx_data.p_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        if (p_evt->params.rx_data.p_data[p_evt->params.rx_data.length - 1] == '\r')
        {
            while (app_uart_put('\n') == NRF_ERROR_BUSY);
        }
    }

    //Catch the events from the Loli. these events let us know if it is ok to send data or not.
    switch(p_evt->type)
    {
      case BLE_NUS_EVT_TX_RDY: //the previous data is finished being sent and it's ok to send more.
        tx_rdy = true;
        #ifdef DEMO_WRITE
        printf("txrdy: %d", txrdycount++);
        #endif
        break;
      case BLE_NUS_EVT_COMM_STARTED: //if we get this flag it means that notifications have been enabled on the Loli side. We can begin sending data.
	      #ifdef DEMO_WRITE
	      printf("commstarted: %d", commstartedcount++);
	      #endif
	      comm_started = true;
	      break;/**< Notification has been enabled. */
      case BLE_NUS_EVT_COMM_STOPPED:
	      comm_started = false;
	      #ifdef DEMO_WRITE
	      printf("commstopped");
	      #endif
        break;
    }
}
/**@snippet [Handling the data received over BLE] */



/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

//This function initialises the bluetooth service used by the Nordic Uart Service. this is how we are going to pass the data. 
void nServices_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


//This funtion passes the data in 243 byte chunks to the NUS. it has to wait for an event(tx_rdy) before it can send the next piece. 
//Not waiting for the event causes significant delays in data transmissions
uint8_t chunk_array[BLE_NUS_MAX_DATA_LEN];
void uart_data_send(uint8_t * p_data, uint16_t dLen, uint16_t m_conn_handle)
{
    static uint8_t index = 0;
    uint32_t       err_code;
    uint32_t       firstAddr;
    uint32_t       lastAddr;
    uint16_t       dateLen = BLE_NUS_MAX_DATA_LEN;
    
    uint16_t datasent = 0;
    //uint16_t someLen = BLE_NUS_MAX_DATA_LEN;
    //need to set tx_rdy true for the first iteration
    tx_rdy = true;

    while(datasent < (dLen - 1) && comm_started)
    {
        if(tx_rdy)
        { 
          if((dLen - datasent) < BLE_NUS_MAX_DATA_LEN)
          {
              dateLen = (dLen - datasent);
          }
          else
          {
              dateLen = BLE_NUS_MAX_DATA_LEN;
          }
          memcpy(chunk_array,&p_data[datasent],dateLen);
          do
          {
              //
              err_code = ble_nus_data_send(&m_nus, chunk_array, &dateLen, m_conn_handle);
              if ((err_code != NRF_ERROR_INVALID_STATE) &&
                  (err_code != NRF_ERROR_RESOURCES) &&
                  (err_code != NRF_ERROR_NOT_FOUND) &&
                  (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
              {
                  APP_ERROR_CHECK(err_code);
              }
          } while (err_code == NRF_ERROR_RESOURCES);

          datasent += (dateLen - 1);
          tx_rdy = false;
        }
     }
}   


// this code is not used, it is for the physical uart and handles data from a terminal input. should probably be cut out..
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\n') ||
                (data_array[index - 1] == '\r') ||
                (index >= m_ble_nus_max_data_len))
            {
                if (index > 1)
                {
                    NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                    NRF_LOG_HEXDUMP_DEBUG(data_array, index);

                    do
                    {
                        uint16_t length = (uint16_t)index;
                        //err_code = ble_nus_data_send(&m_nus, data_array, &length, m_conn_handle);
                        if ((err_code != NRF_ERROR_INVALID_STATE) &&
                            (err_code != NRF_ERROR_RESOURCES) &&
                            (err_code != NRF_ERROR_NOT_FOUND))
                        {
                            APP_ERROR_CHECK(err_code);
                        }
                    } while (err_code == NRF_ERROR_RESOURCES);
                }

                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
#else
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */