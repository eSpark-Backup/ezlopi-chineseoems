#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "device_00110_Qiao_Wifi_Circuit_Breaker.h"
#include "qiao_breaker.h"

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);
static int _get_item(l_ezlopi_item_t* item, void* arg);

static uint32_t switch_item_id;
static uint32_t mode_item_id;
static uint32_t lock_item_id;

static const char* qiao_mode_switch_states[] = {
    "automatic",
    "manual",
    "unknown"
};

static const char* qiao_lock_switch_states[] = {
    "lock_off",
    "lock_on",
    "unknown"
};

// const char* breaker_name = 'Breaker Switch';

static void __prepare_switch_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_mode_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_switch_lock_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_switch_lock_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);

static int __prepare_switch_device(uint32_t *parent_id, cJSON* cj_device);
static int __prepare_mode_device(uint32_t parent_id, cJSON* cj_device);
static int __prepare_switch_lock_device(uint32_t parent_id, cJSON* cj_device);


// static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);

int device_00110_Qiao_Wifi_Circuit_Breaker(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
        {
            ret = _get_item(item,args);

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

static int __prepare(void* arg)
{
    int ret = 0;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if (prep_arg)
    {
        if (-1 == (ret = __prepare_switch_device(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (-1 == (ret = __prepare_mode_device(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE MODE DEVICE");
        }
        if (-1 == (ret = __prepare_switch_lock_device(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE LOCK DEVICE");
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
            if (false == qiao_init(&item->interface.uart))
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
    if(item && arg && !(Values.mode_value))
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            ret = 1;
            if (lock_item_id == item->cloud_properties.item_id)
            {
                // send_command(cj_value->valueint);
                // lock_update(cj_value->valueint);
                TRACE_I(cj_value->valuestring);
                if(!strcmp(cj_value->valuestring, qiao_lock_switch_states[1]))
                {
                    send_command(1);
                    lock_update(1);
                }
                else if(!strcmp(cj_value->valuestring, qiao_lock_switch_states[0]))
                {
                    send_command(0);
                    lock_update(0);
                }
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

    if (switch_item_id == item->cloud_properties.item_id)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, qiao_switch_value(), item->cloud_properties.scale);
    }
    else if (mode_item_id == item->cloud_properties.item_id)
    {
        if (qiao_mode_value() == true) {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_mode_switch_states[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_mode_switch_states[1]);
        }
        else if(qiao_mode_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_mode_switch_states[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_mode_switch_states[0]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_mode_switch_states[2]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_mode_switch_states[2]);
        }
        
        // ezlopi_valueformatter_bool_to_cjson(cj_params, qiao_mode_value(), item->cloud_properties.scale);
    }
    else if (lock_item_id == item->cloud_properties.item_id)
    {
        if (qiao_lock_value() == true) {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_lock_switch_states[1]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_lock_switch_states[1]);
        }
        else if(qiao_lock_value() == false)
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_lock_switch_states[0]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_lock_switch_states[0]);
        }
        else
        {
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, qiao_lock_switch_states[2]);
            cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, qiao_lock_switch_states[2]);
        }
        
        // ezlopi_valueformatter_bool_to_cjson(cj_params, qiao_lock_value(), item->cloud_properties.scale);
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

static int _get_item(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_result = (cJSON*)arg;
        if (mode_item_id == item->cloud_properties.item_id)
        {
            uint8_t size = sizeof(qiao_mode_switch_states)/ sizeof(qiao_mode_switch_states[0]);
            __add_enums_to_json(cj_result, qiao_mode_switch_states, size);
        }
        else if (lock_item_id == item->cloud_properties.item_id)
        {
            uint8_t size = sizeof(qiao_lock_switch_states)/ sizeof(qiao_lock_switch_states[0]);
            __add_enums_to_json(cj_result, qiao_lock_switch_states, size); 
        }
        ret = 1;
    }
    return ret;
}


static int __prepare_switch_device(uint32_t *parent_id, cJSON* cj_device) {
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, " Breaker Status");
        if (device)
        {
            ret = 1;
            __prepare_switch_device_cloud_properties(device);
            *parent_id = device->cloud_properties.device_id;
            l_ezlopi_item_t* switch_item = ezlopi_device_add_item_to_device(device, device_00110_Qiao_Wifi_Circuit_Breaker);
            if (switch_item)
            {
                __prepare_switch_item_properties(switch_item, cj_device);
            }
            if ((NULL == switch_item))
            {
                ezlopi_device_free_device(device);
                ret = -1;
            }
        }

        return ret;
}

static int __prepare_mode_device(uint32_t parent_id, cJSON* cj_device) {
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, " Mode Status");
        if (device)
        {
            ret = 1;
            __prepare_mode_device_cloud_properties(device);
            device->cloud_properties.parent_device_id = parent_id;
            l_ezlopi_item_t* mode_item = ezlopi_device_add_item_to_device(device, device_00110_Qiao_Wifi_Circuit_Breaker);
            if (mode_item)
            {
                __prepare_mode_item_properties(mode_item, cj_device);
            }
            if ((NULL == mode_item))
            {
                ezlopi_device_free_device(device);
                ret = -1;
            }
        }

    return ret;
}

static int __prepare_switch_lock_device(uint32_t parent_id, cJSON* cj_device) {
    int ret = 0;
    l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, " Switch lock");
        if (device)
        {
            ret = 1;
            __prepare_switch_lock_device_cloud_properties(device);
            device->cloud_properties.parent_device_id = parent_id;
            l_ezlopi_item_t* lock_item = ezlopi_device_add_item_to_device(device, device_00110_Qiao_Wifi_Circuit_Breaker);
            if (lock_item)
            {
                __prepare_switch_lock_item_properties(lock_item, cj_device);
            }
            if ((NULL == lock_item))
            {
                ezlopi_device_free_device(device);
                ret = -1;
            }
        }

    return ret;
}



static void __prepare_switch_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_relay;
    device->cloud_properties.device_type = dev_type_switch_inwall;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;
    switch_item_id = item->cloud_properties.item_id;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_mode_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_mode;
    device->cloud_properties.subcategory = subcategory_mode_selection;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_program_mode_selection;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;
    mode_item_id = item->cloud_properties.item_id;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_switch_lock_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_lock;
    device->cloud_properties.subcategory = subcategory_child_lock;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_switch_lock_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_child_lock;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;
    lock_item_id = item->cloud_properties.item_id;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}


























































































// static void __prepare_mode_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.has_setter = false;
//     item->cloud_properties.item_name = ezlopi_item_name_switch;
//     item->cloud_properties.value_type = value_type_bool;
//     item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
//     CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
// }

// static void __prepare_switch_lock_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.item_name = ezlopi_item_name_switch;
//     item->cloud_properties.value_type = value_type_bool;
//     item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
//     CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
// }