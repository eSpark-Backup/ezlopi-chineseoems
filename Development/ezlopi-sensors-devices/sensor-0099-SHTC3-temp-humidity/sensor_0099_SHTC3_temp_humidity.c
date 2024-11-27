#include <math.h>
#include <shtc3.h>
#include "ezlopi_util_trace.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "sensor_0099_SHTC3_temp_humidity.h"

typedef struct s_ezlopi_shtc3_data
{
    float temperature;
    float humidity;
} s_ezlopi_shtc3_data_t;

static int __0099_prepare(void* arg);
static int __0099_init(l_ezlopi_item_t* item);
static int __0099_get_cjson_value(l_ezlopi_item_t* item, void* arg);
static int __shtc3_item_cloud_properties_temp(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);
static int __shtc3_item_cloud_properties_humidity(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);
static int __prepare_shtc3_temp_device_item(uint32_t parent_id, cJSON* cj_device, s_ezlopi_shtc3_data_t* user_data);
static int __prepare_shtc3_humid_device_item(uint32_t parent_id, cJSON* cj_device, s_ezlopi_shtc3_data_t* user_data);

int sensor_0099_SHTC3_temp_humidity(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __0099_prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __0099_init(item);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __0099_get_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    // {
    //     ret = __0099_notify(item);
    //     break;
    // }
    default:
    {
        break;
    }
    }
    return ret;
}

static void __prepare_device_cloud_properties_temp(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_temperature;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_device_cloud_properties_humid(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_humidity;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static int __shtc3_item_cloud_properties_temp(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    if (item && cj_device)
    {
        item->user_arg = user_arg;
        item->cloud_properties.show = true;
        item->cloud_properties.has_getter = true;
        item->cloud_properties.has_setter = false;
        item->cloud_properties.item_name = ezlopi_item_name_temp;
        item->cloud_properties.value_type = value_type_temperature;
        item->cloud_properties.scale = scales_celsius;
        item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
        CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);

        item->is_user_arg_unique = true;
        item->interface.i2c_master.enable = true;
        item->interface.i2c_master.clock_speed = I2C_MASTER_FREQ_100KHZ;
        CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_scl_str, item->interface.i2c_master.scl);
        CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_sda_str, item->interface.i2c_master.sda);
        item->interface.i2c_master.address = SHTC3_ADDR;
    }
    return ret;
}

static int __shtc3_item_cloud_properties_humidity(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    int ret = 0;

    if (item && cj_device)
    {
        item->user_arg = user_arg;
        item->cloud_properties.show = true;
        item->cloud_properties.has_getter = true;
        item->cloud_properties.has_setter = false;
        item->cloud_properties.item_name = ezlopi_item_name_humidity;
        item->cloud_properties.value_type = value_type_humidity;
        item->cloud_properties.scale = scales_percent;
        item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
        CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);
        
        item->is_user_arg_unique = true;
        item->interface.i2c_master.enable = true;
        item->interface.i2c_master.clock_speed = I2C_MASTER_FREQ_100KHZ;
        CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_scl_str, item->interface.i2c_master.scl);
        CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_sda_str, item->interface.i2c_master.sda);
        item->interface.i2c_master.address = SHTC3_ADDR;
    }

    return ret;
}

static int __0099_prepare(void* arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;

    if (prep_arg)
    {
        s_ezlopi_shtc3_data_t* user_data = (s_ezlopi_shtc3_data_t*)ezlopi_malloc(__FUNCTION__, sizeof(s_ezlopi_shtc3_data_t));
        l_ezlopi_device_t* device_head = ezlopi_device_get_head();
        if (user_data && device_head)
        {
            memset(user_data, 0, sizeof(s_ezlopi_shtc3_data_t));

            if (-1 == (ret = __prepare_shtc3_temp_device_item(device_head->cloud_properties.device_id, prep_arg->cjson_device, user_data)))
            {
                TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
            }
            if (-1 == (ret = __prepare_shtc3_humid_device_item(device_head->cloud_properties.device_id, prep_arg->cjson_device, user_data)))
            {
                TRACE_E("UNABLE TO PREPARE HUMID DEVICE");
            }
        }
    }

    return ret;
}

static int __prepare_shtc3_temp_device_item(uint32_t parent_id, cJSON* cj_device, s_ezlopi_shtc3_data_t* user_data) // parent device
{
    int ret = 0;
    
    l_ezlopi_device_t *device= ezlopi_device_add_device(cj_device, " Temperature");
    
    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_temp(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0099_SHTC3_temp_humidity);
        if (item && user_data)
        {
            __shtc3_item_cloud_properties_temp(item, cj_device, user_data);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static int __prepare_shtc3_humid_device_item(uint32_t parent_id, cJSON* cj_device, s_ezlopi_shtc3_data_t* user_data)
{
    int ret = 0;
    l_ezlopi_device_t *device= ezlopi_device_add_device(cj_device, " Humidity");
    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_humid(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0099_SHTC3_temp_humidity);
        if (item && user_data)
        {
            __shtc3_item_cloud_properties_humidity(item, cj_device, user_data);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static int __0099_init(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        s_ezlopi_shtc3_data_t* user_data = (s_ezlopi_shtc3_data_t*)item->user_arg;
        if (user_data)
        {
            if (item->interface.i2c_master.enable && (ezlopi_item_name_temp == item->cloud_properties.item_name))
            {
                shtc3_init_sensor(&item->interface.i2c_master);
                ret = 1;
            }
            else if(ezlopi_item_name_humidity == item->cloud_properties.item_name)
            {
                ret = 1;
            }
            else
            {
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }
    }
    return ret;
}

static int __0099_get_cjson_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;

    if (item && arg)
    {
        cJSON* cj_device = (cJSON*)arg;
        if (cj_device)
        {
            s_ezlopi_shtc3_data_t* user_data = (s_ezlopi_shtc3_data_t*)item->user_arg;

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
            }
        }
    }

    return ret;
}

int __0099_notify(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        s_ezlopi_shtc3_data_t* user_data = (s_ezlopi_shtc3_data_t*)item->user_arg;
        if (user_data)
        {
            float temperature = user_data->temperature;
            float humidity = user_data->humidity;
            
            shtc3_wakeup_sensor(&item->interface.i2c_master);
            shtc3_read_out(&item->interface.i2c_master,&temperature, &humidity);
            shtc3_sleep_sensor(&item->interface.i2c_master);

            if (ezlopi_item_name_temp == item->cloud_properties.item_name)
            {
                if (fabs(user_data->temperature - temperature) > 0.05)
                {
                    user_data->temperature = temperature;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                }
            }
            if (ezlopi_item_name_humidity == item->cloud_properties.item_name)
            {
                if (fabs(user_data->humidity - humidity) > 0.05)
                {
                    user_data->humidity = humidity;
                    ezlopi_device_value_updated_from_device_broadcast(item);
                }
            }
            ret = 1;
        }
    }

    return ret;
}