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

#include "ariza_water_uart.h"
#include "device_00105_Ariza_Water_Detector_Alarm.h"

static const char* neo_water_leak_alarm_states[] = {
    "no_water_leak",
    "water_leak_detected",
    "unknown",
};

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static int __prepare_ariza_water_alarm_device_item(uint32_t *parent_id, cJSON* cj_device);
static void __prepare_water_alarm_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_water_alarm_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device);
static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device);
static void __prepare_battery_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device);

int device_00105_Ariza_Water_Detector_Alarm(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        {
            ret =  __get_item(item,args);
        }
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

static int __get_item(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (ezlopi_item_name_water_leak_alarm == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(neo_water_leak_alarm_states)/ sizeof(neo_water_leak_alarm_states[0]);
            __add_enums_to_json(cj_result, neo_water_leak_alarm_states, size);
        }
        ret = 1;
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

    if (ezlopi_item_name_battery == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_uint32_to_cjson(cj_params, get_battery_value(), item->cloud_properties.scale);
    }
    else if(ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, get_alarm_value(), item->cloud_properties.scale);
    }
    else if(ezlopi_item_name_water_leak_alarm == item->cloud_properties.item_name)
    {
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, neo_water_leak_alarm_states[get_alarm_value()]);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, neo_water_leak_alarm_states[get_alarm_value()]);
    }
    
    return ret;
}

void uart_task(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*) arg;
    while(1)
    {
        for (size_t i = 0; i < 3; i++)
        {
            uint8_t network_status[] = {0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02};
            send_packet(network_status);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        break;
    }
    vTaskDelete(NULL);
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
            if (false == water_alarm_init(item))
            {
                ret = -1;
            }
            else
            {
                xTaskCreate(uart_task, "uart_task", 2048, item, 10, NULL);
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

static void __prepare_water_alarm_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_security_sensor;
    device->cloud_properties.subcategory = subcategory_leak;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_water_alarm_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_water_leak_alarm;
    item->cloud_properties.value_type = value_type_token;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_security_threat;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_battery_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_battery;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.scale = scales_percent;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static int __prepare(void* arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (-1 == (ret = __prepare_ariza_water_alarm_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
    }

    return ret;
}

static int __prepare_ariza_water_alarm_device_item(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    int ret = 0;

    l_ezlopi_device_t *device= ezlopi_device_add_device(cj_device, " Water Alarm");
    if (device)
    {
        ret = 1;
         __prepare_water_alarm_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00105_Ariza_Water_Detector_Alarm);
        if (item)
        {
            __prepare_water_alarm_item_cloud_properties(item, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }

        l_ezlopi_item_t *item2 = ezlopi_device_add_item_to_device(device, device_00105_Ariza_Water_Detector_Alarm);
        if (item2)
        {
            __prepare_battery_item_cloud_properties(item2, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
        
        l_ezlopi_item_t *item3 = ezlopi_device_add_item_to_device(device, device_00105_Ariza_Water_Detector_Alarm);
        if (item3)
        {
            __prepare_security_threat_item_cloud_properties(item3, cj_device);
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