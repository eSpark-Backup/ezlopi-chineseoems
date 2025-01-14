#ifndef _SENSOR_0099_SHTC3_TEMP_HUMIDITY_H_
#define _SENSOR_0099_SHTC3_TEMP_HUMIDITY_H_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

ezlopi_error_t __notify(l_ezlopi_item_t* item);
ezlopi_error_t sensor_0099_SHTC3_temp_humidity(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif //_SENSOR_0099_SHTC3_TEMP_HUMIDITY_H_