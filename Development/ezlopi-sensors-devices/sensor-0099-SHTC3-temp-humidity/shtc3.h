#ifndef __SHTC3_H__
#define __SHTC3_H__

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ezlopi_hal_i2c_master.h"

//CS =  CLOCK_STRETCHING
//LP = LOW POWER
//N = NORMAL
#define T_FIRST_N_CS 0x7CA2
#define RH_FIRST_N_CS 0x5C24
#define T_FIRST_LP_CS 0x6458
#define RH_FIRST_LP_CS 0x44DE

#define T_FIRST_N 0x7866
#define RH_FIRST_N 0x58E0
#define T_FIRST_LP 0x609C
#define RH_FIRST_LP 0x401A

#define I2C_MASTER_FREQ_100KHZ 100000
#define SHTC3_ADDR 0x70
void shtc3_init_sensor(s_ezlopi_i2c_master_t *i2c_master_conf);
// esp_err_t wakeup_sensor(uint8_t addr);
esp_err_t shtc3_wakeup_sensor(s_ezlopi_i2c_master_t *i2c_master_conf);
// esp_err_t read_out(uint8_t addr, uint16_t read_mode, float *temp, float *hum);
esp_err_t shtc3_read_out(s_ezlopi_i2c_master_t *i2c_master_conf, float *temp, float *hum);
esp_err_t shtc3_sleep_sensor(s_ezlopi_i2c_master_t *i2c_master_conf);
esp_err_t get_ID_sensor(uint8_t addr);

#endif//__SHTC3_H__