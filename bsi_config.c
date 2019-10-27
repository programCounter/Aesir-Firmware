#include "bsi_config.h"

//Flash data storage. FDS

/* Array to map FDS return values to strings. */
char const * fds_err_str[] =
{
    //From the FDS example
    "FDS_SUCCESS",
    "FDS_ERR_OPERATION_TIMEOUT",
    "FDS_ERR_NOT_INITIALIZED",
    "FDS_ERR_UNALIGNED_ADDR",
    "FDS_ERR_INVALID_ARG",
    "FDS_ERR_NULL_ARG",
    "FDS_ERR_NO_OPEN_RECORDS",
    "FDS_ERR_NO_SPACE_IN_FLASH",
    "FDS_ERR_NO_SPACE_IN_QUEUES",
    "FDS_ERR_RECORD_TOO_LARGE",
    "FDS_ERR_NOT_FOUND",
    "FDS_ERR_NO_PAGES",
    "FDS_ERR_USER_LIMIT_REACHED",
    "FDS_ERR_CRC_CHECK_FAILED",
    "FDS_ERR_BUSY",
    "FDS_ERR_INTERNAL",
};


/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
    //From the FDS example
    "FDS_EVT_INIT",
    "FDS_EVT_WRITE",
    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD",
    "FDS_EVT_DEL_FILE",
    "FDS_EVT_GC",
};

//Init the struct for sensor 1
//BSI_Sensor_Config s1_config = 
//{
//  .configChanged      = false,
//  .sensorEnabled      = false,
//  .sensorAddress      = 0,
//  .measInterval       = 0,
//  .deltaTimeAlarmOn   = 0,
//  .deltaTimeAlarmOff  = 0,
//  .deltaMeasAlarmOn   = 0,
//  .deltaMeasAlarmOff  = 0,
//};

//Init the struct for sensor 2
//BSI_Sensor_Config s2_config =
//{
//  .configChanged      = false,
//  .sensorEnabled      = false,
//  .sensorAddress      = 0,
//  .measInterval       = 0,
//  .deltaTimeAlarmOn   = 0,
//  .deltaTimeAlarmOff  = 0,
//  .deltaMeasAlarmOn   = 0,
//  .deltaMeasAlarmOff  = 0,
//};

//Init the struct for sensor 3
//BSI_Sensor_Config s3_config =
//{
//  .configChanged      = false,
//  .sensorEnabled      = false,
//  .sensorAddress      = 0,
//  .measInterval       = 0,
//  .deltaTimeAlarmOn   = 0,
//  .deltaTimeAlarmOff  = 0,
//  .deltaMeasAlarmOn   = 0,
//  .deltaMeasAlarmOff  = 0,
//};

BSI_Config bsi_config = {
  .configChanged  = false,
//  .sensor1_config = s1_config,
//  .sensor2_config = s2_config,
//  .sensor3_config = s3_config,
  .uploadSize     = 9,
  .uploadInterval = 0,
};

//Create FDS record for Sensor1
//static fds_record_t const FDS_sensor1_record =
//{
//    .file_id           = fds_Sensor1_File,
//    .key               = fds_Sensor1_Key,
//    .data.p_data       = &bsi_config.sensor1_config,
//    /* The length of a record is always expressed in 4-byte units (words). */
//    .data.length_words = (sizeof(bsi_config.sensor1_config) + 3) / sizeof(uint32_t),
//};
//Create FDS record for Sensor2
//static fds_record_t const FDS_sensor2_record =
//{
//    .file_id           = fds_Sensor2_File,
//    .key               = fds_Sensor2_Key,
//    .data.p_data       = &bsi_config.sensor2_config,
//    /* The length of a record is always expressed in 4-byte units (words). */
//    .data.length_words = (sizeof(bsi_config.sensor2_config) + 3) / sizeof(uint32_t),
//};
//Create FDS record for Sensor3
//static fds_record_t const FDS_sensor3_record =
//{
//    .file_id           = fds_Sensor3_File,
//    .key               = fds_Sensor3_Key,
//    .data.p_data       = &bsi_config.sensor3_config,
//    /* The length of a record is always expressed in 4-byte units (words). */
//    .data.length_words = (sizeof(bsi_config.sensor3_config) + 3) / sizeof(uint32_t),
//};

//Create FDS record for Sensor3
static fds_record_t const FDS_BSI_record =
{
    .file_id           = fds_BSI_File,
    .key               = fds_BSI_Key,
    .data.p_data       = &bsi_config,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(bsi_config) + 3) / sizeof(uint32_t),
};

ret_code_t read_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec,BSI_Sensor_Config * sens_config )
{
  ret_code_t retCode;
  fds_record_desc_t descript    = {0};
  fds_find_token_t  token       = {0};
  fds_flash_record_t flash_rec  = {0};
  retCode = fds_record_find(sensorFile, sensorKey, &descript, &token);

  /* Open the record and read its contents. */
  retCode = fds_record_open(&descript, &sens_config);
  APP_ERROR_CHECK(retCode);

  memcpy(&sens_config, flash_rec.p_data, sizeof(BSI_Sensor_Config));
}

ret_code_t write_fds(uint16_t sensorFile, uint16_t sensorKey, fds_record_t fdsRec)
{
  //This checks if the passed file exists. IF the file exists, it updates it. Otherwise, it creates the file
  ret_code_t retCode;
  fds_record_desc_t descript  = {0};
  fds_find_token_t  token     = {0};
  
  //Check if the file exists in the flash
  retCode = fds_record_find(sensorFile, sensorKey, &descript, &token);
  
  //switch on the retCode, are we updating or making a new file?
  switch(retCode)
  {
    case FDS_SUCCESS:
      //The file exists, we can update it.
      retCode = fds_record_update(&descript, &fdsRec);
      break;

    case FDS_ERR_NOT_FOUND:
      //The file DOES NOT exists, we can need to make it.
      retCode = fds_record_write(&descript, &fdsRec);
      break;
  }
  return retCode;
}


/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

ret_code_t init_fds()
{
    //Code from the FDS examples for setting up the FDS
    ret_code_t retCode;
     /* Register first to receive an event when initialization is complete. */
    (void) fds_register(fds_evt_handler);
   
    retCode = fds_init();
    APP_ERROR_CHECK(retCode);

    /* Wait for fds to initialize. */
    while(m_fds_initialized!=true)
    {
      (void) sd_app_evt_wait();//Sleep till next event. This line requires a soft device to be present.
    }
    return retCode;
}

static bool volatile fds_record_deleted;
ret_code_t delete_config_fds(void)
{
    ret_code_t retCode;
    fds_record_desc_t descript  = {0};
    fds_find_token_t  token     = {0};
    
    //Loop through the records stored and delete them one by one.
    while (fds_record_iterate(&descript, &token) == FDS_SUCCESS)
    {
      fds_record_deleted=false;
      retCode  = fds_record_delete(&descript);
      while(fds_record_deleted==false) //Tie this up until we get the event saying that the record was deleted.
      {
        (void) sd_app_evt_wait();//Sleep till next event. This line requires a soft device to be present.
      }
    }
    
    return retCode;
}
bool update_BSI_Config(void)
{
    // Updates the FDS with the new config after it's been updated through the characteristic
    bool retVal = true;
    if(bsi_config.configChanged == false )
    {
      retVal = false; // No configs have changed, time to leave
    }
    else 
    {
      if(bsi_config.configChanged == true)
      {
        write_fds(fds_BSI_File,fds_BSI_Key,FDS_BSI_record);
      }
//      if(bsi_config.sensor1_config.configChanged == true)
//      {
//        write_fds(fds_Sensor1_File,fds_Sensor1_Key,FDS_sensor1_record);
//      }
//      if(bsi_config.sensor2_config.configChanged == true)
//      {
//        write_fds(fds_Sensor2_File,fds_Sensor2_Key,FDS_sensor2_record);
//      }
//      if(bsi_config.sensor3_config.configChanged == true)
//      {
//        write_fds(fds_Sensor3_File,fds_Sensor3_Key,FDS_sensor3_record);
//      }
    }

    return retVal; //did we update the config?
}

void fds_evt_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_initialized = true; // Set the flag so we can verify that the init funtion actually finished
            }
         break;

        case FDS_EVT_DEL_RECORD:
        {
          fds_record_deleted = true; //Set the flag so that the program knows the delet operation finished
        } 
        break;

        default:
        break;
    }
}

ret_code_t init_sensors(uint16_t sensorMap)
{
    uint8_t sensors = Uint8(sensorMap & 0xff);


}