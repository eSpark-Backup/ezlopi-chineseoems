#include "dimmer.h"
#include "ezlopi_util_trace.h"

uint8_t packet[5] = {HEADER,0,0,BRIGHTNESS,0};
uint8_t saved_value = 0;
s_ezlopi_uart_object_handle_t dimmer_uart_object_handle;

void dimmer_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    
}

void update_packet(uint8_t value)
{
    saved_value = value;
}

uint8_t fetch_packet()
{
    return saved_value;
}

void send_command_dimmer(int value)
{
    packet[2] = POWER_ON;
    packet[3] = value;
    update_packet(value);
    packet[4] = packet[1] + packet[2] + packet[3]; // checksum
    TRACE_I("SENDING COMMANDDDDDDDDDDDDDDDDDDDDDDDDDDDD DIMMER");
    uart_write_bytes(dimmer_uart_object_handle->ezlopi_uart.channel, (const char *)packet,sizeof(packet));
    ++packet[1];
}

void send_command_switch(bool value)
{
    TRACE_I("PACKET VALUEEEEEEEEEEEE: %d",value);
    packet[2] = value ? POWER_ON : POWER_OFF;
    packet[3] = value ? saved_value : 0x00;
    // update_packet(value);
    packet[4] = packet[1] + packet[2] + packet[3]; // checksum
    TRACE_I("SENDING COMMANDDDDDDDDDDDDDDDDDDDDDDDDDDDD SWITCH %X %X %X %X %X",packet[0],packet[1],packet[2],packet[3],packet[4]);
    uart_write_bytes(dimmer_uart_object_handle->ezlopi_uart.channel, (const char *)packet, sizeof(packet));
    ++packet[1];
}

bool dimmer_init(s_ezlopi_uart_t *dimmer)
{
    if (dimmer->enable)
    {
        dimmer_uart_object_handle = EZPI_hal_uart_init(dimmer->baudrate, dimmer->tx, dimmer->rx, dimmer_callback, NULL);
        TRACE_I("I2C CONFIGURATIONSSSSSSSSSSSSSS %d %d %d %d",dimmer->baudrate, dimmer->tx, dimmer->rx, dimmer_uart_object_handle);
        return dimmer_uart_object_handle;
    }
    return false;
}