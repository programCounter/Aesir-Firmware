//#include <stdint.h>

typedef struct BSISensorConfig { 
  bool configChanged;
  bool sensorEnabled;
  uint16_t sensorAddress;
  uint16_t measInterval;
  uint16_t deltaTimeAlarmOn;
  uint16_t deltaTimeAlarmOff;
  uint16_t deltaMeasAlarmOn;
  uint16_t deltaMeasAlarmOff;
}BSI_Sensor_Config; 

extern BSI_Sensor_Config sensor1_config;
extern BSI_Sensor_Config sensor2_config;
extern BSI_Sensor_Config sensor3_config;

#define fds_Sensor1_File (0xF001)
#define fds_Sensor2_File (0xF002)
#define fds_Sensor3_File (0xF003)

#define fds_Sensor1_Key (0x0001)
#define fds_Sensor2_Key (0x0002)
#define fds_Sensor3_Key (0x0003)


ret_code_t write_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec); //Handles write and update

ret_code_t init_fds(void);

void fds_evt_handler(fds_evt_t const * p_evt);

ret_code_t delete_config_fds(void);

ret_code_t read_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec,BSI_Sensor_Config * sens_config );