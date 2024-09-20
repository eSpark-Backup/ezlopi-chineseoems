#include "shtc3.h"

#define I2C_MASTER_TIMEOUT_MS 1000

uint8_t calc_crc8(uint8_t *data, size_t len);
static const char *TAG = "SHCT";

void shtc3_init_sensor(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    ezlopi_i2c_master_init(i2c_master_conf);
}

esp_err_t shtc3_wakeup_sensor(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    uint8_t data[2] = {0x35, 0x17};  // Data to be sent
    esp_err_t err = ezlopi_i2c_master_write_to_device(i2c_master_conf, data, sizeof(data));

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "I2C Command failed: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t shtc3_read_out(s_ezlopi_i2c_master_t *i2c_master_conf, float *temp, float *hum)
{
    esp_err_t err = 0;
    uint8_t write_buffer[2] = {0x78, 0x66};
    uint8_t read_buffer[6] = {0};

    vTaskDelay(pdMS_TO_TICKS(10));
    err = ezlopi_i2c_master_write_to_device(i2c_master_conf, write_buffer, sizeof(write_buffer));
    vTaskDelay(pdMS_TO_TICKS(30));
    err = ezlopi_i2c_master_read_from_device(i2c_master_conf, read_buffer, sizeof(read_buffer));

    if (calc_crc8(read_buffer, 2) == read_buffer[2])
    {
        *temp = ((((read_buffer[0] << 8) + read_buffer[1]) * 175) / 65536.0) - 45;
    }
    else
    {
        ESP_LOGI(TAG, "Temperature CRC error");
    }

    if (calc_crc8(read_buffer + 3, 2) == read_buffer[5])
    {
        *hum = ((((read_buffer[3] << 8) + read_buffer[4]) * 100) / 65536.0);
    }
    else
    {
        ESP_LOGI(TAG, "Humidity CRC error");
    }

    return err;
}

esp_err_t shtc3_sleep_sensor(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    uint8_t data[2] = {0xB0, 0x98};  // Data to be sent to put the sensor to sleep
    esp_err_t err = ezlopi_i2c_master_write_to_device(i2c_master_conf, data, sizeof(data));

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to put sensor to sleep: %s", esp_err_to_name(err));
    }

    return err;
}

uint8_t calc_crc8(uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}