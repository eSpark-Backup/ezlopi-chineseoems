#ifndef __BHT003_H__
#define __BHT003_H__

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

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
void send_temp_sp(float value, CommandType cmd);
void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length, float data);
bool bht003_init(s_ezlopi_uart_t *bht003_uart_config);

bool bht003_power_value();
uint32_t bht003_temp_sp_value();
float bht003_amb_temp_value();
bool bht003_child_lock_value();
uint8_t bht003_mode_selection_value();
bool bht003_energy_saving_value();
#endif//__BHT003_H__