/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_bas.h"
#include "nrf_drv_qspi.h"
#include "boards.h"
#include "sdk_config.h"
#include "nrf_delay.h"
#include "app_util_platform.h"

//#include "ble_nus.h"
//#include "app_uart.h"
//#if defined (UART_PRESENT)
//#include "nrf_uart.h"
//#endif
//#if defined (UARTE_PRESENT)
//#include "nrf_uarte.h"
//#endif

#include "bsi_config.h"
#include "bsi_qspi.h"
#include "BLE_CUS.h"
#include "bsi_measure.h"
#include "bsi_uart.h"


//#define QSPI_STD_CMD_WRSR   0x01
//#define QSPI_STD_CMD_RSTEN  0x66
//#define QSPI_STD_CMD_RST    0x99

//#define QSPI_TEST_DATA_SIZE 256

#define WAIT_FOR_PERIPH() do { \
        while (!m_finished) {} \
        m_finished = false;    \
    } while (0)

/************************DEBUG DEFINITIONS**************************************************************************/
// see defines in sdk_config
/******************************************************************************************************************/


#define DEVICE_NAME                     "AEsir9"                                /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "RioT Wireless"                         /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                500                                       /**< The advertising duration (5 seconds) in units of 10 milliseconds. */
//#define APP_ADV_DURATION                18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                      /**< Maximum encryption key size. */

#define pulseInterval 60 //needs to be set up in config, need to talk to will about changing characteristics

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

//from the custom service tutorial
BLE_CUS_DEF(m_cus);

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static uint32_t ticksS2;
static uint32_t ticksS3;
static bool S2MeasureNow;
static bool S3MeasureNow;
static bool UploadNow;
static bool pushData;
static bool bleConnected;
static bool factoryReset;
static bool advertisingStarted;
static bool pendingUpload;
#ifdef DEBUG_GENERAL
//uint32_t qspiAddress = 0; //ONLY FOR DEBUG
bool StressTest_FAILED = false;
#endif

#ifdef DEBUG_GENERAL // Button-press-factory-reset
static bool factoryReset = true;
#else
static bool factoryReset = false;
#endif

uint32_t p_reset_reason; //holds a register(RESETREAS) indicating the reason the device was reset
// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpower.html
uint32_t Vbat_reset = 0b100000000000000000000; // 1 at bit 21 should be "Reset due to wake up from System OFF mode by VBUS rising into valid range"
/* YOUR_JOB: Declare all services structure your application is using
 *  BLE_XYZ_DEF(m_xyz);
 */
BLE_BAS_DEF(m_bas);

// YOUR_JOB: Use UUIDs for service(s) used in your application.
static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}//,
    //{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE} // exposes the uart service
    //,
    //{CONFIG_SERVICE_UUID, BLE_UUID_TYPE_VENDOR_BEGIN } /**< Universally unique service identifiers. Doesnt need to be here, has no effect on the service*/
    //{BLE_UUID_BATTERY_SERVICE,            BLE_UUID_TYPE_BLE},
   
};

/* When we added the custom service we had to change the ram map as shown...
    RAM_START=0x20002210  -> 0x20002220 
    RAM_SIZE=0x3ddf0      -> 0xDDE0
*/

static volatile uint8_t m_advert_data[1650]; // 256 bytes on air at one time, max 1650 byte in total chained advert.

static uint32_t NumFlashAddrs = 16777215; // the device is configured in 24bit addressing mode so 2^24 adresses are possible

#ifdef DEBUG
#define MINUTE_TIMER_TICK APP_TIMER_TICKS(1000) // 1 second, debuggin time mudda fuxa
#define ADVERT_RETRY_TIMER_TICK APP_TIMER_TICKS(5000)
#else
#define MINUTE_TIMER_TICK APP_TIMER_TICKS(60000) //1 min, lowest resolution of time we will think about.
#define ADVERT_RETRY_TIMER_TICK APP_TIMER_TICKS(5000)
#endif
//#define SENSOR_MEASURE_TICK APP_TIMER_TICKS(3600000) //1 hour. default measurement time for analog sensors

APP_TIMER_DEF(m_minute_timer_id);      
APP_TIMER_DEF(m_advert_timer_id);                            
//APP_TIMER_DEF(m_measure_timer1_id);  
//APP_TIMER_DEF(m_measure_timer2_id);  

static void advertising_start(bool erase_bonds); 


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}





/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
       err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
       APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


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


/**@brief Function for handling the YYY Service events.
 * YOUR_JOB implement a service handler function depending on the event the service you are using can generate
 *
 * @details This function will be called for all YY Service events which are passed to
 *          the application.
 *
 * @param[in]   p_yy_service   YY Service structure.
 * @param[in]   p_evt          Event received from the YY Service.
 *
 *
static void on_yys_evt(ble_yy_service_t     * p_yy_service,
                       ble_yy_service_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_YY_NAME_EVT_WRITE:
            APPL_LOG("[APPL]: charact written with value %s. ", p_evt->params.char_xx.value.p_str);
            break;
        default:
            // No implementation needed.
            break;
    }
}
 */

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_bas_init_t     bas_init;
    ble_cus_init_t     cus_init;//From the custom service tutorial
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Here the sec level for the Battery Service can be changed/increased.
    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
    //BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

    //ble_bas_init_t  bas_init;
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    //bas_init.p_report_ref = 1;
    err_code = ble_bas_init(&m_bas, &bas_init);

    /* YOUR_JOB: Add code to initialize the services used by the application.
       ble_xxs_init_t                     xxs_init;
       ble_yys_init_t                     yys_init;
       
       // Initialize XXX Service.
       memset(&xxs_init, 0, sizeof(xxs_init));
       xxs_init.evt_handler                = NULL;
       xxs_init.is_xxx_notify_supported    = true;
       xxs_init.ble_xx_initial_value.level = 100;
       err_code = ble_bas_init(&m_xxs, &xxs_init);
       APP_ERROR_CHECK(err_code);
       // Initialize YYY Service.
       memset(&yys_init, 0, sizeof(yys_init));
       yys_init.evt_handler                  = on_yys_evt;
       yys_init.ble_yy_initial_value.counter = 0;
       err_code = ble_yy_service_init(&yys_init, &yy_init);
       APP_ERROR_CHECK(err_code);
     */

    //Code below from custom service tutorial
    // Initialize CUS Service init structure to zero.
    memset(&cus_init, 0, sizeof(cus_init));
      
    err_code = ble_cus_init(&m_cus, &cus_init);
    APP_ERROR_CHECK(err_code);	

    //Sets the read and write permisions for the custom service.
    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init.custom_value_char_attr_md.read_perm);
    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init.custom_value_char_attr_md.write_perm);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

static void Advet_timeout_handler(void * p_context)
{
   advertisingStarted = false; //we make this true and the pending upload flag is true we should restart our timer
}

static void minute_timer_timeout_handler(void * p_context)
{
  //What happens every minute?
  //Wake up and increment the time variable
  //If the variable 
  UNUSED_PARAMETER(p_context);
  
  //pushData = true;
  
  ticksTUpload++;//Increment our minutes since last upload.
  //bsi_config.configChanged = true;
  //  #ifdef DEBUG
  //  if(ticksTUpload >= 60) //every minute
  //  #else
  //  if(ticksTUpload >= bsi_config.uploadInterval)
  //  #endif
  //  {
  //    // It is now time to initiate our upload
  //    UploadNow = true;
  //    ticksTUpload=0;

  //  }

  #ifdef DEBUG
  if(true)
  #else
  if(bsi_config.sensor1_config.sensorEnabled == true) // IF the sensors not enabled dont even increment the count
  #endif
  {
    ticksPulse++;
    if(ticksPulse >= pulseInterval) //pulse interval is defines in sdk config
    {
      pulseWriteNow = true;
      ticksPulse = 0;
    }
    if(pulseAlarmOn)
    {
    pulse_alarm_check();
    }
  }


  #ifdef DEBUG
  if(true) //for debugging only, will ensure sensor 1 data is received
  #else
  if(bsi_config.sensor2_config.sensorEnabled == true) // IF the sensors not enabled dont even increment the count
  #endif
  {
    ticksS2++;
    #ifdef DEBUG
    if(ticksS2 >= analogInterval) //analogInterval defined in sdk config
    #else
    if(ticksS2 >= bsi_config.sensor2_config.measInterval)
    #endif
    {
      S2MeasureNow = true;
      ticksS2 = 0;
    }
  }

  #ifdef DEBUG
  if(true) //for debugging only, will ensure sensor 1 data is received
  #else
  if(bsi_config.sensor3_config.sensorEnabled == true) // IF the sensors not enabled dont even increment the count
  #endif
  {
    ticksS3++;
    #ifdef DEBUG
    if(ticksS3 >= analogInterval) //analogInterval defined in sdk config
    #else
    if(ticksS3 >= bsi_config.sensor2_config.measInterval)
    #endif
    {
      S3MeasureNow = true;
      ticksS3 = 0;
    }
  }
}



/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
       ret_code_t err_code;
       err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
       APP_ERROR_CHECK(err_code); */

    // Create timers.
    err_code = app_timer_create(&m_minute_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                minute_timer_timeout_handler);
                              
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_advert_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                Advet_timeout_handler);
                              
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
       ret_code_t err_code;
       err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
       APP_ERROR_CHECK(err_code); */

    ret_code_t err_code;//Start the 1 minute timer, this is the timer used to trigger the updload
    err_code = app_timer_start(m_minute_timer_id, MINUTE_TIMER_TICK, NULL);
    APP_ERROR_CHECK(err_code);

}

static void advert_restart_timer_start(void)
{
    ret_code_t err_code;//Start the 10 second wait before we retry advertising for connection.
    err_code = app_timer_start(m_advert_timer_id, ADVERT_RETRY_TIMER_TICK, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

//    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
//    APP_ERROR_CHECK(err_code);
//
//    // Prepare wakeup buttons.
//    err_code = bsp_btn_ble_sleep_mode_prepare();
//    APP_ERROR_CHECK(err_code);
//
//    // Go to system-off mode (this function will not return; wakeup will cause a reset).
//    err_code = sd_power_system_off();
//    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cus_t * p_cus = (ble_cus_t *) p_context;
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE: //from BLE_CUS.c
            if (p_cus == NULL || p_ble_evt == NULL)
            {
                return; //Returns if the evet has no evt or context. Null
            }
           on_write(p_cus, p_ble_evt);
           break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected.");
            // LED indication will be changed when advertising starts.

            on_disconnect(p_cus, p_ble_evt); //from BLE_CUS.c
            bleConnected = false;
            break;

        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);

            on_connect(p_cus, p_ble_evt);//from BLE_CUS.c
            bleConnected = true;
            advertisingStarted = false;
            break;
        case BLE_GAP_EVT_TIMEOUT:
            if(pendingUpload==true)
            {
              advert_restart_timer_start();//If we time out but still have a pending upload, wait a few seconds and try re-advertising for a connection.
            }
             
            break;
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, &m_ble_observer);
    //NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
//                     BLE_HRS_BLE_OBSERVER_PRIO,                                                     \
//                     ble_cus_on_ble_evt, &_name)
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in] event Event generated when button is pressed.
 */
static void bsp_event_handler(bsp_event_t event)
{

    ret_code_t err_code;
    uint8_t d;
    uint8_t count;
    uint8_t digit_read;
    m_finished = false;
    switch (event)
    {
        case BSP_EVENT_KEY_0:
            //doesn't work
            break;

        case BSP_EVENT_KEY_1:
            //Increment the button counter, only 4 LEDs so rolls over after 15.
            //pressCount ++;
            //if(pressCount > 15)
            //{
            //  pressCount = 0;
            //}
            //ble_bas_battery_level_update(&m_bas,pressCount,BLE_CONN_HANDLE_ALL);
            //LB: Just me debugging again :)
            //PLS ERASE SECTOR 1 and return LastKnownAddress to 4096 (start of Sector 1)
            lerase_sector = true;
            break;
 
        case BSP_EVENT_KEY_2:
            #ifdef DEBUG_QSPI
            lwrite_qspi = true;
            #endif
            break;
       
        case BSP_EVENT_KEY_3:
            #ifdef DEBUG_QSPI
            lread_qspi = true;
            #endif
            break;
        
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break; // BSP_EVENT_SLEEP

        case BSP_EVENT_RESET:
            NVIC_SystemReset();
            break; // Will completely reset system :O RIP

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break; // BSP_EVENT_KEY_0

        default:
            break;
    }
}


/**@brief Function for initializing the Advertising functionality.*/
static void advertising_init(bool codedPHY)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type                = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance       = true;
    init.advdata.flags                    = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt  = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids   = m_adv_uuids;

    init.config.ble_adv_fast_enabled      = true;
    init.config.ble_adv_fast_interval     = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout      = APP_ADV_DURATION;
    init.config.ble_adv_extended_enabled  = false;
    if(codedPHY==true)
    {
      init.config.ble_adv_primary_phy = BLE_GAP_PHY_CODED;
      init.config.ble_adv_secondary_phy = BLE_GAP_PHY_CODED;
    }
    else
    {
      init.config.ble_adv_primary_phy = BLE_GAP_PHY_1MBPS;
      init.config.ble_adv_secondary_phy = BLE_GAP_PHY_1MBPS;
    }

    //init.config.
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.*/
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.*/
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


/**@brief Function for starting advertising.*/
static void advertising_start(bool erase_bonds)
{
    
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event
    }
    else
    {

        
        ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

     }
     advertisingStarted = true;
}

///**@brief Struct that contains pointers to the encoded advertising data. */
//static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
//static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */
//static ble_gap_adv_data_t m_adv_data =
//{
//    .adv_data =
//    {
//        .p_data = m_enc_advdata,
//        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX 
//    },
//    .scan_rsp_data =
//    {
//        .p_data = NULL,
//        .len    = 0
//
//    }
//};

//void update_advert(void) //depricated, now using UART
//{
//    
//    ret_code_t err_code;
//    ble_advdata_t advdata;
//    uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
//    uint8_t junkvar = 9;
//    // Build and set advertising data.
//    memset(&advdata, 0, sizeof(advdata));
//
//    advdata.name_type             = BLE_ADVDATA_NO_NAME;
//    advdata.flags                 = flags;
//    advdata.p_manuf_specific_data = junkvar;
//
//    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
//    APP_ERROR_CHECK(err_code);
//
//    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, NULL);
//    APP_ERROR_CHECK(err_code);
//}

//function for updating characteristic value
void sensor_value_characteristic_update(ble_cus_t * p_cus, int16_t data)
{
    //little endian converter::::::: -> swapped = (num>>8) | (num<<8);
    ble_gatts_value_t  gatts_value;
    //data = (data>>8) | (data<<8);
        // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint16_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &data;

        // Update database.
    //sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
    //                                      p_cus->custom_value_handles.value_handle,
    //                                      &gatts_value);
}

/**@brief Function for application main entry.*/
int main(void)
{
    ret_code_t retCode;
    uint32_t responce;
    bool erase_bonds;
    nrf_drv_qspi_config_t config = NRF_DRV_QSPI_DEFAULT_CONFIG;
    advertisingStarted = false;

    // Initialize.
    log_init();
    timers_init();
    saadc_init();
    gpio_init();
    buttons_leds_init(&erase_bonds);
    power_management_init();
    ble_stack_init();
    retCode = init_fds();
    APP_ERROR_CHECK(retCode);
    gap_params_init();
    gatt_init();
    services_init(); //Initialises the basic services and calls ble_cus_init which inits the custom service and charateristics for configuring the BSI
    nServices_init(); //Initialises the NUS(for UART) and the qued write module

    // uint32_t sd_power_reset_reason_get(uint32_t *p_reset_reason)
    //responce = sd_power_reset_reason_get(&p_reset_reason);

    advertising_init(false);
    
    conn_params_init();
    peer_manager_init();

    nrf_drv_qspi_init(&config, qspi_handler, NULL);

    configure_memory();
    m_finished = false;


    

//    nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, 0);
//    WAIT_FOR_PERIPH();
    
//    retCode = init_fds();
//    APP_ERROR_CHECK(retCode);
//    retCode = read_fds(fds_BSI_File,fds_BSI_Key, &bsi_config);

    //APP_ERROR_CHECK(retCode);

    // Start execution.
    //NRF_LOG_INFO("Template example started.");
    application_timers_start();
    uart_init();

    //LB: Moving towards code that can be reset with a button
    if(factoryReset) 
    {
      erase_qspi_sector(1);
      //write_qspi_header();
      //read_qspi_header();
      qspi_update_time();
      bsi_config.configChanged = true; //set flash bsi_config to defaults
      factoryReset = false;
    }
    else
    {
      //If we have a configuration already we should load it into the characteristics.
      retCode = read_fds(fds_BSI_File,fds_BSI_Key, &bsi_config);
    } //

    advertising_start(erase_bonds);
    
    //somejunkvar = true;
    //ble_bas_battery_level_update(&m_bas, 5, BLE_CONN_HANDLE_ALL);
    // Enter main loop.
    for (;;)
    {
        if(bsi_config.configChanged == true)
        {
          retCode = write_fds(fds_BSI_File,fds_BSI_Key);
          APP_ERROR_CHECK(retCode);

          bsi_config.configChanged = false; // Written the config, set this back to false...
        }
        if(pulseWriteNow == true)
        {
            measureSensor(2);
            lwrite_qspi = true;
          pulseWriteNow = false;
        }
        #ifdef DEBUG
        if(S2MeasureNow == true)
        #else
        if(S2MeasureNow == true && bsi_config.sensor2_config.sensorEnabled == true)
        #endif
        {
          #ifdef DEBUG
            measureSensor(0);
            lwrite_qspi = true;
          #else
          measureSensor(0);
          lwrite_qspi = true;
          #endif
          
          //Time to take a measurement on Analog S2
          S2MeasureNow = false;
        }
        #ifdef DEBUG
        else if(S3MeasureNow == true)
        #else
        else if(S3MeasureNow == true && bsi_config.sensor3_config.sensorEnabled == true)
        #endif
        {
          #ifdef DEBUG
            measureSensor(1);
            lwrite_qspi = true;
          #else
          measureSensor(1);
          lwrite_qspi = true;
          #endif
          // Time to take a measurement on Analog S3
          S3MeasureNow = false;
        }
//        else if(UploadNow == true)
//        {
//          //There may be an issue with how the advert api plays with the code the update advert,
//          //based on what I read we should dodge the issues by only changing the config when the advertising is stopped.
//          //we need the data from the flash
//          
//          // read_qspi(qspiAddress); // *** TO BE DISCUSSED ***
//          //read_qspi_page(4096 + sizeof(CurrentPage));
//          read_qspi_sector(1);
//          //we need to put that data into our advert
//          //update_advert();
//          //Advertising should be stopped
//          //advertising_start(erase_bonds);
//          //The advertisment should then time out and stop.
//          UploadNow = false;
//        }

        if(lwrite_qspi == true) // BUTTON 0 is PRESSED
        {
          
          #ifdef DEBUG_QSPI
          for(int i = 0; i < (1024*14); i++) // 1024 pages fit in 4kB (if 4 bytes per page)
          {                                  // *14 intentionally not filling last (15th) sector to prevent erasing in rewrite prep           // measureSensor(0);  //debug  
              CurrentPage.sensorValue = rand();
              write_qspi_page();   //debug
              CurrentPage.countMin ++;
          }
          uint16_t StressCountMin = 0; 
          bsi_config.qspi_currentSector = 1;
          for(uint16_t i = 1; i < 15; i++)
          {
              read_qspi_sector(i);
              for(uint16_t j = 0; j < 1024; j++)
              {
                  StressCountMin++;
                  if(ReadSector.Page[j].countMin != (j+((i-1)*1024))){
                      StressTest_FAILED = true;
                      break;
                  }
              }
          }
          //end of test cleanup
          bsi_config.lastKnownAddr = 4096;
          bsi_config.qspi_currentSector = 1;

          #else
          write_qspi_page();
          #endif

          // write_qspi_header();
          // write_qspi(qspiAddress); // *** TO BE DISCUSSED ***
          #ifdef DEBUG
            //lread_qspi = true;  //debug
          #else
          #endif
        }//END OF if(lwrite_qspi == true)


        if(lread_qspi == true) // BUTTON 3 is PRESSED
        {
          #ifdef DEBUG_QSPI
            read_qspi_sector(bsi_config.qspi_currentSector-1);
          #else
          #endif
            
          //read_qspi_page(4096);
          //read_qspi_header();
        }

        if(lerase_sector == true)
        {
          //nrf_drv_qspi_chip_erase();
          //erase_qspi_sector(1);            
        }
        
        //read_qspi_header();

        //if(bsi_config.lastKnownAddr >= (bsi_config.uploadSize+(4096*bsi_config.qspi_currentSector)))
        if(bsi_config.lastKnownAddr >= (75+(4096*bsi_config.qspi_currentSector)))
        {
          pendingUpload = true;
        }
        
         if(pendingUpload == true)
        {
          if(advertisingStarted==false)
          {
            //Start a CODED PHY Advertisement. We are going to need two types of advertising. Coded and un-coded.
            advertising_init(true);
            advertising_start(erase_bonds);
          }
          
          if(bleConnected == true) //If we dont have a connection, dont send data.
          {

            qspi_prepare_packet(bsi_config.qspi_currentSector);

            //gPacket.Header
            //gPacket.Sector
            uint32_t sOf = sizeof(gPacket);
  //          uint32_t sOf = sizeof(uint32_t);
  //          uart_data_send(&sOf,sOf,m_conn_handle);
            uart_data_send(&gPacket,sOf,m_conn_handle);
            //pushData = false;
           // erase_qspi_sector(bsi_config.qspi_currentSector);
            //bsi_config.lastKnownAddr =0;

            //Data is all done, break the connection...// Connection break is done by the Loli
            pendingUpload = false;

          }
        }
        idle_state_handle();
    }
}
