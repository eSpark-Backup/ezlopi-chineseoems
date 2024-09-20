
#include <stdlib.h>
#include <string.h>

#include "ezlopi_util_trace.h"

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices_list.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_cloud_constants.h"

#include "sensor_0069_ze08_ch02_gas_sensor.h"

#include "ze08_ch2o.h"
#include "pms5003.h"
#include "ens160.h"
#include "mics6814.h"
#include "device_airbnb.h"
#include "EZLOPI_USER_CONFIG.h"

static int __prepare(void* arg, void* user_arg);
static int __init(l_ezlopi_item_t* item);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __get_cjson_value(l_ezlopi_item_t* item, void* arg);
static int __notify(l_ezlopi_item_t* item);

static int __prepare_gas_sensor_ch2o(uint32_t *parent_id, cJSON* cj_device, void* user_arg);
static int __prepare_gas_sensor_vape(uint32_t parent_id, cJSON* cj_device, void* user_arg);
static int __prepare_gas_sensor_smoke(uint32_t parent_id, cJSON* cj_device, void* user_arg);
static int __prepare_gas_sensor_marijuana(uint32_t parent_id, cJSON* cj_device, void* user_arg);

static void __prepare_smoke_alarm_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);
static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);
static void __prepare_smoke_vape_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_smoke_vape_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);
static void __prepare_smoke_marijuana_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_smoke_marijuana_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);

static void __add_enums_to_json(cJSON* cj_result, const char** enums, uint8_t size);

static s_pms5003_sensor_object *pms_object;
static ezlopi_smoke_t gases;
static mics6814_t* mics6814;
static ens160_t* ens160_sensor;

// Function to detect smoke
static bool ezlopi_detect_smoke(ezlopi_smoke_t* gases) {
    return (gases->tvoc > SMOKE_TVOC_THRESHOLD &&
            gases->pm25_standard > SMOKE_PM2_5_THRESHOLD &&
            gases->co > SMOKE_CO_THRESHOLD &&
            gases->nh3 > SMOKE_NH3_THRESHOLD);
}

// Function to detect vape
static bool ezlopi_detect_vape(ezlopi_smoke_t* gases) {
    return (gases->tvoc > VAPE_TVOC_THRESHOLD_LOW && 
            gases->tvoc < VAPE_TVOC_THRESHOLD_HIGH &&
            gases->pm25_standard > VAPE_PM2_5_THRESHOLD &&
            gases->co > VAPE_CO_THRESHOLD &&
            gases->nh3 > VAPE_NH3_THRESHOLD);
}
/*
{\
    \"dev_type\": 6,\
    \"dev_name\": \"ZE08CH02 gas sensor\",\
    \"id_room\": 35,\
    \"id_item\": 69,\
    \"gpio_tx\": 48,\
    \"gpio_rx\": 45,\
    \"baud\": 9600\
}\
*/

static const char* vape_alarm_token[] = {
    "no_vape_detected",
    "vape_detected",
    "unknown",
};

static const char* marijuana_alarm_token[] = {
    "no_marijuana_detected",
    "marijuana_detected",
    "unknown",
};

static const char* smoke_alarm_token[] = {
    "no_smoke",
    "smoke_detected",
    "unknown",
};


int sensor_0069_ze08_ch02_gas_sensor(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    int ret = 0;
    // TRACE_D("%s", ezlopi_actions_to_string(action));
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
        if (ezlopi_item_name_vape == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(vape_alarm_token)/ sizeof(vape_alarm_token[0]);
            __add_enums_to_json(cj_result, vape_alarm_token, size);
        }
        else if (ezlopi_item_name_marijuana == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(marijuana_alarm_token)/ sizeof(marijuana_alarm_token[0]);
            __add_enums_to_json(cj_result, marijuana_alarm_token, size); 
        }
        else if (ezlopi_item_name_smoke_alarm == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(smoke_alarm_token)/ sizeof(smoke_alarm_token[0]);
            __add_enums_to_json(cj_result, smoke_alarm_token, size);
        }
        ret = 1;
    }
    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;

    cJSON* cj_params = (cJSON*)arg;
    ze08_ch2o_sensor_data_t* ze08_ch2o_sensor = (ze08_ch2o_sensor_data_t*)item->user_arg;
    if(!ze08_ch2o_sensor || !cj_params)
    {
        return ret;
    }

    ret = 1;

    if ((true == pms_is_data_available(&pms_object->pms_data)))
    {
        gases.pm25_standard = pms_object->pms_data.pm25_standard;
        pms_set_data_available_to_false(&pms_object->pms_data);
    }

    dfrobot_ens160_get_data(ens160_sensor);
    gases.tvoc = ens160_sensor->data.tvoc;

    gases.nh3 = mics6814_get_gas(mics6814, MICS6814_GAS_NH3);
    gases.co = mics6814_get_gas(mics6814, MICS6814_GAS_CO);

    if (ezlopi_item_name_particulate_matter_1 == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_float_to_cjson(cj_params, ze08_ch2o_sensor->ppm, item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_vape == item->cloud_properties.item_name)
    {
        // bool detected = ze08_ch2o_sensor->ppm > 150.0;
        bool detected = ezlopi_detect_vape(&gases);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, vape_alarm_token[detected]);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, vape_alarm_token[detected]);
    }
    else if (ezlopi_item_name_marijuana == item->cloud_properties.item_name)
    {
        bool detected = 0;
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, marijuana_alarm_token[detected]);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, marijuana_alarm_token[detected]);
    }
    else if (ezlopi_item_name_smoke_alarm == item->cloud_properties.item_name)
    {
        bool detected = ezlopi_detect_smoke(&gases);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, smoke_alarm_token[detected]);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, smoke_alarm_token[detected]);
    }
    else if (ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        bool detected = ezlopi_detect_smoke(&gases);
        ezlopi_valueformatter_bool_to_cjson(cj_params, detected, item->cloud_properties.scale);
    }
    
    return ret;
}

static int __notify(l_ezlopi_item_t* item)
{
    int ret = 0;
    ezlopi_device_value_updated_from_device_broadcast(item);
    // ze08_ch2o_sensor_data_t* ze08_ch2o_sensor = (ze08_ch2o_sensor_data_t*)item->user_arg;
    // if (ze08_ch2o_sensor && ze08_ch2o_sensor->available)
    // {
    //     ezlopi_device_value_updated_from_device_broadcast(item);
    //     ze08_ch2o_sensor->available = false;
    // }
    // else
    // {
    //     ret = 1;
    // }

    return ret;
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;
    ze08_ch2o_sensor_data_t* ze08_ch2o_sensor = (ze08_ch2o_sensor_data_t*)item->user_arg;
    static bool intitialized = false;
    if(intitialized)
    {
        TRACE_W("Already Initialized");
        return ret;
    }

    if (ze08_ch2o_sensor)
    {
        ret = 1;
        // if (item->interface.uart.enable)
        // {
        //     if (false == ze08_ch2o_sensor_init(&item->interface.uart, ze08_ch2o_sensor))
        //     {
        //         ret = -1;
        //     }
        //     else
        //     {
        //         intitialized = true;
        //     }
            
        // }
        intitialized = true;

        pms_object = (s_pms5003_sensor_object *)ezlopi_malloc(__FUNCTION__, sizeof(s_pms5003_sensor_object));
        if (pms_object)
        {
            pms_object->pms_tx_pin = 40;
            pms_object->pms_rx_pin = 41;
            pms_object->pms_set_pin = 39;
            pms_object->pms_reset_pin = 42;
            pms_object->pms_baud_rate = 9600;

            pms_object->pms_active_time = 30;
            pms_object->pms_sleep_time = 120;
            pms_object->pms_stability_time = 15;
            pms_object->pmsStatusReg = 0x00;

            memset(&pms_object->pms_data, 0, sizeof(PM25_AQI_Data));

            pms_object->counter = 0;
            pms_init(pms_object);
        }

        ens160_sensor = (ens160_t*) ezlopi_malloc(__FUNCTION__, sizeof(ens160_t));
        if (ens160_sensor)
        {
            memset(ens160_sensor, 0, sizeof(ens160_t));
            ens160_sensor->ezlopi_i2c = (s_ezlopi_i2c_master_t*) ezlopi_malloc(__FUNCTION__, sizeof(s_ezlopi_i2c_master_t));
            ens160_sensor->ezlopi_i2c->clock_speed = ENS160_I2C_SPEED;
            ens160_sensor->ezlopi_i2c->scl = 17;
            ens160_sensor->ezlopi_i2c->sda = 18;
            ens160_sensor->ezlopi_i2c->channel = I2C_NUM_0;
            ens160_sensor->ezlopi_i2c->address = ENS160_I2C_ADDRESS;
            ens160_sensor->ezlopi_i2c->enable = true;

            if (NO_ERR != dfrobot_ens160_i2c_begin(ens160_sensor))
            {
                TRACE_E("Communication with device failed, please check connection");
                ret = -1;
                vTaskDelay(3000 / portTICK_PERIOD_MS);
            }
            else
            {
                TRACE_E("Begin ok!");
                dfrobot_ens160_set_pwr_mode(ens160_sensor, ENS160_STANDARD_MODE);
                // dfrobot_ens160_set_temp_and_hum(ens160_sensor, /*temperature=*/25.0, /*humidity=*/50.0); // These should be able to be changed froms settings.
                dfrobot_ens160_set_temp_and_hum(ens160_sensor, 25, 50);
            }
        }

        mics6814 = (mics6814_t*) ezlopi_malloc(__FUNCTION__, sizeof(mics6814_t));
        if(mics6814)
        {
            mics6814->adc.ezlopi_i2c = (s_ezlopi_i2c_master_t*) ezlopi_malloc(__FUNCTION__, sizeof(s_ezlopi_i2c_master_t));
            if(mics6814->adc.ezlopi_i2c)
            {
                mics6814->adc.ezlopi_i2c->enable = true;
                mics6814->adc.ezlopi_i2c->address = ADS101X_I2C_ADDRESS_VDD;
                mics6814->adc.ezlopi_i2c->channel = I2C_NUM_0;
                mics6814->adc.ezlopi_i2c->sda = ADS101X_I2C_SDA;
                mics6814->adc.ezlopi_i2c->scl = ADS101X_I2C_SCL;
                mics6814->adc.ezlopi_i2c->clock_speed = ADS101X_I2C_SPEED;
                mics6814_init(mics6814, -1); // For Harware Interrupt
            }        
        }
    }
    else
    {
        ret = -1;
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

static void __prepare_ze08_ch2o_sensor_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_level_sensor;
    device->cloud_properties.subcategory = subcategory_particulate_matter;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
}

static void __prepare_ze08_ch2o_sensor_ppm_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_particulate_matter_1;
    item->cloud_properties.value_type = value_type_substance_amount;
    item->cloud_properties.scale = scales_parts_per_million;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static void __prepare_smoke_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_security_sensor;
    device->cloud_properties.subcategory = subcategory_smoke;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
}

static void __prepare_smoke_alarm_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_smoke_alarm;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_security_threat;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static void __prepare_smoke_vape_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_generic_sensor;
    device->cloud_properties.subcategory = subcategory_vape;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
}

static void __prepare_smoke_vape_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_vape;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static void __prepare_smoke_marijuana_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_generic_sensor;
    device->cloud_properties.subcategory = subcategory_marijuana;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
}

static void __prepare_smoke_marijuana_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_marijuana;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static int __prepare(void* arg, void* user_arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if (prep_arg)
    {
        ze08_ch2o_sensor_data_t* ze08_ch2o_sensor = (ze08_ch2o_sensor_data_t*)ezlopi_malloc(__FUNCTION__, sizeof(ze08_ch2o_sensor_data_t));
        if (ze08_ch2o_sensor)
        {
            memset(ze08_ch2o_sensor, 0, sizeof(ze08_ch2o_sensor_data_t));
            // if (-1 == (ret = __prepare_gas_sensor_ch2o(&parent_id, prep_arg->cjson_device, ze08_ch2o_sensor)))
            // {
            //     ezlopi_free(__FUNCTION__, ze08_ch2o_sensor);
            // }
            if (-1 == (ret = __prepare_gas_sensor_smoke(parent_id, prep_arg->cjson_device, ze08_ch2o_sensor)))
            {
                ezlopi_free(__FUNCTION__, ze08_ch2o_sensor);
            }
            if (-1 == (ret = __prepare_gas_sensor_marijuana(parent_id, prep_arg->cjson_device, ze08_ch2o_sensor)))
            {
                ezlopi_free(__FUNCTION__, ze08_ch2o_sensor);
            }
            if (-1 == (ret = __prepare_gas_sensor_vape(parent_id, prep_arg->cjson_device, ze08_ch2o_sensor)))
            {
                ezlopi_free(__FUNCTION__, ze08_ch2o_sensor);
            }
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}


static int __prepare_gas_sensor_ch2o(uint32_t *parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* ze08_ch2o_sensor_device = ezlopi_device_add_device(cj_device, "CH2O");

    if (ze08_ch2o_sensor_device)
    {
        ret = 1;
        __prepare_ze08_ch2o_sensor_device_cloud_properties(ze08_ch2o_sensor_device);
        // *parent_id = ze08_ch2o_sensor_device->cloud_properties.device_id; 
        l_ezlopi_item_t* ze08_ch02_ppm_item = ezlopi_device_add_item_to_device(ze08_ch2o_sensor_device, sensor_0069_ze08_ch02_gas_sensor);
        if (ze08_ch02_ppm_item)
        {
            __prepare_ze08_ch2o_sensor_ppm_item_cloud_properties(ze08_ch02_ppm_item, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(ze08_ch2o_sensor_device);
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static int __prepare_gas_sensor_vape(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Vape Sensor");

    if (device)
    {
        ret = 1;
        __prepare_smoke_vape_device_cloud_properties(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0069_ze08_ch02_gas_sensor);
        if (item)
        {
            __prepare_smoke_vape_item_cloud_properties(item, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static int __prepare_gas_sensor_smoke(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Smoke Sensor");

    if (device)
    {
        ret = 1;
        __prepare_smoke_device_cloud_properties(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0069_ze08_ch02_gas_sensor);
        if (item)
        {
            __prepare_smoke_alarm_item_cloud_properties(item, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
            goto out;
        }

        l_ezlopi_item_t* item_2 = ezlopi_device_add_item_to_device(device, sensor_0069_ze08_ch02_gas_sensor);
        if (item_2)
        {
            __prepare_security_threat_item_cloud_properties(item_2, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }

        ezlopi_airbnb_set_device(device);
    }
    else
    {
        ret = -1;
    }

    out:
    return ret;
}

static int __prepare_gas_sensor_marijuana(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Marijuana Sensor");

    if (device)
    {
        ret = 1;
        __prepare_smoke_marijuana_device_cloud_properties(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0069_ze08_ch02_gas_sensor);
        if (item)
        {
            __prepare_smoke_marijuana_item_cloud_properties(item, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}