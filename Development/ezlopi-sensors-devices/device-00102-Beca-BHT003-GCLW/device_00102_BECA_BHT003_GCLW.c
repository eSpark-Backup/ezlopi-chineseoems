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

#include "bht003.h"
#include "device_00102_BECA_BHT003_GCLW.h"

static const char* power_state_token[] ={
    "power_on",
    "power_off",
    "energy_saving_on",
    "energy_saving_off",
    "unknown",
};

static const char* child_lock_token[] ={
    "child_lock_enable",
    "child_lock_disable",
    "unknown",
};

static const char* mode_select_token[] = {
    "manual_mode_enable",
    "program_mode_enable",
    "unknown",
};

static int device_beca_bht003_prepare_v3(void* arg);
static int device_beca_bht003_init_v3(l_ezlopi_item_t* item);
static int device_beca_bht003_notify_v3(l_ezlopi_item_t* item);
static int device_beca_bht003_set_value(l_ezlopi_item_t* item, void* arg);
static int device_beca_bht003_get_item(l_ezlopi_item_t* item, void* arg);
static int device_beca_bht003_get_value_cjson_v3(l_ezlopi_item_t* item, void* arg);

static int __prepare_beca_bht003_ambient_temp_device_item(uint32_t *parent_id, cJSON* cj_device);// parent device
static int __prepare_beca_bht003_temp_sp_device_item(uint32_t parent_id, cJSON* cj_device); 
static int __prepare_beca_bht003_power_device_item(uint32_t parent_id, cJSON* cj_device);
static int __prepare_beca_bht003_child_lock_device_item(uint32_t parent_id, cJSON* cj_device);
static int __prepare_beca_bht003_mode_select_device_item(uint32_t parent_id, cJSON* cj_device);

int device_00102_BECA_BHT003_GCLW(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
{
    int ret = 0;

    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            ret = device_beca_bht003_prepare_v3(args);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            ret = device_beca_bht003_init_v3(item);
            break;
        }
        case EZLOPI_ACTION_SET_VALUE:
        {
            ret = device_beca_bht003_set_value(item, args);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        {
            ret =  device_beca_bht003_get_item(item,args);
        }
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            ret = device_beca_bht003_get_value_cjson_v3(item, args);
            break;
        }
        case EZLOPI_ACTION_NOTIFY_1000_MS:
        {
            ret = device_beca_bht003_notify_v3(item);
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

static int device_beca_bht003_set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            ret = 1;
            if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
            {
                send_temp_sp(cj_value->valuedouble,CMD_TEMP);
            }
            else if (ezlopi_item_name_power_mode == item->cloud_properties.item_name)
            {
                if(!strcmp(cj_value->valuestring, power_state_token[0]))
                {
                    send_command(CMD_TURN_ON);
                }
                else if(!strcmp(cj_value->valuestring, power_state_token[1]))
                {
                    send_command(CMD_TURN_OFF);
                }
                else if(!strcmp(cj_value->valuestring, power_state_token[2]))
                {
                    send_command(CMD_ENERGY_SAVING_ON);
                }
                else if(!strcmp(cj_value->valuestring, power_state_token[3]))
                {
                    send_command(CMD_ENERGY_SAVING_OFF);
                }
            }
            else if(ezlopi_item_name_child_lock == item->cloud_properties.item_name)
            {
                if(!strcmp(cj_value->valuestring, child_lock_token[0]))
                {
                    send_command(CMD_LOCK_ENABLE);
                }
                else if(!strcmp(cj_value->valuestring, child_lock_token[1]))
                {
                    send_command(CMD_LOCK_DISABLE);
                }
            }
            else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name)
            {
                if(!strcmp(cj_value->valuestring, mode_select_token[0]))
                {
                    send_command(CMD_MANUAL_MODE);
                }
                else if(!strcmp(cj_value->valuestring, mode_select_token[1]))
                {
                    send_command(CMD_PROGRAM_MODE);
                }
            }
            ezlopi_device_value_updated_from_device_broadcast(item);
        }
    }
    return ret;
}

static int device_beca_bht003_get_item(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (ezlopi_item_name_power_mode == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(power_state_token)/ sizeof(power_state_token[0]);
            __add_enums_to_json(cj_result, power_state_token, size);
        }
        else if (ezlopi_item_name_child_lock == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(child_lock_token)/ sizeof(child_lock_token[0]);
            __add_enums_to_json(cj_result, child_lock_token, size); 
        }
        else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(mode_select_token)/ sizeof(mode_select_token[0]);
            __add_enums_to_json(cj_result, mode_select_token, size);
        }
        ret = 1;
    }
    return ret;
}

static int device_beca_bht003_get_value_cjson_v3(l_ezlopi_item_t* item, void* args)
{
    int ret = 0;

    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return ret;
    }

    ret = 1;

    if (ezlopi_item_name_temp == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_float_to_cjson(cj_params, bht003_amb_temp_value(), item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_float_to_cjson(cj_params, bht003_temp_sp_value(), item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_power_mode == item->cloud_properties.item_name)
    {   
        if(bht003_power_value() == true)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[0]);
            
            if(bht003_energy_saving_value() == true)
            {
                cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[2]);
                cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[2]);    
            }
            else
            {
                cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[2]);
                cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[2]);    
            }
        }
        else if(bht003_power_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[1]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[4]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[4]);
        }
    }
    else if(ezlopi_item_name_child_lock == item->cloud_properties.item_name)
    {
        if(bht003_child_lock_value() == true)
        {    
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[0]);
        }
        else if(bht003_child_lock_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[1]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[2]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[2]);
        }
    }
    else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name)
    {
        if(bht003_mode_selection_value() == true)
        {    
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[0]);
        }
        else if(bht003_mode_selection_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[1]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[2]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[2]);
        }
    }
    
    return ret;
}

static int device_beca_bht003_notify_v3(l_ezlopi_item_t* item)
{
    int ret = 0;
    if(item)
        ezlopi_device_value_updated_from_device_broadcast(item);
    return ret;
}

static int device_beca_bht003_init_v3(l_ezlopi_item_t* item)
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
            if (false == bht003_init(&item->interface.uart))
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

static void __prepare_ambient_temp_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_temperature;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_ambient_temp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_temp;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_temp_sp_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_hvac;
    device->cloud_properties.subcategory = subcategory_hvac;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_temp_sp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_setpoint;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_power_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_power;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_power_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_power_mode;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_child_lock_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_security_sensor;
    device->cloud_properties.subcategory = subcategory_child_lock;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_child_lock_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_child_lock;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_mode_select_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_generic_sensor;
    device->cloud_properties.subcategory = subcategory_mode_selection;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_mode_select_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_program_mode_selection;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static int device_beca_bht003_prepare_v3(void* arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (-1 == (ret = __prepare_beca_bht003_ambient_temp_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (-1 == (ret = __prepare_beca_bht003_temp_sp_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE TEMP SP DEVICE");
        }
        if (-1 == (ret = __prepare_beca_bht003_power_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE POWER DEVICE");
        }
        if (-1 == (ret = __prepare_beca_bht003_child_lock_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE CHILD LOCK DEVICE");
        }
        if (-1 == (ret = __prepare_beca_bht003_mode_select_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE MODE SELECT CHILD DEVICE");
        }
    }

    return ret;
}

static int __prepare_beca_bht003_ambient_temp_device_item(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    int ret = 0;

    l_ezlopi_device_t *device= ezlopi_device_add_device(cj_device, "Ambient Temperature");
    if (device)
    {
        ret = 1;
         __prepare_ambient_temp_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_ambient_temp_item_cloud_properties(item, cj_device);
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

static int __prepare_beca_bht003_temp_sp_device_item(uint32_t parent_id, cJSON* cj_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device, "Temp SP");
    if (device)
    {
        __prepare_temp_sp_device_cloud_properties(device);
        device->cloud_properties.device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_temp_sp_item_cloud_properties(item, cj_device);
        }

        if (!item)
        {
            ezlopi_device_free_device(device);
        }
        else
        {
            ret = 1;
        }
    }

    return ret;
}

static int __prepare_beca_bht003_power_device_item(uint32_t parent_id, cJSON* cj_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device, "Power");
    if (device)
    {
        __prepare_power_device_cloud_properties(device);
        device->cloud_properties.device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_power_item_cloud_properties(item, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int __prepare_beca_bht003_child_lock_device_item(uint32_t parent_id, cJSON* cj_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device, "Child Lock");
    if (device)
    {
        __prepare_child_lock_device_cloud_properties(device);
        device->cloud_properties.device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_child_lock_item_cloud_properties(item, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int __prepare_beca_bht003_mode_select_device_item(uint32_t parent_id, cJSON* cj_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device, "Mode Select");
    if (device)
    {
        __prepare_mode_select_device_cloud_properties(device);
        device->cloud_properties.device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_mode_select_item_cloud_properties(item, cj_device);
        }
        else
        {
            ret = 1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}