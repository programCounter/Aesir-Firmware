//The intent of this file to keep all the code needed to make a measurment from the sensors in one place.
//This code will need to:
//-Set up the ADC AN1 = Analog Sensor1, AN0 = Analog Sensor2
//                SIO_12 = Analog Sensor1 PWR, SIO_11 = Analog Sensor2 PWR,
//-Power regulator SI0_47
//-Set up the an Interrupt handler for the ADC
//-Set up the code to pull the data from the ADC and put it into the m_buffer_tx array
//-Set up PPI for the ADC? Not needed, PPI is only needed if a timer is going to wake the ADC directly.
//-Enable/Disable GPIO for toggling sensor power.
// Refer to "\examples\peripheral\saadc\main.c" for an example of implementing the ADC with PPI
// https://devzone.nordicsemi.com/f/nordic-q-a/18174/why-there-is-no-easy-api-for-analog-input-of-nrf52-saadc - link to a simpler example


#include "bsi_measure.h"
#include "bsi_qspi.h"
#include "BLE_CUS.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "bsi_config.h"

#define BUFFER_SIZE 3
//Pin_map(port,pin)
//pin map for the data lines to the 2 analog sensors, the pulse, and the voltage regulator.
#define VREG_PWR NRF_GPIO_PIN_MAP(1,15) 
#define ANLG_SENSOR1_PWR NRF_GPIO_PIN_MAP(0,12) 
#define ANLG_SENSOR2_PWR NRF_GPIO_PIN_MAP(0,11) 
#define PULSE_SENSOR_INP NRF_GPIO_PIN_MAP(1,02) 

#ifdef DEBUG
#define ALARM_RATE_ON 30
#define ALARM_RATE_OFF 10
#endif

static nrf_saadc_value_t m_buffer[BUFFER_SIZE];
uint32_t ticksPulse = false;
bool pulseAlarmOn = false;
bool pulseWriteNow = false;
char alarmState = 0; // 0 = off | 1 = alarm off, alarm state on | 2 = alarm on, alarm state on

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrfx_err_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, BUFFER_SIZE);
        APP_ERROR_CHECK(err_code);
    }
}

//Checks the time between pulses and determins if the BSI should enter an alarm state
void pulse_alarm_check()
{
  if(pulseAlarmOn)
  {
    #ifdef DEBUG
    if((pulseInterval/(ticksPulse - bsi_config.pulseTime)) < ALARM_RATE_OFF)
    #else
    if((pulseInterval/(ticksPulse - bsi_config.pulseTime)) < bsi_config.sensor1_config.deltaMeasAlarmOff)
    #endif
    {
      //shut off alarm
      #ifdef DEMO_WRITE
      printf("Whew We Safe\n"); // Clear the alarm on the LoLi
      #endif
      pulseAlarmOn = false;
      alarmState = 1; //state changed, notify the loli
    }
  }
  else
  {
    #ifdef DEBUG
    if((pulseInterval/(ticksPulse - bsi_config.pulseTime)) > ALARM_RATE_ON)
    #else
    if((pulseInterval/(ticksPulse - bsi_config.pulseTime)) > bsi_config.sensor1_config.deltaMeasAlarmOn)
    #endif
    {
      //enter alarm mode
      #ifdef DEMO_WRITE
      printf("AAAHHHHH ALARM!!!\n"); // Send the alarm to the Loli
      #endif
      pulseAlarmOn = true;
      alarmState = 2; //state changed, notify the loli
    }
  }
}

//event handler for the pulse input. 
void pulse_evt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    bsi_config.pulseNum++;
    pulse_alarm_check(); // check if the alarm needs to be set.
    bsi_config.pulseTime = ticksPulse; // write the current time of the pulse
    bsi_config.configChanged = true; // the pulse count has been incremenet, change the flag so the info get saved.
}

void gpio_init(void)
{
  //Analog and voltage reg config
  nrf_gpio_cfg_output(VREG_PWR);
  nrf_gpio_cfg_output(ANLG_SENSOR1_PWR);
  nrf_gpio_cfg_output(ANLG_SENSOR2_PWR);
  
  //Pulse sensor config
  ret_code_t err_code;

  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);

 // nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

    //err_code = nrf_drv_gpiote_out_init(PIN_OUT, &out_config);
    //APP_ERROR_CHECK(err_code);

  //Set the mode for the pulse input, event fire on low to high transition.
  nrf_drv_gpiote_in_config_t pulse_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
  pulse_config.pull = NRF_GPIO_PIN_NOPULL;//no pullup resistor needed, have one on the board

  //Register our event handler for the pulse input.
  err_code = nrf_drv_gpiote_in_init(PULSE_SENSOR_INP, &pulse_config, pulse_evt_handler); 
  APP_ERROR_CHECK(err_code);
  nrf_drv_gpiote_in_event_enable(PULSE_SENSOR_INP, true);
}


//Init the ADC channels we are going to need for the 2 analog sensors and the battery level resistor
void saadc_init(void)
{
    nrfx_err_t err_code;
    nrf_saadc_channel_config_t channel0_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);

    nrf_saadc_channel_config_t channel1_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);

    nrf_saadc_channel_config_t vBatt_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN6);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel0_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(1, &channel1_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(6, &vBatt_config);
    APP_ERROR_CHECK(err_code);

    //err_code = nrf_drv_saadc_buffer_convert(m_buffer[0], BUFFER_SIZE); //possibly not needed, function samples all adc's and stores values into buffer
    //APP_ERROR_CHECK(err_code);
}

nrf_saadc_value_t measureSensor(uint8_t channel)
{
  nrf_saadc_value_t p_ADC_Result;
  switch(channel)
  {
    case 0:
      nrf_gpio_pin_write(VREG_PWR,1); //Power Regulator On
      nrf_delay_ms(1);
      nrf_gpio_pin_write(ANLG_SENSOR1_PWR,1); //Sensor power on
      nrf_delay_ms(bsi_config.sensor2_config.pwrOnDelay); //Just pull the mS ammount from the config struct. // The warm up time needed before the sensor is in a steady state
      nrfx_saadc_sample_convert(channel,&p_ADC_Result);// This returns a single value from the specified ADC channel. THIS FUNCTION IS BLOCKING!
      nrf_gpio_pin_write(ANLG_SENSOR1_PWR,0); //Sensor power off
      nrf_gpio_pin_write(VREG_PWR,0); //Power Regulator Off
      break;
    case 1:
      nrf_gpio_pin_write(VREG_PWR,1); //Power Regulator On
      nrf_delay_ms(1);
      nrf_gpio_pin_write(ANLG_SENSOR2_PWR,1); //Sensor power on
      nrf_delay_ms(bsi_config.sensor3_config.pwrOnDelay);//Just pull the mS ammount from the config struct. // The warm up time needed before the sensor is in a steady state
      nrfx_saadc_sample_convert(channel,&p_ADC_Result);// This returns a single value from the specified ADC channel. THIS FUNCTION IS BLOCKING!
      nrf_gpio_pin_write(ANLG_SENSOR2_PWR,0); //Sensor power off
      nrf_gpio_pin_write(VREG_PWR,0); //Power Regulator Off
      break;
     case 2:
      //pulse sensor data upload
      p_ADC_Result = bsi_config.pulseNum;
      bsi_config.pulseNum = 0;
      break;
    case 6:
      //Check the battery level.
      nrfx_saadc_sample_convert(channel,&p_ADC_Result);// This returns a single value from the specified ADC channel. THIS FUNCTION IS BLOCKING!
      break;
    default:
      break;
  } 
  //CurrentPage.sensorCh = channel;
  //CurrentPage.sensorValue = p_ADC_Result;
  //CurrentPage.sensorValue = CurrentPage.sensorValue | channel;
  CurrentPage.countMin = ticksTUpload;
  CurrentPage.sensorValue = (p_ADC_Result << 4) | channel;
  return p_ADC_Result;
}