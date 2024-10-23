#include <stdlib.h>
#include <string.h>
#include "ezlopi_util_trace.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"

#include "dimmer.h"
#include "device_00104_Frankever_dimmer.h"

#define GREEN GPIO_NUM_21
bool switch_update = false;

typedef struct dimmer_data{
    bool switch_value;
    uint8_t dimmer_value;
}dimmer_data_t;

dimmer_data_t Dimmer_data;

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __notify(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

int device_00104_Frankever_dimmer(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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

static int __set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            l_ezlopi_device_t* device = ezlopi_device_get_head();
            l_ezlopi_item_t* dimmer_item = device->items;
            l_ezlopi_item_t* switch_item = dimmer_item->next;
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            ret = 1;
            if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
            {
                send_command_dimmer(cj_value->valueint);
                Dimmer_data.dimmer_value = cj_value->valueint;
                if(Dimmer_data.dimmer_value > 0)
                {
                    Dimmer_data.switch_value = true;
                }
                else{
                    Dimmer_data.switch_value = false;
                }
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                send_command_switch(cj_value->valueint);
                Dimmer_data.switch_value = cj_value->valueint;
                if(Dimmer_data.switch_value)
                {
                    Dimmer_data.dimmer_value = 100;
                }
                else
                {
                    Dimmer_data.dimmer_value = 0;
                }
            }
            ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
            ezlopi_device_value_updated_from_device_broadcast(switch_item);
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
TRACE_I("###################################################################################");
    ret = 1;
    if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_uint32_to_cjson(cj_params, Dimmer_data.dimmer_value, item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, Dimmer_data.switch_value, item->cloud_properties.scale);
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
            if (false == dimmer_init(&item->interface.uart))
            {
                ret = -1;
            }
            else
            {
                send_command_switch(false);
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

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cj_device)
{
    device->cloud_properties.category = category_dimmable_light;
    device->cloud_properties.subcategory = subcategory_dimmable_bulb;
    device->cloud_properties.device_type = dev_type_dimmer_outlet;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_dimmer_item_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_dimmer;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();

    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->interface.uart.enable = true;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);//9600
}

static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.show = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    // CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_str, item->interface.gpio.gpio_out.gpio_num);
    item->interface.gpio.gpio_out.gpio_num = GREEN;
    item->interface.gpio.gpio_in.enable = false;

    item->interface.gpio.gpio_out.enable = true;
    item->interface.gpio.gpio_out.interrupt = false;
    item->interface.gpio.gpio_out.invert = false;
    item->interface.gpio.gpio_out.mode = GPIO_MODE_OUTPUT;
    item->interface.gpio.gpio_out.pull = false;
    item->interface.gpio.gpio_out.value = true;
}

static int __prepare(void* arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        // dimmer_data_t *Dimmer_data = (dimmer_data_t*)ezlopi_malloc(__FUNCTION__, sizeof(dimmer_data_t));
        l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, NULL);
        if (device)
        {
            ret = 1;
            // memset(Dimmer_data, 0, sizeof(dimmer_data_t));
            __prepare_device_cloud_properties(device, cj_device);

            l_ezlopi_item_t* dimmer_item = ezlopi_device_add_item_to_device(device, device_00104_Frankever_dimmer);
            if (dimmer_item)
            {
                __prepare_dimmer_item_properties(dimmer_item, cj_device);
            }

            l_ezlopi_item_t* toggle_item = ezlopi_device_add_item_to_device(device, device_00104_Frankever_dimmer);
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