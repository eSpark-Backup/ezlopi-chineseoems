#ifndef __WATER_DETECTOR_ALARM_H__
#define __WATER_DETECTOR_ALARM_H__

#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

bool get_alarm_value();
uint8_t get_battery_value();
bool water_alarm_init(l_ezlopi_item_t* item);

#endif//__WATER_DETECTOR_ALARM_H__