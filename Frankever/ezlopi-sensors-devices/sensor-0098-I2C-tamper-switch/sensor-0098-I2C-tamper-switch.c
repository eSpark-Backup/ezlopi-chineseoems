
#include <stdlib.h>
#include <math.h>

#include "ezlopi_util_trace.h"

#include "ezlopi_core_devices_list.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_device_value_updated.h"
#include "ezlopi_core_valueformatter.h"

#include "ezlopi_hal_i2c_master.h"
#include "sensor-0098-I2C-tamper-switch.h"

#include "ezlopi_cloud_constants.h"
#include "device_airbnb.h"
#include "EZLOPI_USER_CONFIG.h"

static const char* tampering_cover_alarm_token[] = {
    "no_tampering_cover",
    "tampering_cover",
    "unknown",
};

static int __prepare(void* arg, void* user_arg);
static int __init(l_ezlopi_item_t* item);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __get_cjson_value(l_ezlopi_item_t* item, void* arg);
static int __notify(l_ezlopi_item_t* item);

int sensor_0098_I2C_tamper_switch(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
  int ret = 0;
  switch (action)
  {
  case EZLOPI_ACTION_PREPARE:
  {
    ret = __prepare(arg, user_arg);
    break;
  }
  case EZLOPI_ACTION_INITIALIZE:
  {
    ret = __init(item);
    break;
  }
  case EZLOPI_ACTION_HUB_GET_ITEM:
  {
    ret = __get_item(item, arg);
  }
  case EZLOPI_ACTION_GET_EZLOPI_VALUE:
  {
    ret = __get_cjson_value(item, arg);
    break;
  }
  case EZLOPI_ACTION_NOTIFY_1000_MS:
  {
    ret = __notify(item);
    break;
  }
  default:
  {
    break;
  }
  }
  return ret;
}

static int __get_item(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (cj_result)
        {
            //-------------------  POSSIBLE JSON ENUM CONTENTS ----------------------------------
            cJSON* json_array_enum = cJSON_CreateArray(__FUNCTION__);
            if (NULL != json_array_enum)
            {
                for (uint8_t i = 0; i < (sizeof(tampering_cover_alarm_token)/ sizeof(tampering_cover_alarm_token[0])); i++)
                {
                    cJSON* json_value = cJSON_CreateString(__FUNCTION__, tampering_cover_alarm_token[i]);
                    if (NULL != json_value)
                    {
                        cJSON_AddItemToArray(json_array_enum, json_value);
                    }
                }
                cJSON_AddItemToObject(__FUNCTION__, cj_result, ezlopi_enum_str, json_array_enum);
            }
            ret = 1;
        }
    }
    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t* item, void* arg)
{
  int ret = 0;

  if (item && arg)
  {
    cJSON* cj_result = (cJSON*)arg;
    int value = pca9536_digital_read(TAMPER_SWITCH_GPIO);
    cJSON_AddStringToObject(__FUNCTION__, cj_result, ezlopi_valueFormatted_str, tampering_cover_alarm_token[!value]);
    cJSON_AddStringToObject(__FUNCTION__, cj_result, ezlopi_value_str, tampering_cover_alarm_token[!value]);
  } 
  else
  {
    ret = 1;
  }

  return ret;
}

static int __notify(l_ezlopi_item_t* item)
{
  int ret = 0;
  ezlopi_device_value_updated_from_device_broadcast(item);

  return ret;
}

static int __init(l_ezlopi_item_t* item)
{
  int ret = -1;
  if (pca9536_init(&item->interface.i2c_master))
  {
    if (pca9536_begin())
    {
      pca9536_pin_mode(TAMPER_SWITCH_GPIO, INPUT);
      TRACE_I("Tamper Switch : pca9536 initialized");
      ret = 1;
    }
  }

  return ret;
}

static void prepare_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_tampering_cover_alarm;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
}

static void prepare_item_interface_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_I2C_MASTER;
    item->interface.i2c_master.enable = true;
    item->interface.i2c_master.channel = EZLOPI_I2C_0;
    item->interface.i2c_master.clock_speed = 400000;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_sda_str, item->interface.i2c_master.sda);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_scl_str, item->interface.i2c_master.scl);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_slave_addr_str, item->interface.i2c_master.address);
}

static int __prepare(void* arg, void* user_arg)
{
    int ret = 0;

    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        if (cj_device)
        {
            l_ezlopi_device_t* device = ezlopi_airbnb_get_device();
            if (device)
            {
                ret = 1;
                l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0098_I2C_tamper_switch);
                if (item)
                {
                    prepare_item_cloud_properties(item, cj_device);
                    prepare_item_interface_properties(item, cj_device);
                }
                else
                {
                    ezlopi_device_free_device(device);
                    ret = -1;
                }
            }
            else
            {
                ret = -1;
            }
        }
    }

    return ret;
}