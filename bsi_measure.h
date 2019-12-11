#include "nrf_drv_saadc.h"

extern uint32_t ticksPulse;
extern bool pulseAlarmOn;
extern bool pulseWriteNow;
extern char alarmState;

void saadc_callback(nrf_drv_saadc_evt_t const * p_event);
void saadc_init(void);
void gpio_init(void);
void pulse_alarm_check();
nrf_saadc_value_t measureSensor(uint8_t channel);

