#ifndef __SCENT_DIFFUSER_H
#define __SCENT_DIFFUSER_H

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

typedef enum
{
    CMD_CONNECTION_START,
    CMD_HEARTBEAT,
    CMD_TURN_ON,
    CMD_TURN_OFF,
    CMD_FAN_ON,
    CMD_FAN_OFF,
    CMD_SEQUENCE_ON,
    CMD_SEQUENCE_OFF,
    CMD_WIFI,
} CommandType;

bool scent_diffuser_init(s_ezlopi_uart_t *scent_diffuser_uart_config);
void send_command(CommandType cmd);
void start_packet();
void heart_beat_packet();

#endif

