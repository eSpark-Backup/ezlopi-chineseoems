#ifndef __WATER_DETECTOR_ALARM_H__
#define __WATER_DETECTOR_ALARM_H__

#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

bool get_alarm_value();
uint8_t get_battery_value();
bool water_alarm_init(s_ezlopi_uart_t *water_alarm_uart_config);
#endif//__WATER_DETECTOR_ALARM_H__