#ifndef __BECA_1000_3A_H__
#define __BECA_1000_3A_H__


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
    CMD_LOCK_ENABLE,
    CMD_LOCK_DISABLE,
    CMD_TEMP,
    CMD_TIME,
    CMD_WIFI,
} CommandType;






bool beca10003A_init(s_ezlopi_uart_t *bht003_uart_config);
float beca10003A_amb_temp_value();
float beca10003A_temp_sp_value();
bool beca10003A_child_lock_value();
bool beca10003A_power_value();
uint8_t beca10003A_operational_mode_selection_value();
void send_command(CommandType cmd);
void send_temp_sp(int value,CommandType cmd);














#endif //__BECA_1000_3A_H__