#ifndef __DEVICE_00110_QIAO_WIFI_CIRCUIT_BREAKER_H__
#define __DEVICE_00110_QIAO_WIFI_CIRCUIT_BREAKER_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

typedef struct values
{
    bool switch_value;
    bool mode_value;
    bool lock_value;
} values_t;

bool qiao_switch_value();
bool qiao_mode_value();
bool qiao_lock_value();
void lock_update(uint8_t value);

extern values_t Values;

void send_command(int value);
bool qiao_init(s_ezlopi_uart_t *xuguang_uart_config);
void paired();

#endif//__DEVICE_00110_QIAO_WIFI_CIRCUIT_BREAKER_H__