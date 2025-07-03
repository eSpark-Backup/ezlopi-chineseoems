#include "beca10003A.h"
#include "device_00116_BECA_1000_3A_Thermostat.h"

#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"


static const char* power_state_token[] ={
    "on",
    "off",
    "unknown",
};



static const char* child_lock_token[] ={
    "child_lock_enable",
    "child_lock_disable",
    "unknown",
};

static const char* mode_select_token[] = {
    "manual_mode",
    "program_mode",
    "unknown",
};

typedef struct _temp
{
    float curr;
    float prev;
}_temp_t;

typedef struct _item_name
{
    uint32_t first_item_name;
    uint32_t second_item_name;
}_item_name_t;

_temp_t Amb_temp={0,0};
// _temp_t floor_temp;
_item_name_t _item_name_object;

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __notify(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static ezlopi_error_t __prepare_beca_temperatureConfig_device(uint32_t *parent_id, cJSON* cj_device);// parent device
// static ezlopi_error_t __prepare_beca_10003A_temp_sp_device_item(uint32_t parent_id, cJSON* cj_device); 
// static ezlopi_error_t __prepare_beca_10003A_power_device_item(uint32_t parent_id, cJSON* cj_device);
// static ezlopi_error_t __prepare_beca_10003A_child_lock_device_item(uint32_t parent_id, cJSON* cj_device);
// static ezlopi_error_t __prepare_beca_10003A_mode_select_device_item(uint32_t parent_id, cJSON* cj_device);
// static ezlopi_error_t __prepare_beca_10003A_floor_temp_device_item(uint32_t parent_id, cJSON* cj_device);


ezlopi_error_t device_00116_BECA_1000_3A(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *args, void *user_arg)
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
            // error = __set_value(item, args);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        {
            // error =  __get_item(item,args);
        }
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            error = __get_value_cjson(item, args);
            break;
        }
        case EZLOPI_ACTION_NOTIFY_1000_MS:
        {
            error = __notify(item);
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
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (EZPI_FAILED == (error = __prepare_beca_temperatureConfig_device(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }

    }

    return error;
}


static void __prepare_ambient_temp_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_hvac;
    device->cloud_properties.subcategory = subcategory_custom_hvac;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_temp_sp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_setpoint;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = scales_celsius;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

// static void __prepare_power_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.item_name = ezlopi_item_name_thermostat_mode;
//     item->cloud_properties.value_type = value_type_token;
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
//     CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
// }


static void __prepare_temp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    _item_name_object.second_item_name = item->cloud_properties.item_id;
    item->cloud_properties.item_name = ezlopi_item_name_temp;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = scales_celsius;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

// static void __prepare_Time_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     _item_name_object.second_item_name = item->cloud_properties.item_id;
//     item->cloud_properties.item_name = ezlopi_item_name_time;
//     item->cloud_properties.value_type = value_type_temperature;
//     item->cloud_properties.scale = scales_celsius;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
//     CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
// }

// static void __prepare_child_lock_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.item_name = ezlopi_item_name_child_lock;
//     item->cloud_properties.value_type = value_type_token;
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
//     CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
// }
// static void __prepare_mode_select_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.item_name = ezlopi_item_name_program_mode_selection;
//     item->cloud_properties.value_type = value_type_token;
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
//     CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
// }

static ezlopi_error_t __prepare_beca_temperatureConfig_device(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device( cj_device, "Temperature Config");
    if (device)
    {
        error = EZPI_SUCCESS;
         __prepare_ambient_temp_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item1 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        if (item1)
        {
            __prepare_temp_item_cloud_properties(item1, cj_device);
        }
        l_ezlopi_item_t *item2 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        if (item2)
        {
            __prepare_temp_sp_item_cloud_properties(item2, cj_device);
        }

        // l_ezlopi_item_t *item3 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        // if (item3)
        // {
        //     __prepare_power_item_cloud_properties(item3, cj_device);
        // }
        // l_ezlopi_item_t *item4 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        // if (item4)
        // {
        //     __prepare_child_lock_item_cloud_properties(item4, cj_device);
        // }
        // l_ezlopi_item_t *item5 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        // if (item5)
        // {
        //     __prepare_mode_select_item_cloud_properties(item5, cj_device);
        // }
        // l_ezlopi_item_t *item6 = EZPI_core_device_add_item_to_device(device, device_00116_BECA_1000_3A);
        // if (item6)
        // {
        //     __prepare_floor_temp_item_cloud_properties(item6, cj_device);
        // }
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
        error = EZPI_SUCCESS;
        if (item->interface.uart.enable)
        {
            if (false == beca10003A_init(&item->interface.uart))
            {
                error = EZPI_FAILED;

            }
            else
            {
                intitialized = true;
            }
            
        }
    }
    else
    {
        error = EZPI_FAILED;
    }
    return error;
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

static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (ezlopi_item_name_thermostat_mode == item->cloud_properties.item_name)
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
        error = EZPI_SUCCESS;
    }
    return error;
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;

    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return error;
    }

    error = EZPI_SUCCESS;

    if (ezlopi_item_name_temp == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_float_to_cjson(cj_params, Amb_temp.prev, item->cloud_properties.scale);
        if(_item_name_object.first_item_name == item->cloud_properties.item_id)
        {
            EZPI_core_valueformatter_float_to_cjson(cj_params, Amb_temp.prev, item->cloud_properties.scale);
        }
        // else if(_item_name_object.second_item_name == item->cloud_properties.item_id)
        // {
        //     EZPI_core_valueformatter_float_to_cjson(cj_params, floor_temp.prev, item->cloud_properties.scale);
        // }
    }
    else if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_float_to_cjson(cj_params, beca10003A_temp_sp_value(), item->cloud_properties.scale);
    }
    // else if (ezlopi_item_name_thermostat_mode == item->cloud_properties.item_name)
    // {   
    //     if(bht003_power_value() == true)
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[0]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[0]);
    //     }
    //     else if(bht003_power_value() == false)
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[1]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[1]);
    //     }
    //     else
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[2]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[2]);
    //     }
    // }
    // else if(ezlopi_item_name_thermostat_energy_saving_mode == item->cloud_properties.item_name)
    // {
    //     if(bht003_energy_saving_value() == true)
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, energy_saving_token[1]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, energy_saving_token[1]);    
    //     }
    //     else
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, energy_saving_token[0]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, energy_saving_token[0]);    
    //     }
    // }
    // else if(ezlopi_item_name_child_lock == item->cloud_properties.item_name)
    // {
    //     if(bht003_child_lock_value() == true)
    //     {    
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[0]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[0]);
    //     }
    //     else if(bht003_child_lock_value() == false)
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[1]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[1]);
    //     }
    //     else
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, child_lock_token[2]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, child_lock_token[2]);
    //     }
    // }
    // else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name)
    // {
    //     if(bht003_mode_selection_value() == true)
    //     {    
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[0]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[0]);
    //     }
    //     else if(bht003_mode_selection_value() == false)
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[1]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[1]);
    //     }
    //     else
    //     {
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, mode_select_token[2]);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, mode_select_token[2]);
    //     }
    // }
    
    return error;
}

static ezlopi_error_t __notify(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_FAILED;
    // ezlopi_error_t error = EZPI_SUCCESS;
    if(item && (ezlopi_item_name_temp == item->cloud_properties.item_name))
    {
        if(_item_name_object.first_item_name == item->cloud_properties.item_id)
        {
            // Amb_temp.curr = 35.3;
            if ((Amb_temp.curr != Amb_temp.prev))
            {
                Amb_temp.prev = Amb_temp.curr;
                EZPI_core_device_value_updated_from_device_broadcast(item);
            }
        }

    }
    return error;
}