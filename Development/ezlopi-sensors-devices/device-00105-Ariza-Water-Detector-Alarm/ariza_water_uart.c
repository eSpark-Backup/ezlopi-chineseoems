#include "ariza_water_uart.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_device_value_updated.h"

#define HEADER_H          0x55
#define HEADER_L          0xAA
#define ALARM             0x0C
#define BATTERY           0x0F

#define NVS_NAMESPACE "ariza-battery"
#define NVS_KEY "battery-value"

nvs_handle_t battery_nvs_handle = NULL;
static void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct command {
    uint16_t command;
    void (*callback)();
}command_t;

static command_t start_cmd[] = {
    {.command = HEADER_H, .callback = normal_packet},
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

void send_packet(uint8_t* packet) {
    if(packet)
    {
        int len = uart_write_bytes(water_alarm_uart_object_handle->ezlopi_uart.channel, (const char *)packet, 8);
        if (len > 0)
        {
            TRACE_I("Pairing Packet sent");
        }
        else
        {
            TRACE_I("Failed to send packet");
        }
    }
}

bool wifi_connection_status()
{
    ezlopi_wifi_status_t* wifi_con= EZPI_core_wifi_status();
    return wifi_con->wifi_connection;
}

void battery_nvs_read()
{
    uint8_t battery_val = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &battery_nvs_handle) == ESP_OK)
    {
        if (nvs_get_u8(battery_nvs_handle, NVS_KEY, &battery_val) == ESP_OK)
        {
            TRACE_D("Battery Value from NVS: %d", battery_val);
            Values.battery_value = battery_val;
        }
        else{
            TRACE_D("Unable to read from NVS.");
        }
    }
    else{
        TRACE_D("Unable to open NVS.");
    }
    nvs_close(battery_nvs_handle);
}

static void battery_nvs_save(uint8_t bat_value)
{
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &battery_nvs_handle) == ESP_OK)
    {
        uint8_t battery_val = 0;
        if (nvs_get_u8(battery_nvs_handle, NVS_KEY, &battery_val) == ESP_OK)
        {
            TRACE_D("Battery Value from NVS: %d", battery_val);
        }
        else
        {
            TRACE_E("Failed to read battery value from NVS!");
        }
        if(battery_val != bat_value)
        {
            TRACE_D("Battery Value to be saved: %d", bat_value);
            if(nvs_set_u8(battery_nvs_handle, NVS_KEY, bat_value) == ESP_OK)
            {
                TRACE_D("Value saved: %d", bat_value);
            }
            else
            {
                TRACE_D("Value not saved: %d", bat_value);
            }
            nvs_commit(battery_nvs_handle);
        }
        else
        {
            TRACE_I("No CHANGE IN THE VALUE!");
        }
        nvs_close(battery_nvs_handle);
    }   
    else
    {
        TRACE_E("Failed to open NVS!");
    }
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
        if(item && item->next && item->next->next)
        {
            EZPI_core_device_value_updated_from_device_broadcast(item);
            EZPI_core_device_value_updated_from_device_broadcast(item->next->next);
        }
        TRACE_I("----------ALARM VALUEEEEEEE----------%X",Values.alarm_value);
    }
    else if(buffer[5] == BATTERY)
    {
        Values.battery_value = buffer[20];
        if(item && item->next)
        {
            EZPI_core_device_value_updated_from_device_broadcast(item->next);
        }
        battery_nvs_save(Values.battery_value);
        TRACE_I("----------BATTERY VALUEEEEEEE----------%X",Values.battery_value);
    }
}

void water_alarm_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(uart_object_handle && buffer)
    {
        if((buffer[0]) == (start_cmd[0].command) && (buffer[1] == HEADER_L))
        {
            TRACE_I("----------HEADER----------%X",buffer[0]);
            if(buffer[10] == (0x04))
            {
                TRACE_I("PAIRING OR NO PAIRING");
                if(buffer[13] == 0x00)
                {
                    TRACE_I("PAIRINGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
                }
                else if(buffer[13] == 0x01)
                {
                    TRACE_I("RESET");
                }
            }
            else{
                start_cmd[0].callback(buffer, output_len, uart_object_handle);
            }
        }
        else {}
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
            water_alarm_uart_object_handle = EZPI_hal_uart_init(item->interface.uart.baudrate, item->interface.uart.tx, item->interface.uart.rx, water_alarm_callback, (void *)item);
            item->interface.uart.channel = water_alarm_uart_object_handle->ezlopi_uart.channel;
            return water_alarm_uart_object_handle;
        }
    }

    return false;
}