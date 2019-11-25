#include "nrf_drv_saadc.h"

void saadc_callback(nrf_drv_saadc_evt_t const * p_event);
void saadc_init(void);
void gpio_init(void);
nrf_saadc_value_t measureSensor(uint8_t channel, uint32_t * currentMins);

