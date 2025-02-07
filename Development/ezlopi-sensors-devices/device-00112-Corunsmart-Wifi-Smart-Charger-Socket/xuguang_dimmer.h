#ifndef __DEVICE_00112_CORUNSMART_WIFI_CHARGER_SOCKET_H__
#define __DEVICE_00112_CORUNSMART_WIFI_CHARGER_SOCKET_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

#define CMD_PWR         1
#define CMD_INTENSITY   2
#define CMD_WIFI        3
#define CMD_PAIR        4
#define CMD_PAIRED      5
#define CMD_RESET       6

bool xuguang_power_value();
void power_update(bool value);
uint16_t xuguang_amb_temp_value();
void intensity_update(uint8_t value);

void send_command(uint8_t cmd, int value);
void create_packet(uint8_t cmd, uint8_t *packet, uint8_t *length, int data);
bool corunsmart_init(s_ezlopi_uart_t *xuguang_uart_config);

#endif//__DEVICE_00112_CORUNSMART_WIFI_CHARGER_SOCKET_H__