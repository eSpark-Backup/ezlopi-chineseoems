#ifndef __DEVICE_00113_BREEZE_THERMOSTAT_H__
#define __DEVICE_00113_BREEZE_THERMOSTAT_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

#define CMD_PWR         1
#define CMD_FAN         2
#define CMD_WIFI        3
#define CMD_PAIR        4
#define CMD_PAIRED      5
#define CMD_RESET       6
#define CMD_LOCK        7
#define CMD_MODE        8
#define CMD_FORMAT      9
#define CMD_UP_LIMIT    10
#define CMD_LOW_LIMIT   11
#define CMD_PIR         12
#define CMD_HEARTBEAT   13

bool breeze_power_value();
bool lock_value();
bool breeze_fan_value();
bool breeze_format_value();
void power_update(bool value);
uint32_t upper_value();
uint32_t lower_value();

void send_command(uint8_t cmd, int value);
void create_packet(uint8_t cmd, uint8_t *packet, uint8_t *length, int data);
bool breeze_init(s_ezlopi_uart_t *breeze_uart_config);

s_ezlopi_uart_object_handle_t breeze_uart_object_handle;

#endif//__DEVICE_00113_BREEZE_THERMOSTAT_H__