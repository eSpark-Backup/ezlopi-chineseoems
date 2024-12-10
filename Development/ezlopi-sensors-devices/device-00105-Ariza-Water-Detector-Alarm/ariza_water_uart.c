#include "ariza_water_uart.h"
#include "ezlopi_core_device_value_updated.h"

#define HEADER          0x55
#define ALARM           0x0C
#define BATTERY         0x0F

#define BUF_SIZE        1024

uint8_t network_status[] = {0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02};
uint8_t config_done[] = {0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x04, 0x06};

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

void send_packet(l_ezlopi_item_t* item) {
    int len = uart_write_bytes(item->interface.uart.channel, (const char *)network_status, sizeof(network_status));
    if (len > 0) {
        printf("Packet Sent: ");
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", network_status[i]);
        }
        printf("\n");
    } else {
        TRACE_I("Failed to send packet");
    }
}

static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    l_ezlopi_item_t* item = NULL;
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    TRACE_I("----------PACKET VALUEEEEEEE----------%X",buffer[5]);
    if(uart_object_handle)
    {
        item = (l_ezlopi_item_t*) uart_object_handle->arg;
    }
    for (size_t i = 0; i < 3; i++)
    {
        send_packet(item);
        int len = uart_read_bytes(item->interface.uart.channel, data, BUF_SIZE, 500 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            printf("Response Received: ");
            for (int i = 0; i < len; i++)
            {
                printf("0x%02X ", data[i]);
            }
        } else {
            TRACE_I("No response received");
        }
    }
    if(buffer[5] == ALARM)
    {
        Values.alarm_value = !buffer[17];
        if(item && item->next && item->next->next)
        {
            ezlopi_device_value_updated_from_device_broadcast(item);
            ezlopi_device_value_updated_from_device_broadcast(item->next->next);
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