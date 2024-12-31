#include "xuguang_dimmer.h"
#include "ezlopi_core_device_value_updated.h"

#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint16_t)(src)[0] << 8) | \
              ((uint16_t)(src)[1]))


#define HEADER_H        0x55
#define HEADER_L        0xAA

#define POWER           0x05
#define INTENSITY       0x08

#define IN_MIN          0x0A
#define IN_MAX          0x03E8
#define OUT_MIN         0x01
#define OUT_MAX         0x64

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void intensity(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct values
{
    bool power_value;
    uint16_t intensity_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t xuguang_uart_object_handle;

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(buffer)
    {
        Values.power_value = buffer[10];
    }
}

void intensity(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        COPY_AND_SWAP32(Values.intensity_value, &buffer[24]);
        Values.intensity_value = map(Values.intensity_value, IN_MIN, IN_MAX, OUT_MIN, OUT_MAX);
    }
}

bool xuguang_power_value()
{
    TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.power_value;
}

uint16_t xuguang_amb_temp_value()
{
    TRACE_I("----------BRIGHTNESS----------: %d",(Values.intensity_value));
    return (Values.intensity_value);
}

// void checksum()
// {
//     for (int i = 1; i < *length; i++) 
//     {
//         packet[14] += packet[i-1]; 
//     }
//     packet[14] +=1;
// }

void create_packet(uint8_t cmd, uint8_t *packet, uint8_t *length, int value)
{
    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x00;

    switch (cmd)
    {
        case CMD_PWR:
            //55 AA 00 06 00 05 01 01 00 01 00 0D
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            for (int i = 1; i < *length; i++) 
            {
                packet[11] += packet[i-1]; 
            }
        break;
        case CMD_INTENSITY:
            //55 AA 00 06 00 08 02 02 00 04 00 00 03 E8 00
            *length = 15;
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = 0x02; packet[7] = 0x02;
            packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
            
            // packet[14] = 0x1F;
            for (int i = 1; i < *length; i++) 
            {
                packet[14] += packet[i-1]; 
            }
        break;
    // case CMD_WIFI:
    //     packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x05; packet[7] = 0x08;
    //     *length = 8;
    //     break;
    default:
        TRACE_E("Unknown command type");
        break;
    }
}

void xuguang_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
    {
        if((buffer[5]) == POWER)
        {
            power(buffer, output_len, uart_object_handle);
        }
        if((buffer[17]) == INTENSITY)
        {
            intensity(buffer, output_len, uart_object_handle);
        }
        l_ezlopi_device_t* device = ezlopi_device_get_head();
        if(device)
        {
            l_ezlopi_item_t* dimmer_item = device->items;
            l_ezlopi_item_t* switch_item = dimmer_item->next;
            if(dimmer_item && switch_item)
            {
                ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
                ezlopi_device_value_updated_from_device_broadcast(switch_item);
            }
        }
    }
    else{
    }
}

bool xuguang_init(s_ezlopi_uart_t *xuguang_uart_config)
{
    if (xuguang_uart_config->enable)
    {
        xuguang_uart_object_handle = ezlopi_uart_init(xuguang_uart_config->baudrate, xuguang_uart_config->tx, xuguang_uart_config->rx, xuguang_callback, NULL);
        return xuguang_uart_object_handle;
    }

    return false;
}

void send_command(uint8_t cmd, int value) {
    uint8_t packet[15];
    uint8_t length;
    TRACE_I("Now I'm HERE");
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(xuguang_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

    TRACE_I("Command sent:");
    for (int i = 0; i < length; i++) {
        TRACE_I("%02X ", packet[i]);
    }
}

// void send_temp_sp(float value,CommandType cmd) {
//     uint8_t packet[15];
//     uint8_t length;

//     create_packet(cmd, packet, &length, value);
//     uart_write_bytes(bht003_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

//     ESP_LOGI(TAG, "Command sent:");
//     for (int i = 0; i < length; i++) {
//         TRACE_I("%02X ", packet[i]);
//     }
// }