#include "qiao_breaker.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_device_value_updated.h"

#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint16_t)(src)[0] << 8) | \
              ((uint16_t)(src)[1]))


#define HEADER_H        0x55
#define HEADER_L        0xAA

#define LOCK_S          0x67
#define MODE            0x68
#define SWITCH          0x01

#define WIFI            0x04
#define POWER           0x05
#define INTENSITY       0x08

#define IN_MIN          0x0A
#define IN_MAX          0x03E8
#define OUT_MIN         0x01
#define OUT_MAX         0x64

void lock(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
// void intensity(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct values
{
    bool switch_value;
    bool mode_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t qiao_uart_object_handle;

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void lock(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(buffer)
    {
        Values.switch_value = buffer[10];
        TRACE_I("-------------------------------%d",Values.switch_value);
    }
}

void mode(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(buffer)
    {
        Values.mode_value = buffer[10];
        TRACE_I("-------------------------------%d",Values.mode_value);
    }
}

// void intensity(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
// {
//     if (buffer)
//     {
//         COPY_AND_SWAP32(Values.intensity_value, &buffer[24]);
//         Values.intensity_value = map(Values.intensity_value, IN_MIN, IN_MAX, OUT_MIN, OUT_MAX);
//     }
// }
// void power_update(bool value)
// {
//     Values.power_value = value;
// }

// void intensity_update(uint8_t value)
// {
//     Values.intensity_value = value;
// }


bool qiao_switch_value() {
    return Values.switch_value;
}

bool qiao_mode_value() {
    return Values.mode_value;
}

// bool xuguang_power_value()
// {
//     // TRACE_I("----------POWER----------: %d",Values.power_value);
//     return Values.power_value;
// }

// uint16_t xuguang_amb_temp_value()
// {
//     // TRACE_I("----------BRIGHTNESS----------: %d",(Values.intensity_value));
//     return (Values.intensity_value);
// }

uint8_t checksum(uint8_t *packet, uint8_t length) {
    uint32_t checksum = 0;
    for (int i = 0; i < length - 1; i++)
    {
        checksum += packet[i];
    }
    return checksum & 0xFF;
}

void create_packet(uint8_t cmd, uint8_t *packet, uint8_t *length, int value)
{
    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x00;

    switch (cmd)
    {
        case CMD_PWR:
            //55 AA 00 06 00 05 01 01 00 01 00 0D
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = POWER; packet[6] = 0x01; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_INTENSITY:
            //55 AA 00 06 00 08 02 02 00 04 00 00 03 E8 00
            *length = 15;
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = INTENSITY; packet[6] = 0x02; packet[7] = 0x02;
            packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
            int temp = map(value, OUT_MIN, OUT_MAX, IN_MIN, IN_MAX);
            packet[12] = (temp >> 8) & 0xFF; packet[13] = temp & 0xFF;
            packet[14] = checksum(packet, *length);
        break;
        case CMD_PAIR:
            //55 aa 00 03 00 01 00 03
            packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x00; packet[7] = 0x03;
            *length = 8;
        break;
        case CMD_PAIRED:
            //55 aa 00 03 00 01 04 07
            packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x04; packet[7] = 0x07;
            *length = 8;
        break;
        case CMD_RESET:
            //55 aa 00 04 00 00 03
            packet[3] = 0x04; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0x03;
            *length = 7;
        break;
        default:
            TRACE_E("Unknown command type");
        break;
    }
}

void qiao_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle) 
{
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L) {
        l_ezlopi_device_t* device = ezlopi_device_get_head();
        if(device) {
            l_ezlopi_item_t* switch_item = device->items;
            l_ezlopi_item_t* mode_item = device->next->items;
            if (switch_item)
            {
                if (buffer[6] == SWITCH)
                {
                    lock(buffer, output_len, uart_object_handle);
                    ezlopi_device_value_updated_from_device_broadcast(switch_item);
                }
                
            }
            if (!(mode_item))
            {
                TRACE_I("mode item not null");
            }
            else
            {
                TRACE_I("mode null");
            }

            if (mode_item)
            {
                if (buffer[6] == MODE)
                {
                    TRACE_I("mode q nhi araha");
                    mode(buffer, output_len, uart_object_handle);
                    ezlopi_device_value_updated_from_device_broadcast(mode_item);
                }
            }
            
        }
    }
}

// void xuguang_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
// {
//     if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
//     {
//         l_ezlopi_device_t* device = ezlopi_device_get_head();
//         if(device)
//         {
//             l_ezlopi_item_t* dimmer_item = device->items;
//             l_ezlopi_item_t* switch_item = dimmer_item->next;
//             if(dimmer_item && switch_item)
//             {
//                 if((buffer[5]) == POWER && (buffer[17]) == INTENSITY)
//                 {
//                     power(buffer, output_len, uart_object_handle);
//                     intensity(buffer, output_len, uart_object_handle);
//                     ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
//                     ezlopi_device_value_updated_from_device_broadcast(switch_item);
//                 }
//             }
//             else if(buffer[3] == WIFI)
//             {
//                 send_command(CMD_PAIR, 1);
//                 if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
//                 {
//                 }
//                 if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
//                 {
//                     if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY));
//                 }
//             }
//         }
//     }
//     else{
//     }
// }

bool qiao_init(s_ezlopi_uart_t *qiao_uart_config)
{
    if (qiao_uart_config->enable)
    {
        qiao_uart_object_handle = ezlopi_uart_init(qiao_uart_config->baudrate, qiao_uart_config->tx, qiao_uart_config->rx, qiao_callback, NULL);
        // uint8_t pairing_mode[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05};
        // uint8_t paired[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x04, 0x07};
        uint8_t packet[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05};
        uint8_t length = 8;
        uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
        packet[6] = 0x04;
        packet[7] = 0x07;
        length = 8;
        uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
        TRACE_I("%d",length);
        return qiao_uart_object_handle;
    }

    return false;
}

void send_command(uint8_t cmd, int value) {
    uint8_t packet[15];
    uint8_t length = 0;
    memset(packet, 0, sizeof(packet));
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
    // vTaskDelay(50 / portTICK_PERIOD_MS);
    // TRACE_I("Command sent:");
    // for (int i = 0; i < length; i++) {
    //     TRACE_I("%02X ", packet[i]);
    // }
}