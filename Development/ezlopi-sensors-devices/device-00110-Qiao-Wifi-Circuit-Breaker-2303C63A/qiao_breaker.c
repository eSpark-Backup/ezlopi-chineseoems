#include "qiao_breaker.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_device_value_updated.h"
#include "freertos/FreeRTOS.h"


#define HEADER_H        0x55
#define HEADER_L        0xAA

#define LOCK_S          0x67
#define MODE            0x68
#define SWITCH          0x01


void lock(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
uint8_t disconnect[] = {0x55, 0xAA , 0x03, 0x05, 0x00, 0x01, 0x00, 0x08};

values_t Values;
s_ezlopi_uart_object_handle_t qiao_uart_object_handle;


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


bool qiao_switch_value() {
    return Values.switch_value;
}

bool qiao_mode_value() {
    return Values.mode_value;
}

bool qiao_lock_value() {
    return Values.lock_value;
}



void lock_update(uint8_t value)
{
    Values.lock_value = value;
}


void qiao_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle) 
{
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L) {
        l_ezlopi_device_t* device = EZPI_core_device_get_head();
        if(device) {
            int ret = 1;
            for (int i = 2; i < 8; i++)
            {    
                if (!(disconnect[i] == buffer[i])) {
                    ret = 0;
                }
            }
            if (ret)
            {
                TRACE_I("Reboot");
                // uint8_t packet[] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x00, 0x03};
                // uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, sizeof(packet));
                if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                }
                if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY));
                }
            }
            l_ezlopi_item_t* switch_item = device->items;
            l_ezlopi_item_t* mode_item = device->next->items;
            if (switch_item)
            {
                if (buffer[6] == SWITCH)
                {
                    lock(buffer, output_len, uart_object_handle);
                    EZPI_core_device_value_updated_from_device_broadcast(switch_item);
                }
                
            }
            if (mode_item)
            {
                if (buffer[6] == MODE)
                {
                    TRACE_I("mode q nhi araha");
                    mode(buffer, output_len, uart_object_handle);
                    EZPI_core_device_value_updated_from_device_broadcast(mode_item);
                }
            }
            
        }
    }
}

bool qiao_init(s_ezlopi_uart_t *qiao_uart_config)
{
    if (qiao_uart_config->enable)
    {
        qiao_uart_object_handle = EZPI_hal_uart_init(qiao_uart_config->baudrate, qiao_uart_config->tx, qiao_uart_config->rx, qiao_callback, NULL);
        paired();
        return qiao_uart_object_handle;
    }

    return false;
}

void paired() {
    vTaskDelay(12 / portTICK_RATE_MS);
    uint8_t packet[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05};
    uint8_t length = 8;
    uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
    packet[6] = 0x04; 
    packet[7] = 0x07;
    length = 8;
    uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
}

void send_command(int value) {
    uint8_t length = 12;    
    uint8_t packet[12] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x05, 0x67, 0x01, 0x00, 0x01, value+0x00, value+0x73};

    uart_write_bytes(qiao_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}