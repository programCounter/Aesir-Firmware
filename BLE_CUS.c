/* 

This code is based on the following nordic tutorial
"https://github.com/bjornspock/custom_ble_service_example"

*/

#include "sdk_common.h"
#include "ble_srv_common.h"
#include "ble_cus.h"
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "bsi_config.h"

/**@brief Function for initializing the Custom Service.
 *
 * @param[out]  p_cus       Custom Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */


uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    if (p_cus == NULL || p_cus_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_cus->conn_handle               = BLE_CONN_HANDLE_INVALID;

    // Add Custom Service UUID
    ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CONFIG_SERVICE_UUID;

    // Add the Custom Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    //#define CUSTOM_CHAR_UUID_S2_MEAS_INTV     0x1401 // The time in minutes between each measurment, 1440 mins in a day
    //#define CUSTOM_CHAR_UUID_S3_MEAS_INTV     0x1402 // The time in minutes between each measurment, 1440 mins in a day
    //#define CUSTOM_CHAR_UUID_DT_ALRM_ON       0x1403 // The Delta T for pulses that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_DT_ALRM_OFF      0x1404 // The Delta T for pulses that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_DM_ALRM_S2_ON    0x1405 // The Delta for measurements that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_DM_ALRM_S2_OFF   0x1406 // The Delta for measurements that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_DM_ALRM_S3_ON    0x1407 // The Delta for measurements that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_DM_ALRM_S3_OFF   0x1408 // The Delta for measurements that sets an alarm state. 
    //#define CUSTOM_CHAR_UUID_UPLD_SZE         0x1409 // The size of data we are going to upload. in kb, 5 = 5kbyte
    //#define CUSTOM_CHAR_UUID_SENS_CNFG        0x1410 // The sensors attached to the device. PULSE,ANLG,ANLG. 1 = connected, 0 = disconnected. ie 110.
    //#define CUSTOM_CHAR_UUID_SENS_ADDRS       0x1411 // The Addresses of the attached sensors (uint-16, uint-16, uint-16). 
    //#define CUSTOM_CHAR_UUID_UPLD_INTV        0x1412 // The interval that the BSI uploads the data. 

    // Add Custom Value characteristic 
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_S2_MEAS_INTV,"Analog 1 Measurement Interval");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_S3_MEAS_INTV,"Analog 2 Measurement Interval");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DT_ALRM_ON,"Delta Time Alarm On");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DT_ALRM_OFF,"Delta Time Alarm Off");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DM_ALRM_S2_ON,"Delta Measurment S2 Alarm On");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DM_ALRM_S2_OFF,"Delta Measurment S2 Alarm Off");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DM_ALRM_S3_ON,"Delta Measurment S3 Alarm On");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_DM_ALRM_S3_OFF,"Delta Measurment S3 Alarm Off");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_UPLD_SZE,"Upload Size");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_SENS_CNFG,"Sensor Configuration");
    //custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_SENS_ADDRS,"Sensor Addresses");
    custom_value_char_add(p_cus, p_cus_init,CUSTOM_CHAR_UUID_UPLD_INTV,"Upload Interval");
    return 0;
}

/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init, ble_uuid_t * p_cus_uuid, char * uuid_Desc)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read         = 1;
    char_md.char_props.write        = 1;
    char_md.char_props.notify       = 0; 
    char_md.p_char_user_desc        = uuid_Desc;
    char_md.char_user_desc_size     = strlen(char_md.p_char_user_desc);
    char_md.char_user_desc_max_size = char_md.char_user_desc_size;
    char_md.p_char_pf               = NULL;
    char_md.p_user_desc_md          = NULL;
    char_md.p_cccd_md               = NULL; 
    char_md.p_sccd_md               = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    
    //Sets the read/write permissions of the characteristic.
    attr_md.read_perm  = p_cus_init->custom_value_char_attr_md.read_perm;
    attr_md.write_perm = p_cus_init->custom_value_char_attr_md.write_perm;
    //Need to call the 2 lines below to acctually push the permissions.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = p_cus->uuid_type;
    //ble_uuid.uuid = CUSTOM_CHAR_UUID_MEAS_INTV;//Specified in the BLE_CUS.h file.
    ble_uuid.uuid = (int)p_cus_uuid;
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint16_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = sizeof(uint16_t);
    attr_char_value.p_value   = 0;
    
    err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cus->custom_value_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle; //Tells the service the device is now valid.
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID; //Tells the service the connection is no longer valid.
}

void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
//    #define CUSTOM_CHAR_UUID_S2_MEAS_INTV     0x1401 // The time in minutes between each measurment, 1440 mins in a day
//    #define CUSTOM_CHAR_UUID_S3_MEAS_INTV     0x1402 // The time in minutes between each measurment, 1440 mins in a day
//    #define CUSTOM_CHAR_UUID_DT_ALRM_ON       0x1403 // The Delta T for pulses that sets an alarm state.
//    #define CUSTOM_CHAR_UUID_DT_ALRM_OFF      0x1404 // The Delta T for pulses that sets an alarm state. 
//    #define CUSTOM_CHAR_UUID_DM_ALRM_S2_ON    0x1405 // The Delta for measurements that sets an alarm state. 
//    #define CUSTOM_CHAR_UUID_DM_ALRM_S2_OFF   0x1406 // The Delta for measurements that sets an alarm state. 
//    #define CUSTOM_CHAR_UUID_DM_ALRM_S3_ON    0x1407 // The Delta for measurements that sets an alarm state. 
//    #define CUSTOM_CHAR_UUID_DM_ALRM_S3_OFF   0x1408 // The Delta for measurements that sets an alarm state. 
//    #define CUSTOM_CHAR_UUID_UPLD_SZE         0x1409 // The size of data we are going to upload. in kb, 5 = 5kbyte
//    #define CUSTOM_CHAR_UUID_SENS_CNFG        0x1410 // The sensors attached to the device. PULSE,ANLG,ANLG. 1 = connected, 0 = disconnected. ie 110.
//    #define CUSTOM_CHAR_UUID_SENS_ADDRS       0x1411 // The Addresses of the attached sensors (uint-16, uint-16, uint-16). 
//    #define CUSTOM_CHAR_UUID_UPLD_INTV        0x1412 // The interval that the BSI uploads the data.  
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
    bsi_config.configChanged = true;
    
    switch(p_evt_write->uuid.uuid)
     {
     case CUSTOM_CHAR_UUID_S2_MEAS_INTV:     
       // The time in minutes between each measurment.
       //sensor2_config.configChanged     = true;
       bsi_config.sensor2_config.measInterval      = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_S3_MEAS_INTV:
       // The time in minutes between each measurment.
       //sensor3_config.configChanged     = true;
       bsi_config.sensor3_config.measInterval      = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DT_ALRM_ON:
       // The Delta T for pulses that sets an alarm state. IF time between pulses is below x time, set alarm.
       //sensor1_config.configChanged     = true;
       bsi_config.sensor1_config.deltaTimeAlarmOn  = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DT_ALRM_OFF:
       // The Delta for measurements that sets an alarm state. 
       //sensor1_config.configChanged     = true;
       bsi_config.sensor1_config.deltaTimeAlarmOff = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DM_ALRM_S2_ON:
       // The Delta for measurements that sets an alarm state for S2. 
       //sensor2_config.configChanged     = true;
       bsi_config.sensor2_config.deltaMeasAlarmOn  = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DM_ALRM_S2_OFF:
       // The Delta for measurements that sets an alarm state for S2. 
       //sensor2_config.configChanged     = true;
       bsi_config.sensor2_config.deltaMeasAlarmOff = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DM_ALRM_S3_ON:
       // The Delta for measurements that sets an alarm state for S3. 
       //sensor3_config.configChanged     = true;
       bsi_config.sensor3_config.deltaMeasAlarmOn  = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_DM_ALRM_S3_OFF:
       // The Delta for measurements that sets an alarm state for S3.
       //sensor3_config.configChanged     = true;
       bsi_config.sensor3_config.deltaMeasAlarmOff = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_UPLD_SZE:
       // What size of data are we going to upload. in kb, 5 = 5kbyte
       bsi_config.uploadSize = (int)p_evt_write->data;
       break;
     case CUSTOM_CHAR_UUID_SENS_CNFG:
       // The sensors attached to the device. PULSE,ANLG,ANLG. 1 = connected, 0 = disconnected. ie 110.
       init_sensors(p_evt_write->data[0]);
       break;
//     case CUSTOM_CHAR_UUID_SENS_ADDRS:
//       // The Addresses of the attached sensors (uint-16, uint-16, uint-16). 
//       break;
     case CUSTOM_CHAR_UUID_UPLD_INTV:
       // The interval that the BSI uploads the data.
       bsi_config.uploadSize = (int)p_evt_write->data;
       break;
    }
    //We are going to want to switch on the uuid of the written event
    //then update the FDS with the new values from the LoLi.
    
    // Check if the handle passed with the event matches the Custom Value Characteristic handle.
    if (p_evt_write->handle == p_cus->custom_value_handles.value_handle)
    {
        nrf_gpio_pin_toggle(LED_4); //dont care about toggling an LED, reserved for future use
    }
}

//BLE Event handler -- Moved to main.c there is already an event handler for connect
//void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
//{
//    ble_cus_t * p_cus = (ble_cus_t *) p_context;
//
//    if (p_cus == NULL || p_ble_evt == NULL)
//    {
//        return;
//    }
//    
//    switch (p_ble_evt->header.evt_id)
//    {
//       case BLE_GAP_EVT_CONNECTED:
//            on_connect(p_cus, p_ble_evt);
//            break;
//
//       case BLE_GAP_EVT_DISCONNECTED:
//            on_disconnect(p_cus, p_ble_evt);
//            break;
//       
//       case BLE_GATTS_EVT_WRITE:
//           on_write(p_cus, p_ble_evt);
//           break;
//
//       default:
//            // No implementation needed.
//            break;
//    }
//}