#include "bl0942.h"
#include "device_00114_Moko_Smart_Plug_MK110Plus01.h"

#define RED_LED     32
#define BLUE_LED    25
#define GREEN_LED   33

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static ezlopi_error_t __prepare_smart_plug_device_item(uint32_t *parent_id, cJSON* cjson_device);
static ezlopi_error_t __prepare_metering_device_item(uint32_t parent_id, cJSON* cjson_device);

ezlopi_error_t device_00114_Moko_Smart_Plug(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
{
    ezlopi_error_t error = 0;

    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            error = __prepare(args);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            error = __init(item);
            break;
        }
        case EZLOPI_ACTION_SET_VALUE:
        {
            error = __set_value(item, args);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            error = __get_value_cjson(item, args);
            break;
        }

        default:
        {
            break;
        }
    }

    return error;
}

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    
    if (prep_arg)
    {
        if (-1 == (error = __prepare_smart_plug_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (-1 == (error = __prepare_metering_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
    }
    
    return error;
}

static ezlopi_error_t __init(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_ERR_INIT_DEVICE_FAILED;
    static bool intitialized = false;
    if(intitialized)
    {
        return error;
    }

    if (item)
    {
        if(item->cloud_properties.item_name == ezlopi_item_name_switch)
        {
            if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_out.gpio_num) &&
                (255 != item->interface.gpio.gpio_out.gpio_num))
            {
                const gpio_config_t io_conf = {
                    .pin_bit_mask = (1ULL << item->interface.gpio.gpio_out.gpio_num),
                    .mode = GPIO_MODE_OUTPUT,
                    .pull_up_en = ((item->interface.gpio.gpio_out.pull == GPIO_PULLUP_ONLY) ||
                                (item->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
                                    ? GPIO_PULLUP_ENABLE
                                    : GPIO_PULLUP_DISABLE,
                    .pull_down_en = ((item->interface.gpio.gpio_out.pull == GPIO_PULLDOWN_ONLY) ||
                                    (item->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
                                        ? GPIO_PULLDOWN_ENABLE
                                        : GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_DISABLE,
                };
                if (0 == gpio_config(&io_conf))
                {   
                    const gpio_config_t io_conf = {
                        .pin_bit_mask = (1ULL << RED_LED) | (1ULL << RED_LED),
                        .mode = GPIO_MODE_OUTPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_POSEDGE,
                    };
                    gpio_config(&io_conf);
                    error = EZPI_SUCCESS;
                }
                else
                {
                    error = EZPI_FAILED;
                }
            }
        }
        else if(item->cloud_properties.item_name == ezlopi_item_name_current)
        {
            if(EZPI_SUCCESS == bl0942_init())
            {
                TRACE_I("BL0942 SPI initialized in 3-wire mode.");
            }
            else
            {
                TRACE_E("BL0942 SPI init failed!");
            }
        }
        else if(item->cloud_properties.item_name == ezlopi_item_name_voltage)
        {

        }
    }
    
    return error;
}

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if (item && arg)
    {
        bool value = 0;
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            if (cj_value)
            {
                switch (cj_value->type)
                {
                    case cJSON_False: {
                        value = false;
                        break;
                    }
                    case cJSON_True: {
                        value = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                item->user_arg = (void *) value;
                gpio_set_level(item->interface.gpio.gpio_out.gpio_num, item->user_arg);
                gpio_set_level(RED_LED, item->user_arg);
                EZPI_core_device_value_updated_from_device_broadcast(item);
            }
        }
    }
    return error;
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;
    if (item && args)
    {
        if (item && args)
        {
            cJSON* cj_properties = (cJSON*)args;
            if (cj_properties)
            {
                EZPI_core_valueformatter_bool_to_cjson(cj_properties, item->user_arg, item->cloud_properties.scale);
                error = EZPI_SUCCESS; 
            }
        }
    }
    return error;
}

static void __setup_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_relay;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __setup_metering_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_power_meter;
    device->cloud_properties.subcategory = subcategory_power;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __setup_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_gpio_out_str, item->interface.gpio.gpio_out.gpio_num);
}

static void __setup_current_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_current;
    item->cloud_properties.value_type = value_type_electric_current;
    item->cloud_properties.scale = scales_ampere;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_gpio_out_str, item->interface.gpio.gpio_out.gpio_num);
}

static void __setup_voltage_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_voltage;
    item->cloud_properties.value_type = value_type_electric_potential;
    item->cloud_properties.scale = scales_volt;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_gpio_out_str, item->interface.gpio.gpio_out.gpio_num);
}

static ezlopi_error_t __prepare_smart_plug_device_item(uint32_t *parent_id, cJSON* cjson_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cjson_device, "Smart Plug");
    if (device)
    {
        error = EZPI_SUCCESS;
        __setup_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00114_Moko_Smart_Plug);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            item->user_arg = false;
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
    }
    else
    {
        error = EZPI_FAILED;
    }

    return error;
}

static ezlopi_error_t __prepare_metering_device_item(uint32_t parent_id, cJSON* cjson_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cjson_device, "Metering Device");
    if (device)
    {
        error = EZPI_SUCCESS;
        __setup_metering_device_cloud_properties(device);
        l_ezlopi_item_t *item1 = EZPI_core_device_add_item_to_device(device, device_00114_Moko_Smart_Plug);
        if (item1)
        {
            item1->user_arg = malloc(sizeof(bool));
            __setup_voltage_item_properties(item1, cjson_device);
            item1->user_arg = false;
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
        
        l_ezlopi_item_t *item2 = EZPI_core_device_add_item_to_device(device, device_00114_Moko_Smart_Plug);
        if (item2)
        {
            item2->user_arg = malloc(sizeof(bool));
            __setup_current_item_properties(item2, cjson_device);
            item2->user_arg = false;
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
    }
    else
    {
        error = EZPI_FAILED;
    }

    return error;
}