#include "breeze_thermostat.h"
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
#define POWER           0x01
#define FAN             0x02
#define LOCK            0x28
#define FORMAT          0x17
#define PIR             0x67
#define UP_LIMIT_C      0x13
#define UP_LIMIT_F      0x12
#define LOW_LIMIT_C     0x1A                
#define LOW_LIMIT_F     0x14   
#define MODE            0x40             


#define IN_MIN          0x000A
#define IN_MAX          0x0073
#define OUT_MIN         0x0032
#define OUT_MAX         0x01F4
#define COMMAND         0x07

static uint64_t start_time = 0;

#define FORMAT_VALUE    0

void power(uint8_t* buffer);
void fan(uint8_t* buffer);
void lock(uint8_t* buffer);
void format(uint8_t* buffer);
void c_lower(uint8_t* buffer);
void f_lower(uint8_t* buffer);
void c_upper(uint8_t* buffer);
void f_upper(uint8_t* buffer);




typedef struct values
{
    bool lock_value;
    bool power_value;
    uint32_t c_lower_value;
    uint32_t f_lower_value;
    uint32_t c_upper_value;
    uint32_t f_upper_value;
    bool format_value;
    bool fan_value;
}values_t;

values_t Values;
// Values.temp_value = 32;


int map(int x, int in_min, int in_max, int out_min, int out_max) {
    // return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return out_min + (out_max) * (x - in_min) / (in_max - in_min);
}

void power(uint8_t* buffer)
{ 
    if(buffer)
    {
        Values.power_value = buffer[10];
    }
}

void fan(uint8_t* buffer)
{ 
    if(buffer)
    {
        Values.fan_value = buffer[10];
    }
}

void lock(uint8_t* buffer)
{
    if (buffer)
    {
        Values.lock_value = buffer[10];
    }
}

void format(uint8_t* buffer)
{
    if (buffer)
    {
        Values.format_value = buffer[10];
    }
}

void c_lower(uint8_t* buffer)
{
    if (buffer)
    {
        Values.c_lower_value = 0;
        Values.c_lower_value = ((buffer[27]<<8) | (buffer[28])) / 10;
    }
}

void f_lower(uint8_t* buffer)
{
    if (buffer)
    {
        Values.f_lower_value = 0;
        Values.f_lower_value = ((buffer[12]<<8) | (buffer[13])) / 10;
    }
}

void c_upper(uint8_t* buffer)
{
    if (buffer)
    {
        Values.c_upper_value = 0;
        Values.c_upper_value = ((buffer[27]<<8) | (buffer[28])) / 10;
    }
}

void f_upper(uint8_t* buffer)
{
    if (buffer)
    {
        Values.f_upper_value = 0;
        Values.f_upper_value = ((buffer[12]<<8) | (buffer[13])) / 10;
    }
}

void power_update(bool value)
{
    Values.power_value = value;
}

void fan_update(bool value)
{
    Values.fan_value = value;
}

void lock_update(uint8_t value)
{
    Values.lock_value = value;
}

void format_update(bool value)
{
    Values.format_value = value;
}

bool breeze_power_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.power_value;
}

bool breeze_fan_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.fan_value;
}

bool breeze_format_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.format_value;
}

bool lock_value() {
    return Values.lock_value;
}

uint32_t upper_value() {
    return Values.format_value?Values.f_upper_value:Values.c_upper_value;
    // return Values.f_upper_value;
}

uint32_t lower_value() {
    return Values.format_value?Values.f_lower_value:Values.c_lower_value;
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
    int temp;

    switch (cmd)
    {
        case CMD_PWR:
            //55 AA 00 06 00 05 01 01 00 01 00 0D 
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = POWER; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_FAN:
            //55 AA 00 06 00 05 02 04 00 01 00 11
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = FAN; packet[7] = 0x04;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_LOCK:
            //55 AA 00 06 00 05 28 01 00 01 00 34
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = LOCK; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_FORMAT:
            //55 AA 00 06 00 05 17 04 00 01 00 26
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = FORMAT; packet[7] = 0x04;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_PIR:
            //55 AA 00 06 00 05 67 04 00 01 00 76
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = PIR; packet[7] = 0x04;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = value;
            *length = 12;
            packet[11] = checksum(packet, *length);
        break;
        case CMD_UP_LIMIT:
            //55 AA 00 06 00 08 13 02 00 04 00 00 01 F4 1B
            *length = 15;
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = (UP_LIMIT_C-Values.format_value); packet[7] = 0x02;
            packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
            // temp = map(value, OUT_MIN, OUT_MAX, IN_MIN, IN_MAX);
            // packet[12] = (temp >> 8) & 0xFF; packet[13] = temp & 0xFF;
            packet[12] = (value >> 8) & 0xFF; packet[13] = value & 0xFF;
            packet[14] = checksum(packet, *length);
            TRACE_I("-------------------------------------------------Packet: %x %x",packet[12],packet[13]);
            break;
            case CMD_LOW_LIMIT:
            //55 AA 00 06 00 08 1A 02 00 04 00 00 01 B8 E6
            *length = 15;
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = (LOW_LIMIT_C-Values.format_value); packet[7] = 0x02;
            packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
            // temp = map(value, OUT_MIN, OUT_MAX, IN_MIN, IN_MAX);
            // packet[12] = (temp >> 8) & 0xFF; packet[13] = temp & 0xFF;
            packet[12] = (value >> 8) & 0xFF; packet[13] = value & 0xFF;
            packet[14] = checksum(packet, *length);
            TRACE_I("--------------------------------------------------Packet: %x %x",packet[12],packet[13]);
        break;
        case CMD_MODE:
            //55 AA 00 06 00 05 40 04 00 01 00 4F
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x40; packet[7] = 0x04; 
            packet[8] = 0x00; packet[9] = 0x01; packet[10] =  value;
            *length = 12;
            packet[11] = checksum(packet, *length);
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
        case CMD_HEARTBEAT:
            //55 aa 00 00 00 00 ff
            packet[3] = 0x00; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0xFF;
            *length = 7;
        break;
        default:
            TRACE_E("Unknown command type");
        break;
    }
}

void breeze_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    // if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
    // {
    //     l_ezlopi_device_t* device = EZPI_core_device_get_head();
    //     if(device)
    //     {
    //         l_ezlopi_item_t* dimmer_item = device->items;
    //         l_ezlopi_item_t* switch_item = dimmer_item->next;
    //         if(dimmer_item && switch_item)
    //         {
    //             if((buffer[5]) == POWER && (buffer[17]) == INTENSITY)
    //             {
    //                 if(esp_timer_get_time()-start_time < (2.5*1000*1000))
    //                 {
    //                     send_command(CMD_PAIR, 1);
    //                     send_command(CMD_RESET, 1);
    //                     cJSON *cj_data = cJSON_CreateObject(__FUNCTION__);
    //                     if (cj_data)
    //                     {
    //                         cJSON_AddStringToObject(__FUNCTION__, cj_data, "method", "hub.firmware.update.start");
    //                         cJSON_AddStringToObject(__FUNCTION__, cj_data, "id", "_ID_");
    //                         cJSON *params = cJSON_CreateObject(__FUNCTION__);
    //                         if (params)
    //                         {
    //                             cJSON_AddItemToObject(__FUNCTION__, cj_data, "params", params);
    //                             if(__send_cjson_data_to_nma_websocket(cj_data) == EZPI_SUCCESS)
    //                             {
    //                                 TRACE_E("OTA METHOD SENT");
    //                             }
    //                             else{
    //                                 TRACE_E("OTA METHOD NOT SENT");
    //                             }
    //                         }
    //                         cJSON_Delete(__FUNCTION__, cj_data);
    //                     }
    //                 }
    //                 else{
    //                     start_time = esp_timer_get_time();
    //                 }
    //                 power(buffer, output_len, uart_object_handle);
    //                 intensity(buffer, output_len, uart_object_handle);
    //                 EZPI_core_device_value_updated_from_device_broadcast(dimmer_item);
    //                 EZPI_core_device_value_updated_from_device_broadcast(switch_item);
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
    //                 send_command(CMD_RESET, 1);
    //                 esp_restart();
    //             }
    //         }
    //     }
    // }
    // else{
    // }
        // for (int i=0;i<output_len;i++) {
        //     TRACE_I("%x",buffer[i]);
        // }
        // TRACE_I("Output_len: %d",output_len);
        // TRACE_I("HELLOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
    {
        TRACE_I("--------------------------recieve krlo bhai");
        l_ezlopi_device_t* device = EZPI_core_device_get_head();
        if(device)
        {
            l_ezlopi_item_t* power_item = device->items;
            if (power_item == NULL)
            {
                TRACE_I("Power NULL");
            }
            l_ezlopi_item_t* fan_item = device->next->items;
            if (fan_item == NULL)
            {
                TRACE_I("fan_item NULL");
            }
            l_ezlopi_item_t* lock_item = device->next->next->items;
            if (lock_item == NULL)
            {
                TRACE_I("lock_item NULL");
            }
            l_ezlopi_item_t* format_item = device->next->next->next->items;
            if (format_item == NULL)
            {
                TRACE_I("format_item NULL");
            }
            l_ezlopi_item_t* pir_item = device->next->next->next->next->items;
            if (pir_item == NULL)
            {
                TRACE_I("pir_item NULL");
            }
            l_ezlopi_item_t* upper_item = device->next->next->next->next->next->items;
            if (upper_item == NULL)
            {
                TRACE_I("upper_item NULL");
            }
            l_ezlopi_item_t* lower_item = device->next->next->next->next->next->next->items;
            if (lower_item == NULL)
            {
                TRACE_I("lower_item NULL");
            }
            l_ezlopi_item_t* mode_item = device->next->next->next->next->next->next->next->items;
            if (lower_item == NULL)
            {
                TRACE_I("mode_item NULL");
            }
            if(power_item && fan_item && lock_item && format_item && pir_item && upper_item && lower_item && mode_item && buffer[3] == COMMAND)
            {
                if ((buffer[6] == POWER))
                {
                    // 55 AA 03 07 00 05 01 01 00 01 00 11
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    TRACE_I("-----------------------------------Paawaaar: %d", buffer[10]);
                    power(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(power_item);
                }
                else if (buffer[6] == FAN)
                {
                    // 55 AA 03 07 00 05 02 04 00 01 00 15
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    fan(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(fan_item);
                }
                else if (buffer[6] == LOCK)
                {
                    // 55 AA 03 07 00 05 28 01 00 01 00 38
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    lock(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(lock_item);
                }
                else if (buffer[6] == FORMAT)
                {
                    // 55 AA 03 07 00 05 17 04 00 01 00 2A
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(format_item);
                }
                else if ((buffer[21] == LOW_LIMIT_C) && (Values.format_value == 0))
                {
                    // 55 AA 03 07 00 08 1A 02 00 04 00 00 00 32 63
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    c_lower(buffer);
                    TRACE_I("-------------------------------------------c_lower");
                    // format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(lower_item);
                }
                else if ((buffer[21] == UP_LIMIT_C) && (Values.format_value == 0))
                {
                    // 55 AA 03 07 00 08 13 02 00 04 00 00 01 C2 ED
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    c_upper(buffer);
                    TRACE_I("-------------------------------------------c _upper");
                    // format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(upper_item);
                }
                else if ((buffer[6] == LOW_LIMIT_F) && (Values.format_value == 1))
                {
                    // 55 AA 03 07 00 08 1A 02 00 04 00 00 00 32 63
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    f_lower(buffer);
                    TRACE_I("-------------------------------------------f_lower");
                    // format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(lower_item);
                }
                else if ((buffer[6] == UP_LIMIT_F) && (Values.format_value == 1))
                {
                    // 55 AA 03 07 00 08 13 02 00 04 00 00 01 C2 ED
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    f_upper(buffer);
                    TRACE_I("-------------------------------------------f _upper");
                    // format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(upper_item);
                }
                else if ((buffer[6] == MODE) )
                {
                    // 55 AA 03 07 00 08 13 02 00 04 00 00 01 C2 ED
                    for (int i=0;i<output_len;i++) {
                        TRACE_I("%x",buffer[i]);
                    }
                    // f_upper(buffer);
                    // TRACE_I("-------------------------------------------f _upper");
                    // format(buffer);
                    EZPI_core_device_value_updated_from_device_broadcast(mode_item);
                }
            }
            if ((buffer[3] == 0x22) && (buffer[6] == FORMAT))
            {
                if (buffer[10])
                {
                    TRACE_I("-------------------------------------------HOGAYAAA!!");
                }
                else
                {
                    TRACE_I("-------------------------------------------NHII HOAA!!");
                }
                
                
            }
            
        }
    }
}

bool breeze_init(s_ezlopi_uart_t *breeze_uart_config)
{
    if (breeze_uart_config->enable)
    {
        breeze_uart_object_handle = EZPI_hal_uart_init(breeze_uart_config->baudrate, breeze_uart_config->tx, breeze_uart_config->rx, breeze_callback, NULL);
        return breeze_uart_object_handle;
    }

    return false;
}

void send_command(uint8_t cmd, int value) {
    uint8_t packet[15];
    uint8_t length = 0;
    memset(packet, 0, sizeof(packet));
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(breeze_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

    // vTaskDelay(50 / portTICK_PERIOD_MS);
    // TRACE_I("Command sent:");
    // for (int i = 0; i < length; i++) {
    //     TRACE_I("%02X ", packet[i]);
    // }
}

