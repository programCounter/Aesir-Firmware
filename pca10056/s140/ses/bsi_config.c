#include "fds.h"
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
BSI_Sensor_Config sensor1_config =
{
  .configChanged  = false,
  .sensorEnabled  = false,
  .sensorAddress  = 0,
  .measInterval   = 0,
  .deltaTimeAlarm = 0,
  .deltaMeasAlarm = 0,
};

//Init the struct for sensor 2
BSI_Sensor_Config sensor2_config =
{
  .configChanged  = false,
  .sensorEnabled  = false,
  .sensorAddress  = 0,
  .measInterval   = 0,
  .deltaTimeAlarm = 0,
  .deltaMeasAlarm = 0,
};

//Init the struct for sensor 3
BSI_Sensor_Config sensor3_config =
{
  .configChanged  = false,
  .sensorEnabled  = false,
  .sensorAddress  = 0,
  .measInterval   = 0,
  .deltaTimeAlarm = 0,
  .deltaMeasAlarm = 0,
};

//Create FDS record for Sensor1
static fds_record_t const FDS_sensor1_record =
{
    .file_id           = fds_Sensor1_File,
    .key               = fds_Sensor1_Key,
    .data.p_data       = &sensor1_config,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(sensor1_config) + 3) / sizeof(uint32_t),
};
//Create FDS record for Sensor2
static fds_record_t const FDS_sensor2_record =
{
    .file_id           = fds_Sensor2_File,
    .key               = fds_Sensor2_Key,
    .data.p_data       = &sensor2_config,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(sensor2_config) + 3) / sizeof(uint32_t),
};
//Create FDS record for Sensor3
static fds_record_t const FDS_sensor3_record =
{
    .file_id           = fds_Sensor3_File,
    .key               = fds_Sensor3_Key,
    .data.p_data       = &sensor3_config,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(sensor3_config) + 3) / sizeof(uint32_t),
};

ret_code_t read_fds()
{
  rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);

  /* Open the record and read its contents. */
  rc = fds_record_open(&desc, &config);
  APP_ERROR_CHECK(rc);

  memcpy(&m_dummy_cfg, config.p_data, sizeof(configuration_t));
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