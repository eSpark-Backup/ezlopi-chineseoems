#include "dimmer.h"
#include "ezlopi_util_trace.h"

uint8_t packet[5] = {HEADER,0,0,0,0};
s_ezlopi_uart_object_handle_t dimmer_uart_object_handle;

void dimmer_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    
}

// int hexToDecimal(char hex[])
// {
//     int length = strlen(hex);
//     int base = 1;
//     int decimalValue = 0;

//     for (int i = length - 1; i >= 0; i--) {
//         if (hex[i] >= '0' && hex[i] <= '9') {
//             decimalValue += (hex[i] - '0') * base;
//             base *= 16;
//         }
//         else if (hex[i] >= 'A' && hex[i] <= 'F') {
//             decimalValue += (hex[i] - 'A' + 10) * base;
//             base *= 16;
//         }
//         else if (hex[i] >= 'a' && hex[i] <= 'f') {
//             decimalValue += (hex[i] - 'a' + 10) * base;
//             base *= 16;
//         }
//     }
//     return decimalValue;
// }

void send_command_dimmer(int value)
{
    packet[2] = POWER_ON;
    packet[3] = value;
    packet[4] = packet[1] + packet[2] + packet[3]; // checksum
    // TRACE_I("SENDING COMMANDDDDDDDDDDDDDDDDDDDDDDDDDDDD DIMMER");
    uart_write_bytes(dimmer_uart_object_handle->ezlopi_uart.channel, (const char *)packet,sizeof(packet));
    ++packet[1];
}

void send_command_switch(bool value)
{
    packet[2] = value ? POWER_ON : POWER_OFF;
    packet[3] = value ? 0x64 : 0x00;
    packet[4] = packet[1] + packet[2] + packet[3]; // checksum
    // TRACE_I("SENDING COMMANDDDDDDDDDDDDDDDDDDDDDDDDDDDD SWITCH %X %X %X %X %X %d",packet[0],packet[1],packet[2],packet[3],packet[4],dimmer_uart_object_handle->ezlopi_uart.channel);
    uart_write_bytes(dimmer_uart_object_handle->ezlopi_uart.channel, (const char *)packet, sizeof(packet));
    ++packet[1];
}

bool dimmer_init(s_ezlopi_uart_t *dimmer)
{
    if (dimmer->enable)
    {
        dimmer_uart_object_handle = ezlopi_uart_init(dimmer->baudrate, dimmer->tx, dimmer->rx, dimmer_callback, NULL);
        TRACE_I("I2C CONFIGURATIONSSSSSSSSSSSSSS %d %d %d %d",dimmer->baudrate, dimmer->tx, dimmer->rx, dimmer_uart_object_handle);
        return dimmer_uart_object_handle;
    }

    return false;
}