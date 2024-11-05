#include "ariza_water_uart.h"
// #include "esp_log.h" 
/*
#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint32_t)(src)[0] << 24) | \
              ((uint32_t)(src)[1] << 16) | \
              ((uint32_t)(src)[2] << 8)  | \
              ((uint32_t)(src)[3]))
*/
#define START           0x00
#define HEADER          0x55

#define ALARM           0x0C
#define BATTERY         0x0F

static void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle);
static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct command {
    uint16_t command;
    void (*callback)();
}command_t;

static command_t start_cmd[] = {
    {.command = START,  .callback = start_packet},
    {.command = HEADER, .callback = normal_packet},
};

typedef struct values
{
    bool alarm_value;
    uint8_t battery_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t water_alarm_uart_object_handle;

bool get_alarm_value()
{
    return Values.alarm_value;
}

uint8_t get_battery_value()
{
    return Values.battery_value;
}

static void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle)
{
    uart_write_bytes(uart_object_handle->ezlopi_uart.channel, (const char *)START, sizeof(START));
}

static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    TRACE_I("----------PACKET VALUEEEEEEE----------%d",buffer[5]);
    if(buffer[5] == ALARM)
    {
        Values.alarm_value = buffer[20];
        TRACE_I("----------ALARM VALUEEEEEEE----------%d",Values.alarm_value);
    }
    if(buffer[5] == BATTERY)
    {
        Values.battery_value = buffer[20];
        TRACE_I("----------BATTERY VALUEEEEEEE----------%d",Values.battery_value);
    }
}

void water_alarm_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(uart_object_handle && buffer)
    {
        if ((buffer[0]) == (start_cmd[0].command))
        {
            TRACE_I("----------Start----------");
            start_cmd[0].callback(uart_object_handle);
        }
        else if((buffer[0]) == (start_cmd[1].command))
        {
            TRACE_I("----------HEADER----------%X",buffer[0]);
            start_cmd[1].callback(buffer, output_len, uart_object_handle);
        }
        else{
        }
    }
    else{
        TRACE_I("----------OOOOOOOOOOPS----------");
    }
}

bool water_alarm_init(s_ezlopi_uart_t *water_alarm_uart_config)
{
    if (water_alarm_uart_config->enable)
    {
        water_alarm_uart_object_handle = ezlopi_uart_init(water_alarm_uart_config->baudrate, water_alarm_uart_config->tx, water_alarm_uart_config->rx, water_alarm_callback, NULL);
        return water_alarm_uart_object_handle;
    }

    return false;
}