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
uint32_t beca10003A_temp_sp_value();















#endif //__BECA_1000_3A_H__