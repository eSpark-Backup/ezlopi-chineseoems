#ifndef __WATER_DETECTOR_ALARM_H__
#define __WATER_DETECTOR_ALARM_H__

#include "nvs.h"
#include "nvs_flash.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

bool get_alarm_value();
void battery_nvs_read();
uint8_t get_battery_value();
bool wifi_connection_status();
void send_packet(uint8_t* packet);
bool water_alarm_init(l_ezlopi_item_t* item);

#endif//__WATER_DETECTOR_ALARM_H__