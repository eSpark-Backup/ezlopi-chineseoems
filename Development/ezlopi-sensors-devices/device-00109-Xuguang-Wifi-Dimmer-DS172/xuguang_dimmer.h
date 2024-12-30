#ifndef __DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__
#define __DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

// void starting_cmd(starting_cmd);
// void send_command(CommandType cmd);
// void send_temp_sp(float value, CommandType cmd);
// void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length, float data);
bool xuguang_init(s_ezlopi_uart_t *xuguang_uart_config);

bool xuguang_power_value();
float xuguang_amb_temp_value();
#endif//__DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__