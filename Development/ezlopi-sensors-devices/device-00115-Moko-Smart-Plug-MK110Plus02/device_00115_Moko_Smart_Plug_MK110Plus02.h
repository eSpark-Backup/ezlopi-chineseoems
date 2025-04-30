#ifndef _DEVICE_00115_MOKO_SMART_PLUG_H_
#define _DEVICE_00115_MOKO_SMART_PLUG_H_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_errors.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "ezlopi_hal_uart.h"
#include "ezlopi_cloud_items.h"

#include "ezlopi_core_ota.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_core_factory_info.h" 

#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

ezlopi_error_t device_00115_Moko_Smart_Plug(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *args, void *user_arg);
#endif // _DEVICE_00115_MOKO_SMART_PLUG_H_