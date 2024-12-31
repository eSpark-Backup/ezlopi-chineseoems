#include "xuguang_dimmer.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "device_00109_Xuguang_Wifi_Dimmer_Switch.h"

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_dimmer_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);

int device_00109_Xuguang_Wifi_Dimmer_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
{
    int ret = 0;

    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            ret = __prepare(args);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            ret = __init(item);
            break;
        }
        case EZLOPI_ACTION_SET_VALUE:
        {
            ret = __set_value(item, args);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            ret = __get_value_cjson(item, args);
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
    // xTaskCreate(wifi_status_task,"wifi_status",1024 * 3,NULL,5,NULL);
    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, NULL);
        if (device)
        {
            ret = 1;
            __prepare_device_cloud_properties(device);

            l_ezlopi_item_t* dimmer_item = ezlopi_device_add_item_to_device(device, device_00109_Xuguang_Wifi_Dimmer_Switch);
            if (dimmer_item)
            {
                __prepare_dimmer_item_properties(dimmer_item, cj_device);
            }

            l_ezlopi_item_t* toggle_item = ezlopi_device_add_item_to_device(device, device_00109_Xuguang_Wifi_Dimmer_Switch);
            if (toggle_item)
            {
                __prepare_dimmer_switch_item_properties(toggle_item, cj_device);
            }

            if ((NULL == dimmer_item) || (NULL == toggle_item))
            {
                ezlopi_device_free_device(device);
                ret = -1;
            }
        }
    }

    return ret;
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;
    static bool intitialized = false;
    if(intitialized)
    {
        return ret;
    }

    if (item)
    {
        ret = 1;
        if (item->interface.uart.enable)
        {
            if (false == xuguang_init(&item->interface.uart))
            {
                ret = -1;
            }
            else
            {
                intitialized = true;
            }
            
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static int __set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            ret = 1;
            if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
            {
                // send_intensity(cj_value->valuedouble, CMD_INTENSITY);
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                TRACE_I("I'm HERE");
                send_command(CMD_PWR, cj_value->valueint);
            }
            ezlopi_device_value_updated_from_device_broadcast(item);
        }
    }
    return ret;
}

static int __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    int ret = 0;

    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return ret;
    }

    ret = 1;

    if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_uint32_to_cjson(cj_params, xuguang_amb_temp_value(), item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, xuguang_power_value(), item->cloud_properties.scale);
    }
    
    return ret;
}

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_dimmable_light;
    device->cloud_properties.subcategory = subcategory_dimmable_bulb;
    device->cloud_properties.device_type = dev_type_dimmer_outlet;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_dimmer_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_dimmer;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}