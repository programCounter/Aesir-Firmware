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
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "bsi_config.h"

#define BUFFER_SIZE 3
//Pin_map(port,pin)
#define VREG_PWR NRF_GPIO_PIN_MAP(1,15) 
#define ANLG_SENSOR1_PWR NRF_GPIO_PIN_MAP(0,12) 
#define ANLG_SENSOR2_PWR NRF_GPIO_PIN_MAP(0,11) 
#define PULSE_SENSOR_INP NRF_GPIO_PIN_MAP(1,04) 

static nrf_saadc_value_t m_buffer[BUFFER_SIZE];

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrfx_err_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, BUFFER_SIZE);
        APP_ERROR_CHECK(err_code);
    }
}

void pulse_evt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // What happens when we get a pulse?
    //nrf_drv_gpiote_out_toggle(PIN_OUT);
}

void gpio_init(void)
{
  //Analog and voltage reg config
  nrf_gpio_cfg_output(VREG_PWR);
  nrf_gpio_cfg_output(ANLG_SENSOR1_PWR);
  nrf_gpio_cfg_output(ANLG_SENSOR2_PWR);
  
  //Pulse sensor config
  nrf_drv_gpiote_in_config_t pulse_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
  pulse_config.pull = NRF_GPIO_PIN_PULLUP;

  nrf_drv_gpiote_in_init(PULSE_SENSOR_INP, &pulse_config, pulse_evt_handler); 
  nrf_drv_gpiote_in_event_enable(PULSE_SENSOR_INP, true);
}



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

nrf_saadc_value_t measureSensor(uint8_t channel, uint32_t *currentMins)
{
  nrf_saadc_value_t p_ADC_Result;
  switch(channel)
  {
    case 0:
      nrf_gpio_pin_write(VREG_PWR,1); //Power Regulator On
      nrf_delay_ms(1);
      nrf_gpio_pin_write(ANLG_SENSOR1_PWR,1); //Sensor power on
      nrf_delay_ms(bsi_config.sensor2_config.pwrOnDelay); //Just pull the mS ammount from the config struct.
      nrfx_saadc_sample_convert(channel,&p_ADC_Result);// This returns a single value from the specified ADC channel. THIS FUNCTION IS BLOCKING!
      nrf_gpio_pin_write(ANLG_SENSOR1_PWR,0); //Sensor power off
      nrf_gpio_pin_write(VREG_PWR,0); //Power Regulator Off
      break;
    case 1:
      nrf_gpio_pin_write(VREG_PWR,1); //Power Regulator On
      nrf_delay_ms(1);
      nrf_gpio_pin_write(ANLG_SENSOR2_PWR,1); //Sensor power on
      nrf_delay_ms(bsi_config.sensor3_config.pwrOnDelay);//Just pull the mS ammount from the config struct.
      nrfx_saadc_sample_convert(channel,&p_ADC_Result);// This returns a single value from the specified ADC channel. THIS FUNCTION IS BLOCKING!
      nrf_gpio_pin_write(ANLG_SENSOR2_PWR,0); //Sensor power off
      nrf_gpio_pin_write(VREG_PWR,0); //Power Regulator Off
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
  CurrentPage.sensorValue = (p_ADC_Result << 4) | channel;
  CurrentPage.countMin = &currentMins;
  return p_ADC_Result;
}