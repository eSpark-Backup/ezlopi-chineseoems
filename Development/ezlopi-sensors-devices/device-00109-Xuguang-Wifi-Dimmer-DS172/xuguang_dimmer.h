#ifndef __DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__
#define __DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

#define CMD_PWR         1
#define CMD_INTENSITY   2

// void starting_cmd(starting_cmd);
void send_command(uint8_t cmd, int value);
// void send_intensity(float value, uint8_t cmd);
void create_packet(uint8_t cmd, uint8_t *packet, uint8_t *length, int data);
bool xuguang_init(s_ezlopi_uart_t *xuguang_uart_config);

bool xuguang_power_value();
uint16_t xuguang_amb_temp_value();
#endif//__DEVICE_00109_XUGUANG_WIFI_DIMMER_SWITCH_H__