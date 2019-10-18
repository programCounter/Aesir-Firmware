/* 

This code is based on the following nordic tutorial
"https://github.com/bjornspock/custom_ble_service_example"

*/

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

//0e28c719-6801-4160-a7d6-a3b252dc43a1 - needs to be stored as small endian...so turn it backwards.
#define CUSTOM_SERVICE_UUID_BASE         {0xA1, 0x43, 0xDC, 0x52, 0xB2, 0xA3, 0xD6, 0xA7, \
                                          0x60, 0x41, 0x01, 0x68, 0x19, 0xC7, 0x28, 0x0E}

#define CONFIG_SERVICE_UUID               0x1400
#define CUSTOM_CHAR_UUID_MEAS_INTV        0x1401 // The time in minutes between each measurment, for each Analog sensor, 2 uint 8. 
#define CUSTOM_CHAR_UUID_DT_ALRM          0x1402 // The Delta T for pulses that sets an alarm state. IF time between pulses is below x time, set alarm.
#define CUSTOM_CHAR_UUID_DM_ALRM          0x1403 // The Delta for measurements that sets an alarm state. IF measurments increase by x since last measure, set alarm.
#define CUSTOM_CHAR_UUID_UPLD_SZE         0x1404 // The size what size of data are we going to upload. in kb, 5 = 5kbyte
#define CUSTOM_CHAR_UUID_SENS_CNFG        0x1405 // The sensors attached to the device. PULSE,ANLG,ANLG. 1 = connected, 0 = disconnected. ie 110.
#define CUSTOM_CHAR_UUID_SENS_ADDRS       0x1406 // The Addresses of the attached sensors (uint-16, uint-16, uint-16). 
/**@brief   Macro for defining a ble_cus instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CUS_DEF(_name)                                                                          \
static ble_cus_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_HRS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus_on_ble_evt, &_name)                  


/**@brief Custom Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    uint8_t                       initial_custom_value;           /**< Initial custom value */
    ble_srv_cccd_security_mode_t  custom_value_char_attr_md;     /**< Initial security level for Custom characteristics attribute */
} ble_cus_init_t;

// Forward declaration of the ble_cus_t type.
typedef struct ble_cus_s ble_cus_t;

/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, ble_uuid_t * p_cus_uuid, char * uuid_Desc);


/**@brief Custom Service structure. This contains various status information for the service. */
struct ble_cus_s
{
    uint16_t                      service_handle;                 /**< Handle of Custom Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      custom_value_handles;           /**< Handles related to the Custom Value characteristic. */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type; 
};

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Custom Service structure.
 */
void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);

uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init);

static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);

static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);

static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);