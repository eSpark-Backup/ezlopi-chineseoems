#include "Moes_dimmer.h"
#include "ezlopi_core_ota.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_device_value_updated.h"

#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint16_t)(src)[0] << 8) | \
              ((uint16_t)(src)[1]))


#define HEADER_H        0x55
#define HEADER_L        0xAA

#define WIFI            0x04
#define POWER           0x05
#define INTENSITY       0x08
#define RESET           0x05

#define IN_MIN          0x0A
#define IN_MAX          0x03E8
#define OUT_MIN         0x01
#define OUT_MAX         0x64

static uint64_t start_time = 0;

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void intensity(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct values
{
    bool power_value;
    uint16_t intensity_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t Moes_uart_object_handle;

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
        COPY_AND_SWAP32(Values.intensity_value, &buffer[12]);
        Values.intensity_value = map(Values.intensity_value, IN_MIN, IN_MAX, OUT_MIN, OUT_MAX);
    }
}
void power_update(bool value)
{
    Values.power_value = value;
}

void intensity_update(uint8_t value)
{
    Values.intensity_value = value;
}

bool Moes_power_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.power_value;
}

uint16_t Moes_amb_temp_value()
{
    // TRACE_I("----------BRIGHTNESS----------: %d",(Values.intensity_value));
    return (Values.intensity_value);
}

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
            //55 aa 00 05 00 00 04
            packet[3] = 0x05; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0x04;
            *length = 7;
        break;
        default:
            TRACE_E("Unknown command type");
        break;
    }
}

void Moes_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
    {
        l_ezlopi_device_t *device = EZPI_core_device_get_head();
        if (device)
        {
            l_ezlopi_item_t *dimmer_item = device->items;
            l_ezlopi_item_t *switch_item = dimmer_item->next;
            if (switch_item) 
            {
                if (buffer[5] == POWER)
                {
                    if (esp_timer_get_time() - start_time < (2 * 1000 * 1000))
                    {
                        cJSON *cj_data = cJSON_CreateObject(__FUNCTION__);
                        if (cj_data)
                        {
                            cJSON_AddStringToObject(__FUNCTION__, cj_data, "method", "hub.firmware.update.start");
                            cJSON_AddStringToObject(__FUNCTION__, cj_data, "id", "_ID_");
                            cJSON *params = cJSON_CreateObject(__FUNCTION__);
                            if (params)
                            {
                                cJSON_AddItemToObject(__FUNCTION__, cj_data, "params", params);
                                if (__send_cjson_data_to_nma_websocket(cj_data) == EZPI_SUCCESS)
                                {
                                    TRACE_E("OTA METHOD SENT");
                                }
                                else
                                {
                                    TRACE_E("OTA METHOD NOT SENT");
                                }
                            }
                            cJSON_Delete(__FUNCTION__, cj_data);
                        }
                        // send_command(CMD_RESET, 1);
                        // send_command(CMD_PAIR,1);
                    }
                    else
                    {
                        start_time = esp_timer_get_time();
                    }
                    power(buffer, output_len, uart_object_handle);
                    EZPI_core_device_value_updated_from_device_broadcast(switch_item);
                    EZPI_core_device_value_updated_from_device_broadcast(dimmer_item);
                }
            }

            if (dimmer_item)
            {
                if (buffer[5] == INTENSITY)
                {
                    intensity(buffer, output_len, uart_object_handle);
                    EZPI_core_device_value_updated_from_device_broadcast(dimmer_item);
                    EZPI_core_device_value_updated_from_device_broadcast(switch_item);
                }
            }
            if (buffer[3] == RESET)
            {
                if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                }
                if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY));
                }
                send_command(CMD_RESET, 1);
                send_command(CMD_PAIR,1);
                TRACE_I("-----------------------REBOOT!");
                esp_restart();
            }

        }
    }
}

bool Moes_init(s_ezlopi_uart_t *Moes_uart_config)
{
    if (Moes_uart_config->enable)
    {
        Moes_uart_object_handle = EZPI_hal_uart_init(Moes_uart_config->baudrate, Moes_uart_config->tx, Moes_uart_config->rx, Moes_callback, NULL);
        return Moes_uart_object_handle;
    }

    return false;
}

void send_command(uint8_t cmd, int value) {
    uint8_t packet[15];
    uint8_t length = 0;
    memset(packet, 0, sizeof(packet));
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(Moes_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
    
}
