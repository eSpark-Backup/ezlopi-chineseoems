#include "bht003.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00102_BECA_BHT003_GCLW.h"
#include "ezlopi_core_device_value_updated.h"


static const char* power_state_token[] ={
    "on",
    "off",
    "unknown",
};

static const char* energy_saving_token[] ={
    "off",
    "aggressive",
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

_temp_t Amb_temp;
_temp_t floor_temp;
_item_name_t _item_name_object;

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __notify(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static ezlopi_error_t __prepare_beca_bht003_ambient_temp_device_item(uint32_t *parent_id, cJSON* cj_device);// parent device
static ezlopi_error_t __prepare_beca_bht003_temp_sp_device_item(uint32_t parent_id, cJSON* cj_device); 
static ezlopi_error_t __prepare_beca_bht003_power_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_beca_bht003_child_lock_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_beca_bht003_mode_select_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_beca_bht003_floor_temp_device_item(uint32_t parent_id, cJSON* cj_device);

ezlopi_error_t device_00102_BECA_BHT003_GCLW(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
        {
            error =  __get_item(item,args);
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

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            if(cj_value)
            {
                error = EZPI_SUCCESS;
                if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
                {
                    send_temp_sp(cj_value->valueint,CMD_TEMP);
                }
                else if (ezlopi_item_name_thermostat_mode == item->cloud_properties.item_name)
                {
                    if(!strcmp(cj_value->valuestring, power_state_token[0]))
                    {
                        send_command(CMD_TURN_ON);
                    }
                    else if(!strcmp(cj_value->valuestring, power_state_token[1]))
                    {
                        send_command(CMD_TURN_OFF);
                    }
                }
                else if (ezlopi_item_name_thermostat_energy_saving_mode == item->cloud_properties.item_name)
                {
                    if(!strcmp(cj_value->valuestring, energy_saving_token[1]))
                    {
                        send_command(CMD_ENERGY_SAVING_ON);
                    }
                    else if(!strcmp(cj_value->valuestring, energy_saving_token[0]))
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
            }
            else{
            }
            EZPI_core_device_value_updated_from_device_broadcast(item);
        }
    }
    return error;
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
        if (ezlopi_item_name_thermostat_energy_saving_mode == item->cloud_properties.item_name)
        {
            uint8_t size = sizeof(energy_saving_token)/ sizeof(energy_saving_token[0]);
            __add_enums_to_json(cj_result, energy_saving_token, size);
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
        if(_item_name_object.first_item_name == item->cloud_properties.item_id)
        {
            EZPI_core_valueformatter_float_to_cjson(cj_params, Amb_temp.prev, item->cloud_properties.scale);
        }
        else if(_item_name_object.second_item_name == item->cloud_properties.item_id)
        {
            EZPI_core_valueformatter_float_to_cjson(cj_params, floor_temp.prev, item->cloud_properties.scale);
        }
    }
    else if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_float_to_cjson(cj_params, bht003_temp_sp_value(), item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_thermostat_mode == item->cloud_properties.item_name)
    {   
        if(bht003_power_value() == true)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[0]);
        }
        else if(bht003_power_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[1]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, power_state_token[2]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, power_state_token[2]);
        }
    }
    else if(ezlopi_item_name_thermostat_energy_saving_mode == item->cloud_properties.item_name)
    {
        if(bht003_energy_saving_value() == true)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, energy_saving_token[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, energy_saving_token[1]);    
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, energy_saving_token[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, energy_saving_token[0]);    
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
    
    return error;
}

static ezlopi_error_t __notify(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_FAILED;
    if(item && (ezlopi_item_name_temp == item->cloud_properties.item_name))
    {
        if(_item_name_object.first_item_name == item->cloud_properties.item_id)
        {
            Amb_temp.curr = bht003_amb_temp_value();
            if ((Amb_temp.curr != Amb_temp.prev))
            {
                Amb_temp.prev = Amb_temp.curr;
                EZPI_core_device_value_updated_from_device_broadcast(item);
            }
        }
        else if(_item_name_object.second_item_name == item->cloud_properties.item_id)
        {
            floor_temp.curr = bht003_floor_temp_value();
            if ((floor_temp.curr != floor_temp.prev))
            {
                floor_temp.prev = floor_temp.curr;
                EZPI_core_device_value_updated_from_device_broadcast(item);
            }
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
        error = EZPI_SUCCESS;
        if (item->interface.uart.enable)
        {
            if (false == bht003_init(&item->interface.uart))
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

static void __prepare_ambient_temp_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_hvac;
    device->cloud_properties.subcategory = subcategory_custom_hvac;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_ambient_temp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    _item_name_object.first_item_name = item->cloud_properties.item_id;
    item->cloud_properties.item_name = ezlopi_item_name_temp;
    item->cloud_properties.value_type = value_type_temperature;
    item->cloud_properties.scale = scales_celsius;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
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

static void __prepare_power_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_mode;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

static void __prepare_energy_saving_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_energy_saving_mode;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

static void __prepare_child_lock_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_lock;
    device->cloud_properties.subcategory = subcategory_child_lock;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_child_lock_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_child_lock;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

static void __prepare_mode_select_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_mode;
    device->cloud_properties.subcategory = subcategory_mode_selection;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_mode_select_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_program_mode_selection;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
}

static void __prepare_floor_temp_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_temperature;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_floor_temp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
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

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (EZPI_FAILED == (error = __prepare_beca_bht003_ambient_temp_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (EZPI_FAILED == (error = __prepare_beca_bht003_child_lock_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE CHILD LOCK DEVICE");
        }
        if (EZPI_FAILED == (error = __prepare_beca_bht003_mode_select_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE MODE SELECT CHILD DEVICE");
        }
        if (EZPI_FAILED == (error = __prepare_beca_bht003_floor_temp_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE FLOOR TEMPERATURE DEVICE");
        }
    }

    return error;
}

static ezlopi_error_t __prepare_beca_bht003_ambient_temp_device_item(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cj_device, " Ambient Temperature");
    if (device)
    {
        error = EZPI_SUCCESS;
         __prepare_ambient_temp_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item1 = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item1)
        {
            __prepare_ambient_temp_item_cloud_properties(item1, cj_device);
        }
        l_ezlopi_item_t *item2 = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item2)
        {
            __prepare_temp_sp_item_cloud_properties(item2, cj_device);
        }
        l_ezlopi_item_t *item3 = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item3)
        {
            __prepare_power_item_cloud_properties(item3, cj_device);
        }
        l_ezlopi_item_t *item4 = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item4)
        {
            __prepare_energy_saving_item_cloud_properties(item4, cj_device);
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

static ezlopi_error_t __prepare_beca_bht003_child_lock_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    
    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, " Child Lock");
    if (device)
    {
        __prepare_child_lock_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_child_lock_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
    }
    else
    {
        error = EZPI_SUCCESS;
    }

    return error;
}

static ezlopi_error_t __prepare_beca_bht003_mode_select_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    
    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, " Mode Select");
    if (device)
    {
        __prepare_mode_select_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_mode_select_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
    }
    else
    {
        error = EZPI_SUCCESS;
    }

    return error;
}

static ezlopi_error_t __prepare_beca_bht003_floor_temp_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    
    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, " Floor Temp");
    if (device)
    {
        __prepare_floor_temp_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00102_BECA_BHT003_GCLW);
        if (item)
        {
            __prepare_floor_temp_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_SUCCESS;
            EZPI_core_device_free_device(device);
        }
    }
    else
    {
        error = EZPI_SUCCESS;
    }

    return error;
}