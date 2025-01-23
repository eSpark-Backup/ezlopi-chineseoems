#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "scent_diffuser.h"
#include "device_00108_DADB_smart_scent_diffuser.h"

typedef struct diffuser_data{
    bool power_value;
    bool fan_value;
}diffuser_data_t;

diffuser_data_t Diffuser_data;

static ezlopi_error_t __prepare_v3(void* arg);
static ezlopi_error_t __init_v3(l_ezlopi_item_t* item);
static ezlopi_error_t __notify_v3(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson_v3(l_ezlopi_item_t* item, void* arg);

static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_device_item(uint32_t *parent_id, cJSON* cj_device);// parent device
static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_fan_device_item(uint32_t parent_id, cJSON* cj_device); 
static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_power_device_item(uint32_t *parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_timing_schedule_device_item(uint32_t parent_id, cJSON* cj_device);

ezlopi_error_t device_00108_DADB_smart_scent_diffuser(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
{
    ezlopi_error_t error = 0;

    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            error = __prepare_v3(args);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            error = __init_v3(item);
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
            error = __get_value_cjson_v3(item, args);
            break;
        }
        case EZLOPI_ACTION_NOTIFY_1000_MS:
        {
            error = __notify_v3(item);
            break;
        }

        default:
        {
            break;
        }
    }

    return error;
}

void comm_start_task(void *arg)
{
    // l_ezlopi_item_t* item = (l_ezlopi_item_t*) arg;
    vTaskDelay(7000 / portTICK_PERIOD_MS);
    // start_packet();
    send_command(CMD_CONNECTION_START);
    TRACE_I("Comm started!");
    //         while(1)
    // {
    //         send_command(CMD_HEARTBEAT);
    //         vTaskDelay(10 / portTICK_PERIOD_MS);
    // }
    vTaskDelete(NULL);
}

void heartBeat_task(void *arg)
{
    // l_ezlopi_item_t* item = (l_ezlopi_item_t*) arg;
    while(1)
    {
            send_command(CMD_HEARTBEAT);
            // heart_beat_packet();
            TRACE_I("Heart_beat sent");
            vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}


static ezlopi_error_t __prepare_v3(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        // if (-1 == (error = __prepare_dadb_smart_scent_diffuser_device_item(&parent_id, prep_arg->cjson_device)))
        // {
        //     TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        // }
        if (-1 == (error = __prepare_dadb_smart_scent_diffuser_power_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE POWER DEVICE");
        }
        if (-1 == (error = __prepare_dadb_smart_scent_diffuser_fan_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE Fan DEVICE");
        }
        if (-1 == (error = __prepare_dadb_smart_scent_diffuser_timing_schedule_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE CHILD LOCK DEVICE");
        }
    }

    return error;
}

static ezlopi_error_t __init_v3(l_ezlopi_item_t* item)
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
            if (false == scent_diffuser_init(&item->interface.uart))
            {
                error = EZPI_FAILED;
            }
            else
            {
                intitialized = true;
                xTaskCreate(comm_start_task,"com_start_task: ",2*2048,NULL,2,NULL);
                xTaskCreate(heartBeat_task,"Heart_beat_task: ",2*2048,NULL,1,NULL);
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

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            l_ezlopi_device_t* device = EZPI_core_device_get_head();
            // l_ezlopi_item_t* parent_item = device->items;
            l_ezlopi_item_t* power_item = device->items;
            l_ezlopi_item_t* fan_item = power_item->next;
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            error = EZPI_SUCCESS;
            if (ezlopi_item_name_power_state == item->cloud_properties.item_name)
            {
                CommandType cmd = (cj_value->valueint) ?  CMD_TURN_ON: CMD_TURN_OFF;
                send_command(cmd);
                TRACE_I("power main masti");
                TRACE_I("-----------------------------------------------%d",cj_value->valueint);
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                CommandType cmd = (cj_value->valueint) ?  CMD_FAN_ON: CMD_FAN_OFF;
                send_command(cmd);
                TRACE_I("fan main masti");
                TRACE_I("-----------------------------------------------%d",cj_value->valueint);
            }
            EZPI_core_device_value_updated_from_device_broadcast(power_item);
            EZPI_core_device_value_updated_from_device_broadcast(fan_item);
        }
    }
    return error;
}

static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;

    return error;
}

static ezlopi_error_t __get_value_cjson_v3(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;

    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return error;
    }

    error = EZPI_SUCCESS;
    if (ezlopi_item_name_power_state == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, Diffuser_data.power_value, item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, Diffuser_data.fan_value, item->cloud_properties.scale);
    }
    
    
    return error;
}

static ezlopi_error_t __notify_v3(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_SUCCESS;
    // if(item)
    //     ezlopi_device_value_updated_from_device_broadcast(item);
    return error;
}


static void __prepare_scent_diffuser_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_not_defined;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_scent_diffuser_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_fan_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_cooler;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_fan_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
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
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_power_state;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}

static void __prepare_timing_schedule_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_hvac;
    device->cloud_properties.subcategory = subcategory_gas;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
}

static void __prepare_timing_schedule_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_child_lock;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
}


static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_device_item(uint32_t *parent_id, cJSON* cj_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cj_device, "smart scent diffuser");
    if (device)
    {
        error = EZPI_SUCCESS;
         __prepare_scent_diffuser_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_DADB_smart_scent_diffuser);
        if (item)
        {
            __prepare_scent_diffuser_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }
    }

    return error;
}


static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_fan_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Fan");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_fan_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_DADB_smart_scent_diffuser);
        if (item)
        {
            __prepare_fan_item_cloud_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_power_device_item(uint32_t *parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Power");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_power_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_DADB_smart_scent_diffuser);
        if (item)
        {
            __prepare_power_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }
    }

    return error;
}

static ezlopi_error_t __prepare_dadb_smart_scent_diffuser_timing_schedule_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    
    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Timing schedule");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_timing_schedule_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_DADB_smart_scent_diffuser);
        if (item)
        {
            __prepare_timing_schedule_item_cloud_properties(item, cj_device);
        }
        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }
    }

    return error;
}