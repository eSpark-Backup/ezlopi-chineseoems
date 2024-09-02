#ifndef __BHT003_H__
#define __BHT003_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"

// bool heartbeatMode = false;

typedef enum
{
    CMD_HEARTBEAT,
    CMD_TURN_ON,
    CMD_TURN_OFF,
    CMD_PROGRAM_MODE,
    CMD_MANUAL_MODE,
    CMD_ENERGY_SAVING_ON,
    CMD_ENERGY_SAVING_OFF,
    CMD_LOCK_ENABLE,
    CMD_LOCK_DISABLE,
    CMD_TEMP,
    CMD_WIFI,
} CommandType;

void starting_cmd(starting_cmd);
void send_command(CommandType cmd);
void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length);
bool bht003_init(s_ezlopi_uart_t *bht003_uart_config);

#endif//__BHT003_H__