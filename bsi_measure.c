//The intent of this file to keep all the code needed to make a measurment from the sensors in one place.
//This code will need to:
//-Set up the ADC AN1 = Analog Sensor1, AN0 = Analog Sensor2
//                SIO_12 = Analog Sensor1 PWR, SIO_11 = Analog Sensor2 PWR,
//-Set up the an Interrupt handler for the ADC
//-Set up the code to pull the data from the ADC and put it into the m_buffer_tx array
//-Set up PPI for the ADC? Not needed, PPI is only needed if a timer is going to wake the ADC directly.
//-Enable/Disable GPIO for toggling sensor power.
// Refer to "\examples\peripheral\saadc\main.c" for an example of implementing the ADC with PPI
// https://devzone.nordicsemi.com/f/nordic-q-a/18174/why-there-is-no-easy-api-for-analog-input-of-nrf52-saadc - link to a simpler example



//#include <nrfx_saadc.h>
//#include "nrf_drv_saadc.h"
#include "bsi_measure.h"

#define BUFFER_SIZE 1
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

 void saadc_init(void)
{
    nrfx_err_t err_code;
    nrf_saadc_channel_config_t channel0_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);

    nrf_saadc_channel_config_t channel1_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel0_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(1, &channel1_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer, BUFFER_SIZE);
    APP_ERROR_CHECK(err_code);
}