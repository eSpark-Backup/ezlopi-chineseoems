#ifndef _SENSOR_0010_I2C_BME680_H_
#define _SENSOR_0010_I2C_BME680_H_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

int bme680_sensor_dev_item_prep(s_ezlopi_prep_arg_t *prep_args);
int sensor_0010_I2C_BME680(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // _SENSOR_0010_I2C_BME680_H_