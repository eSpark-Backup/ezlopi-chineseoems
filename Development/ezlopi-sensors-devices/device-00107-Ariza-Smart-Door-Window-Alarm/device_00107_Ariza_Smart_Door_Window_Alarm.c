#include <stdlib.h>
#include <string.h>
#include "ezlopi_util_trace.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"

#include "device_00107_Ariza_Smart_Door_Window_Alarm.h"

#define BATTERY_LVL_MIN 1380
#define BATTERY_LVL_MAX 1900

static const char* ariza_door_window_states[] = {
    "dw_is_closed",
    "dw_is_opened",
    "unknown",
};

typedef struct ariza_door_window_alarm_s
{
    uint8_t red_led_pin;
    uint8_t pwr_pin;
    bool door_state;
    float battery_voltage;
}ariza_door_window_alarm_t;

ariza_door_window_alarm_t Ariza_Door_Window_Object; 
static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __notify(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_item(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static int __prepare_ariza_smart_door_window_device_item(uint32_t *parent_id, cJSON* cj_device);

int device_00107_Ariza_Smart_Door_Window_Alarm(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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

float mapValue(uint32_t input)
{
    return ((input - BATTERY_LVL_MIN) * (100.0 - 0.0)) / (BATTERY_LVL_MAX - BATTERY_LVL_MIN);
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
        if (ezlopi_item_name_dw_state == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(ariza_door_window_states)/ sizeof(ariza_door_window_states[0]);
            __add_enums_to_json(cj_result, ariza_door_window_states, size);
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
        ezlopi_valueformatter_uint32_to_cjson(cj_params, Ariza_Door_Window_Object.battery_voltage, item->cloud_properties.scale);
    }
    else if(ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, Ariza_Door_Window_Object.door_state, item->cloud_properties.scale);
    }
    else if(ezlopi_item_name_dw_state == item->cloud_properties.item_name)
    {
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, ariza_door_window_states[Ariza_Door_Window_Object.door_state]);
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, ariza_door_window_states[Ariza_Door_Window_Object.door_state]);
    }
    
    return ret;
}
//2096 - 1380 = 716 = 7.16/each percent
void reed_switch_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    bool dw_value = gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
    TRACE_I("GPIO REED SWITCH: %d", dw_value);
    while(1)
    {
        if(Ariza_Door_Window_Object.pwr_pin)
        {
            if(gpio_get_level(item->interface.gpio.gpio_in.gpio_num) == !dw_value)
            {
                gpio_set_level(Ariza_Door_Window_Object.red_led_pin, 1);
                s_ezlopi_analog_data_t ezlopi_analog_data = { .value = 0, .voltage = 0 };
                ezlopi_adc_get_adc_data(item->next->interface.adc.gpio_num, &ezlopi_analog_data);
                Ariza_Door_Window_Object.battery_voltage = mapValue(ezlopi_analog_data.value);
                if(Ariza_Door_Window_Object.battery_voltage > 100)
                {
                    Ariza_Door_Window_Object.battery_voltage =100;
                }
                else if(Ariza_Door_Window_Object.battery_voltage <= 0)
                {
                    Ariza_Door_Window_Object.battery_voltage = 0;
                }
                dw_value = !dw_value;
                Ariza_Door_Window_Object.door_state = !dw_value;
                TRACE_I("GPIO REED SWITCH: %d", dw_value);
                TRACE_I("ADC VALUE: %d\t\tVOLTAGE CALC: %.2f",ezlopi_analog_data.value, Ariza_Door_Window_Object.battery_voltage);
                gpio_set_level(Ariza_Door_Window_Object.red_led_pin, 0);
                ezlopi_device_value_updated_from_device_broadcast(item);
                ezlopi_device_value_updated_from_device_broadcast(item->next);
                ezlopi_device_value_updated_from_device_broadcast(item->next->next);
            }
        }
        vTaskDelay(10/ portTICK_RATE_MS);  
    }
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;

    if(ezlopi_item_name_dw_state == item->cloud_properties.item_name)
    {
        if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_in.gpio_num))
        {
            Ariza_Door_Window_Object.pwr_pin = GPIO_NUM_25;
            gpio_set_direction(Ariza_Door_Window_Object.pwr_pin, GPIO_MODE_INPUT);
            gpio_set_direction(Ariza_Door_Window_Object.red_led_pin,GPIO_MODE_OUTPUT);
            gpio_set_direction(item->interface.gpio.gpio_in.gpio_num, GPIO_MODE_INPUT);
            gpio_set_level(Ariza_Door_Window_Object.red_led_pin,0);

            xTaskCreate(reed_switch_task,"reed_switch", 2*2048, item, 5, NULL);
        }
    }
    else if (ezlopi_item_name_battery == item->cloud_properties.item_name)
    {
        if (GPIO_IS_VALID_GPIO(item->interface.adc.gpio_num))
        {
            TRACE_I("adc GPIO_NUM is %d", item->interface.adc.gpio_num);
            if (0 == ezlopi_adc_init(item->interface.adc.gpio_num, item->interface.adc.resln_bit))
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
    else if(ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        ret = 1;
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static void __prepare_smart_door_window_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_security_sensor;
    device->cloud_properties.subcategory = subcategory_door;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_smart_door_window_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    int tmp_var = 0;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_dw_state;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_out_str, Ariza_Door_Window_Object.red_led_pin);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_in_str, item->interface.gpio.gpio_in.gpio_num);
    TRACE_I("OUTPUT GPIO_NUM is %d", Ariza_Door_Window_Object.red_led_pin);
}

static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_security_threat;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;
}

static void __prepare_battery_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_battery;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.scale = scales_percent;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_adc_str, item->interface.adc.gpio_num);
    item->interface.adc.resln_bit = 3;//ADC 12-bit
}

static int __prepare(void* arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (-1 == (ret = __prepare_ariza_smart_door_window_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
    }

    return ret;
}

static int __prepare_ariza_smart_door_window_device_item(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    int ret = 0;

    l_ezlopi_device_t *device= ezlopi_device_add_device(cj_device, " Door Window Alarm");
    if (device)
    {
        ret = 1;
         __prepare_smart_door_window_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00107_Ariza_Smart_Door_Window_Alarm);
        if (item)
        {
            __prepare_smart_door_window_item_cloud_properties(item, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }

        l_ezlopi_item_t *item2 = ezlopi_device_add_item_to_device(device, device_00107_Ariza_Smart_Door_Window_Alarm);
        if (item2)
        {
            __prepare_battery_item_cloud_properties(item2, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
        
        l_ezlopi_item_t *item3 = ezlopi_device_add_item_to_device(device, device_00107_Ariza_Smart_Door_Window_Alarm);
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