#include "esp_err.h"

#include "ezlopi_util_trace.h"

#include "ezlopi_core_devices_list.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_cjson_macros.h"

#include "ezlopi_cloud_constants.h"

#include "pms5003.h"
#include "sensor_0037_pms5003_sensor.h"
#include "device_airbnb.h"
#include "EZLOPI_USER_CONFIG.h"

// Device 0
static int __prepare_particulate_matter_particles_0_dot_3_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_0.3_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 0.3 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_0_dot_3_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = true;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
            item->is_user_arg_unique = true;
        }
        else
        {
            ezlopi_device_free_device(device);
            ret = -1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 1
static int __prepare_particulate_matter_particles_0_dot_5_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_0.5_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 0.5 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_0_dot_5_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 2
static int __prepare_particulate_matter_particles_1_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_1_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 1 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_1_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 3
static int __prepare_particulate_matter_particles_2_dot_5_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_2.5_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 2.5 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_2_dot_5_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 4
static int __prepare_particulate_matter_particles_5_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_5_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 5 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_5_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 5
static int __prepare_particulate_matter_particles_10_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_10_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles 10 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particles_10_um;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_particles_per_deciliter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 6
static int __prepare_particulate_matter_standard_particles_1_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_PM_1_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles PM 1 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particulate_matter_1;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_micro_gram_per_cubic_meter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
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

// Device 7
static int __prepare_particulate_matter_standard_particles_2_dot_5_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_PM_2.5_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles PM 2.5 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particulate_matter_2_dot_5;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_micro_gram_per_cubic_meter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

// Device 8
static int __prepare_particulate_matter_standard_particles_10_um_device_and_items(cJSON *cj_properties, void *user_arg)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_properties, "Dust_Particles_PM_10_um");
    if (device)
    {
        ret = 1;
        // char *dev_name = "Dust Particles PM 10 um";
        // ASSIGN_DEVICE_NAME_V2(device, dev_name);
        device->cloud_properties.category = category_level_sensor;
        device->cloud_properties.subcategory = subcategory_particulate_matter;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0037_pms5003_v3);
        if (item)
        {
            item->cloud_properties.has_getter = true;
            item->cloud_properties.has_setter = false;
            item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            item->cloud_properties.item_name = ezlopi_item_name_particulate_matter_10;
            item->cloud_properties.show = true;
            item->cloud_properties.value_type = value_type_substance_amount;
            item->cloud_properties.scale = scales_micro_gram_per_cubic_meter;

            item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
            item->interface.uart.enable = false;
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio1_str, item->interface.uart.tx);
            CJSON_GET_VALUE_GPIO(cj_properties, ezlopi_gpio2_str, item->interface.uart.rx);
            item->interface.uart.baudrate = 9600;

            item->user_arg = user_arg;
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int pms5003_set_pms_object_details(cJSON *cj_properties, s_pms5003_sensor_object *pms_object)
{
    int ret = 0;

    assert(pms_object != NULL);

    CJSON_GET_VALUE_DOUBLE(cj_properties, ezlopi_gpio1_str, pms_object->pms_tx_pin);
    CJSON_GET_VALUE_DOUBLE(cj_properties, ezlopi_gpio2_str, pms_object->pms_rx_pin);
    CJSON_GET_VALUE_DOUBLE(cj_properties, ezlopi_gpio3_str, pms_object->pms_set_pin);
    CJSON_GET_VALUE_DOUBLE(cj_properties, "gpio4", pms_object->pms_reset_pin);
    pms_object->pms_baud_rate = 9600;

    pms_object->pms_active_time = 30;
    pms_object->pms_sleep_time = 120;
    pms_object->pms_stability_time = 15;
    pms_object->pmsStatusReg = 0x00;

    memset(&pms_object->pms_data, 0, sizeof(PM25_AQI_Data));

    pms_object->counter = 0;

    return ret;
}

int pms5003_sensor_preapre_devices_and_items(cJSON *cj_properties)
{
    int ret = 0;

    s_pms5003_sensor_object *pms_object = (s_pms5003_sensor_object *)ezlopi_malloc(__FUNCTION__, sizeof(s_pms5003_sensor_object));
    if (pms_object)
    {
        pms5003_set_pms_object_details(cj_properties, pms_object);
        __prepare_particulate_matter_particles_0_dot_3_um_device_and_items(cj_properties, (void *)pms_object);            
        __prepare_particulate_matter_particles_0_dot_5_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_particles_1_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_particles_2_dot_5_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_particles_5_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_particles_10_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_standard_particles_1_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_standard_particles_2_dot_5_um_device_and_items(cj_properties, (void *)pms_object);
        __prepare_particulate_matter_standard_particles_10_um_device_and_items(cj_properties, (void *)pms_object);
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}
