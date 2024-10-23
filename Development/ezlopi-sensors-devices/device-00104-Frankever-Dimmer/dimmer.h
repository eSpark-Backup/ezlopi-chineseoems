#ifndef __DIMMER_H__
#define __DIMMER_H__

#include "ezlopi_hal_uart.h"

#define HEADER 0xAA
#define POWER_OFF 0x00
#define POWER_ON 0x01

void send_command_dimmer(int value);
void send_command_switch(bool value);
bool dimmer_init(s_ezlopi_uart_t *dimmer);

#endif //__DIMMER_H__