#include <math.h>
#include "ezlopi_util_trace.h"

// #include "ezlopi_core_timer.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_hal_i2c_master.h"

#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"

#include "bme680_bsec.h"
#include "sensor_0010_I2C_BME680.h"
#include "EZLOPI_USER_CONFIG.h"

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __notify(l_ezlopi_item_t* item);
static int __get_cjson_value(l_ezlopi_item_t* item, void* arg);

static void __add_enums_to_json(cJSON* cj_result, const char** enums, uint8_t size);

uint16_t co2_ranges[] = { 700, 800, 1100, 1500, 2000, 3000, 5000 };

const char* aqi_grade[] =  {
    "excellent",
    "good",
    "lightly_polluted",
    "moderately_polluted",
    "heavily_polluted",
    "severely_polluted",
    "extremely_polluted",
    "unknown"
};

int sensor_0010_I2C_BME680(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __init(item);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    {
        ret =__get_item(item, arg);
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

static int __prepare(void* arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    bme680_sensor_dev_item_prep(prep_arg);

    return ret;
}

static int __get_item(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (ezlopi_item_name_air_quality_grade == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(aqi_grade)/ sizeof(aqi_grade[0]);
            __add_enums_to_json(cj_result, aqi_grade, size);
        }
        ret = 1;
    }
    return ret;
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        bme680_data_t* user_data = (bme680_data_t*)item->user_arg;
        if (user_data)
        {
            if (item->interface.i2c_master.enable)
            {
                ezlopi_i2c_master_init(&item->interface.i2c_master);
                bme680_setup(item->interface.i2c_master.sda, item->interface.i2c_master.scl, true);
                ret = 1;
            }
        }
        else
        {
            ret = -1;
        }
    }
    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_device = (cJSON*)arg;
        if (cj_device)
        {
            bme680_data_t* user_data = (bme680_data_t*)item->user_arg;
            if (user_data)
            {
                if (ezlopi_item_name_temp == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->temperature, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_humidity == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->humidity, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_atmospheric_pressure == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->pressure / 1000.0, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_volatile_organic_compound_level == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->voc_equivalent, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_distance == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->altitude, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_co2_level == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->co2_equivalent, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_air_quality == item->cloud_properties.item_name)
                {
                    ezlopi_valueformatter_float_to_cjson(cj_device, user_data->iaq, item->cloud_properties.scale);
                }
                else if (ezlopi_item_name_air_quality_grade == item->cloud_properties.item_name)
                {
                    double aqi_grade_index = (float)user_data->iaq / 50;
                    
                    double co2_ppm_index = 0;
                    for (size_t index = 0; index < sizeof(co2_ranges)/ sizeof(co2_ranges[0]); index++)
                    {
                        if(user_data->co2_equivalent < co2_ranges[index])
                        {
                            co2_ppm_index = index;
                            break;
                        }
                    }
                    
                    double voc_index = user_data->voc_equivalent/ 100;
                    
                    uint8_t index = fmax(voc_index, fmax(co2_ppm_index, aqi_grade_index));
                    cJSON_AddStringToObject(__FUNCTION__, cj_device, ezlopi_valueFormatted_str, aqi_grade[index]);
                    cJSON_AddStringToObject(__FUNCTION__, cj_device, ezlopi_value_str, aqi_grade[index]);
                }
            }
        }
    }
    return ret;
}

static int __notify(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        bme680_data_t* user_data = (bme680_data_t*)item->user_arg;
        if (user_data)
        {
            // float temperature = user_data->temperature;
            // float humidity = user_data->humidity;
            // float pressure = user_data->pressure;
            // float iaq = user_data->iaq;
            // float altitude = user_data->altitude;
            // float voc_equivalent = user_data->voc_equivalent;
            // float co2_eqv = user_data->co2_equivalent;

            bme680_get_data(user_data);

            if (ezlopi_item_name_temp == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->temperature - temperature) > 0.05)
                // {
                    // user_data->temperature = temperature;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_humidity == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->humidity - humidity) > 0.05)
                // {
                    // user_data->humidity = humidity;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_atmospheric_pressure == item->cloud_properties.item_name)
            {
                // if (fabs((user_data->pressure / 1000.0f) - (pressure / 1000.0f)) > 0.05)
                // {
                    // user_data->pressure = pressure;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_volatile_organic_compound_level == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->iaq - iaq) > 0.05)
                // {
                    // user_data->voc_equivalent = voc_equivalent;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_air_quality == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->aqi - aqi) > 0.05)
                // {
                    // user_data->aqi = aqi;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_distance == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->altitude - altitude) > 0.05)
                // {
                    // user_data->altitude = altitude;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
            if (ezlopi_item_name_co2_level == item->cloud_properties.item_name)
            {
                // if (fabs(user_data->co2_equivalent - co2_eqv) > 0.05)
                // {
                    // user_data->co2_equivalent = co2_eqv;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                // }
            }
        }
    }

    return ret;
}

static void __add_enums_to_json(cJSON* cj_result, const char** enums, uint8_t size)
{
    if (cj_result)
    {
        //-------------------  POSSIBLE JSON ENUM CONTENTS ----------------------------------
        cJSON* json_array_enum = cJSON_CreateArray(__FUNCTION__);
        if (NULL != json_array_enum)
        {
            for (uint8_t i = 0; i < size; i++)
            {
                cJSON* json_value = cJSON_CreateString(__FUNCTION__, enums[i]);
                if (NULL != json_value)
                {
                    cJSON_AddItemToArray(json_array_enum, json_value);
                }
            }
            cJSON_AddItemToObject(__FUNCTION__, cj_result, ezlopi_enum_str, json_array_enum);
        }
    }
}