#include "ariza_water_uart.h"
#include "ezlopi_core_device_value_updated.h"

#define HEADER          0x55
#define ALARM           0x0C
#define BATTERY         0x0F

static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct command {
    uint16_t command;
    void (*callback)();
}command_t;

static command_t start_cmd[] = {
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

static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    l_ezlopi_item_t* item = NULL;
    TRACE_I("----------PACKET VALUEEEEEEE----------%X",buffer[5]);
    if(uart_object_handle)
    {
        item = (l_ezlopi_item_t*) uart_object_handle->arg;
    }
    if(buffer[5] == ALARM)
    {
        Values.alarm_value = !buffer[17];
        if(item)
        {
            ezlopi_device_value_updated_from_device_broadcast(item);
        }
        TRACE_I("----------ALARM VALUEEEEEEE----------%X",Values.alarm_value);
    }
    else if(buffer[5] == BATTERY)
    {
        Values.battery_value = buffer[20];
        if(item && item->next)
        {
            ezlopi_device_value_updated_from_device_broadcast(item->next);
        }
        TRACE_I("----------BATTERY VALUEEEEEEE----------%X",Values.battery_value);
    }
}

void water_alarm_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(uart_object_handle && buffer)
    {
        if((buffer[0]) == (start_cmd[0].command))
        {
            TRACE_I("----------HEADER----------%X",buffer[0]);
            start_cmd[0].callback(buffer, output_len, uart_object_handle);
        }
        else{
        }
    }
    else{
        TRACE_I("----------OOOOOOOOOOPS----------");
    }
}

bool water_alarm_init(l_ezlopi_item_t* item)
{
    if(item)
    {
        if (item->interface.uart.enable)
        {
            water_alarm_uart_object_handle = ezlopi_uart_init(item->interface.uart.baudrate, item->interface.uart.tx, item->interface.uart.rx, water_alarm_callback, (void *)item);
            return water_alarm_uart_object_handle;
        }
    }

    return false;
}