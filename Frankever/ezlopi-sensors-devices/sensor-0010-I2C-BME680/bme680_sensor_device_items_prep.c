#include "esp_err.h"
#include "ezlopi_core_devices_list.h"
#include "ezlopi_cloud_constants.h"
#include "bme680_bsec.h"
#include "device_airbnb.h"
#include "sensor_0010_I2C_BME680.h"

#define ENABLE_VOC 0
#define ENABLE_CO2 0

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg);

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
    
}

static void __prepare_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.show = true;
    item->user_arg = user_arg;
    item->interface.i2c_master.enable = false;
}

/* Temperature device and item prepare */
static void __prepare_device_cloud_properties_temperature(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_temperature;
}


static void __prepare_cloud_item_properties_temperature(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_temp;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = scales_celsius;
    
    item->is_user_arg_unique = true;
    item->interface.i2c_master.enable = true;
    item->interface.i2c_master.clock_speed = 100000;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_scl_str, item->interface.i2c_master.scl);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_sda_str, item->interface.i2c_master.sda);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_slave_addr_str, item->interface.i2c_master.address);
}

static int __prepare_bme680_sensor_temperature(uint32_t *parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Ambient Temperature Sensor");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_temperature(device);
        // *parent_id = device->cloud_properties.device_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_temperature(item, cj_device, user_arg);
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

/* Humiditiy device and item prepare */
static void __prepare_device_cloud_properties_humidity(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = ezlopi_item_name_humidity;
}


static void __prepare_cloud_item_properties_humidity(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_humidity;
    item->cloud_properties.value_type = value_type_humidity;
    item->cloud_properties.scale = scales_percent;
}

static int __prepare_bme680_sensor_humidity(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Ambient Humidity Sensor");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_humidity(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_humidity(item, cj_device, user_arg);
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

/* Pressure device and item prepare */
static void __prepare_device_cloud_properties_pressure(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_weather;
}


static void __prepare_cloud_item_properties_pressure(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_atmospheric_pressure;
    item->cloud_properties.value_type = value_type_pressure;
    item->cloud_properties.scale = scales_kilo_pascal;
}

static int __prepare_bme680_sensor_pressure(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Atmospheric Pressure Sensor");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_pressure(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_pressure(item, cj_device, user_arg);
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

#if ENABLE_VOC
/* VOC device and item prepare */
static void __prepare_device_cloud_properties_voc(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_level_sensor;
}


static void __prepare_cloud_item_properties_voc(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_volatile_organic_compound_level;
    item->cloud_properties.value_type = value_type_substance_amount;
    item->cloud_properties.scale = scales_parts_per_million;
}

static int __prepare_bme680_sensor_voc(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "voc");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_voc(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_voc(item, cj_device, user_arg);
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
#endif // ENABLE_VOC

/* Altitude device and item prepare */
static void __prepare_device_cloud_properties_altitude(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_level_sensor;
    device->cloud_properties.subcategory = subcategory_navigation;
}


static void __prepare_cloud_item_properties_altitude(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_distance;
    item->cloud_properties.value_type = value_type_length;
    item->cloud_properties.scale = scales_meter;
}

static int __prepare_bme680_sensor_altitude(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Altitude Sensor");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_altitude(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_altitude(item, cj_device, user_arg);
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

#if ENABLE_CO2
/* CO2 device and item prepare */
static void __prepare_device_cloud_properties_co2(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_level_sensor;
    device->cloud_properties.subcategory = subcategory_co2;
}


static void __prepare_cloud_item_properties_co2(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_co2_level;
    item->cloud_properties.value_type = value_type_substance_amount;
    item->cloud_properties.scale = scales_parts_per_million;
}

static int __prepare_bme680_sensor_co2(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "co2");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_co2(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_co2(item, cj_device, user_arg);
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
#endif // ENABLE_CO2

/* Indoor AQI device and item prepare */
static void __prepare_device_cloud_properties_aqi(l_ezlopi_device_t* device)
{
    __prepare_device_cloud_properties(device);
    device->cloud_properties.category = category_level_sensor;
    device->cloud_properties.subcategory = subcategory_airpollution;
}


static void __prepare_cloud_item_properties_aqi(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_air_quality;
    item->cloud_properties.value_type = value_type_air_quality;
    item->cloud_properties.scale = scales_aqi;
}

static void __prepare_cloud_item_properties_aqi_grade(l_ezlopi_item_t* item, cJSON* cj_device, void* user_arg)
{
    __prepare_item_cloud_properties(item, cj_device, user_arg);
    item->cloud_properties.item_name = ezlopi_item_name_air_quality_grade;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;
}

static int __prepare_bme680_sensor_aqi(uint32_t parent_id, cJSON* cj_device, void* user_arg)
{
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, "Indoor Air Quality Sensor");

    if (device)
    {
        ret = 1;
        __prepare_device_cloud_properties_aqi(device);
        // device->cloud_properties.parent_device_id = parent_id; 
        l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item)
        {
            __prepare_cloud_item_properties_aqi(item, cj_device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }

        l_ezlopi_item_t* item2 = ezlopi_device_add_item_to_device(device, sensor_0010_I2C_BME680);
        if (item2)
        {
            __prepare_cloud_item_properties_aqi_grade(item2, cj_device, user_arg);
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

int bme680_sensor_dev_item_prep(s_ezlopi_prep_arg_t *prep_arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    if (prep_arg && prep_arg->cjson_device)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        bme680_data_t* user_data = (bme680_data_t*)ezlopi_malloc(__FUNCTION__, sizeof(bme680_data_t));
        if (user_data)
        {
            memset(user_data, 0, sizeof(bme680_data_t));
            if (-1 == (ret = __prepare_bme680_sensor_temperature(&parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }

            if (-1 == (ret = __prepare_bme680_sensor_humidity(parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }

            if (-1 == (ret = __prepare_bme680_sensor_pressure(parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }
#if ENABLE_VOC
            if (-1 == (ret = __prepare_bme680_sensor_voc(parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }
#endif
            // if (-1 == (ret = __prepare_bme680_sensor_altitude(parent_id, cj_device, user_data)))
            // {
            //     ezlopi_free(__FUNCTION__, user_data);
            // }

#if ENABLE_CO2
            if (-1 == (ret = __prepare_bme680_sensor_co2(parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }
#endif

            if (-1 == (ret = __prepare_bme680_sensor_aqi(parent_id, cj_device, user_data)))
            {
                ezlopi_free(__FUNCTION__, user_data);
            }
   
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}