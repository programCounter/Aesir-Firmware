#include <stdint.h>

//#define CUSTOM_CHAR_UUID_MEAS_INTV        0x1401 // The time in minutes between each measurment, for each Analog sensor, 2 uint 8. 
//#define CUSTOM_CHAR_UUID_DT_ALRM          0x1402 // The Delta T for pulses that sets an alarm state. IF time between pulses is below x time, set alarm.
//#define CUSTOM_CHAR_UUID_DM_ALRM          0x1403 // The Delta for measurements that sets an alarm state. IF measurments increase by x since last measure, set alarm.
//#define CUSTOM_CHAR_UUID_UPLD_SZE         0x1404 // The size what size of data are we going to upload. in kb, 5 = 5kbyte
//#define CUSTOM_CHAR_UUID_SENS_CNFG        0x1405 // The sensors attached to the device. PULSE,ANLG,ANLG. 1 = connected, 0 = disconnected. ie 110.
//#define CUSTOM_CHAR_UUID_SENS_ADDRS       0x1406 // The Addresses of the attached sensors (uint-16, uint-16, uint-16). 

#define fds_Sensor1_File (0xF001)
#define fds_Sensor2_File (0xF002)
#define fds_Sensor3_File (0xF003)

#define fds_Sensor1_Key (0x0001)
#define fds_Sensor2_Key (0x0002)
#define fds_Sensor3_Key (0x0003)

typedef struct {
  bool configChanged;
  bool sensorEnabled;
  uint16_t sensorAddress;
  uint16_t measInterval;
  uint16_t deltaTimeAlarmOn;
  uint16_t deltaTimeAlarmOff;
  uint16_t deltaMeasAlarmOn;
  uint16_t deltaMeasAlarmOff;
}BSI_Sensor_Config; 

ret_code_t write_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec); //Handles write and update

ret_code_t init_fds(void);

void fds_evt_handler(fds_evt_t const * p_evt);

ret_code_t delete_config_fds(void);

ret_code_t read_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec,BSI_Sensor_Config * sens_config );