
#ifndef _SENSOR_0098_I2C_TAMPER_SWITCH_H_
#define _SENSOR_0098_I2C_TAMPER_SWITCH_H_

#include "pca9536.h"

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

#define TAMPER_SWITCH_GPIO 0
 
// pca9536-io-expander
int sensor_0098_I2C_tamper_switch(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // _SENSOR_0098_I2C_TAMPER_SWITCH_H_

