//#include <stdint.h>
#include "fds.h"

typedef struct BSISensorConfig { 
  bool configChanged;
  bool sensorEnabled;
  uint16_t sensorAddress; // dont think we are going to use this, doesnt really serve a purpose... already organised by BSI.
  uint16_t measInterval;
  uint16_t deltaTimeAlarmOn;
  uint16_t deltaTimeAlarmOff;
  uint16_t deltaMeasAlarmOn;
  uint16_t deltaMeasAlarmOff;
  uint16_t pwrOnDelay;
}BSI_Sensor_Config; 

//extern BSI_Sensor_Config sensor1_config;
//extern BSI_Sensor_Config sensor2_config;
//extern BSI_Sensor_Config sensor3_config;

typedef struct BSIConfig {
    char BSI_Name[16];
    uint32_t UTC_Minutes;
    bool configChanged;
    BSI_Sensor_Config sensor1_config;
    BSI_Sensor_Config sensor2_config;
    BSI_Sensor_Config sensor3_config;
    uint16_t uploadSize; //in Bytes
    uint32_t uploadInterval; //Duration in minutes between broadcasting data.
    uint32_t lastKnownAddr; //Used in QSPI to find write position
    uint8_t qspi_currentSector; //Used in QSPI to find read position
    uint16_t pulseTime;
    uint16_t pulseNum; 

}BSI_Config;

extern BSI_Config bsi_config;

#define fds_BSI_File (0xA001)
//#define fds_Sensor1_File (0xF001)
//#define fds_Sensor2_File (0xF002)
//#define fds_Sensor3_File (0xF003)
//
#define fds_BSI_Key (0x0001)
//#define fds_Sensor1_Key (0x0001)
//#define fds_Sensor2_Key (0x0002)
//#define fds_Sensor3_Key (0x0003)


//ret_code_t write_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec); //Handles write and update
ret_code_t write_fds(uint16_t sensorFile, uint16_t sensorKey); //Handles write and update

ret_code_t init_fds(void);

void fds_evt_handler(fds_evt_t const * p_evt);

ret_code_t delete_config_fds(void); //Deletes all the config files from the FDS

//ret_code_t read_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec,BSI_Sensor_Config * sens_config )
ret_code_t read_fds(uint16_t sensorFile, uint16_t sensorKey,BSI_Sensor_Config * sens_config ); //Reads a file from the FDS

ret_code_t init_sensors(uint16_t sensorMap);