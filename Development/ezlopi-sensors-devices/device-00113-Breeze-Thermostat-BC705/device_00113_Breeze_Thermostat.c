#include "breeze_thermostat.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "device_00113_Breeze_Thermostat.h"

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);


static void __prepare_power_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_fan_device_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_security_lock_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_programming_mode_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_pir_mode_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_upper_limit_cloud_properties(l_ezlopi_device_t* device);
static void __prepare_lower_limit_cloud_properties(l_ezlopi_device_t* device);

static void __prepare_power_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_fan_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_security_lock_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_programming_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_pir_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_upper_limit_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);
static void __prepare_lower_limit_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device);


static ezlopi_error_t __prepare_fan_device_item(uint32_t parent_id, cJSON* cj_device); 
static ezlopi_error_t __prepare_power_device_item(uint32_t *parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_security_lock_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_temp_format_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_pir_mode_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_upper_limit_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_lower_limit_device_item(uint32_t parent_id, cJSON* cj_device);
static ezlopi_error_t __prepare_mode_device_item(uint32_t parent_id, cJSON* cj_device);

// static ezlopi_error_t __prepare_beca_bht003_ambient_temp_device_item(uint32_t parent_id, cJSON* cj_device);

void init_sequence();
bool is_reset();


ezlopi_error_t device_00113_Breeze_Thermostat(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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

void heartBeat_task(void *arg)
{
    TRACE_I("-------hb task main aagaya");

    // send_command(CMD_HEARTBEAT,0);
    // TRACE_I("Heart_beat sent");
    // vTaskDelay(500 / portTICK_PERIOD_MS);
    
    while(1)
    {
        send_command(CMD_HEARTBEAT,0);
        TRACE_I("Heart_beat sent");
        TRACE_I("Power: %d",Values.power_value);
        TRACE_I("lock: %d",Values.lock_value);
        TRACE_I("Mode: %d",Values.mode_value);
        TRACE_I("Format: %d",Values.format_value);
        TRACE_I("Fan: %d",Values.fan_value);
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void init_seq_task(void *arg)
{
    while(1)
    {
        vTaskDelay(500000 / portTICK_PERIOD_MS);
        init_sequence();
        TRACE_I("init");
    }
    vTaskDelete(NULL);
}

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        if (-1 == (error = __prepare_power_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE POWER DEVICE");
        }
        if (-1 == (error = __prepare_fan_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE FAN DEVICE");
        }
        if (-1 == (error = __prepare_security_lock_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE SECURITY LOCK DEVICE");
        }
        if (-1 == (error = __prepare_temp_format_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE TEMPERATURE FORMAT DEVICE");
        }
        if (-1 == (error = __prepare_pir_mode_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PIR MODE DEVICE");
        }
        if (-1 == (error = __prepare_upper_limit_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE UPPER LIMIT DEVICE");
        }
        if (-1 == (error = __prepare_lower_limit_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE LOWER LIMIT DEVICE");
        }
        if (-1 == (error = __prepare_mode_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE MODE DEVICE");
        }
        // if (-1 == (error = __prepare_beca_bht003_ambient_temp_device_item(parent_id, prep_arg->cjson_device)))
        // {
        //     TRACE_E("UNABLE TO PREPARE MODE DEVICE");
        // }
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
            if (false == breeze_init(&item->interface.uart)) 
            {
                error = EZPI_FAILED;
            }
            else
            {
                intitialized = true;
                if (is_reset())
                {
                    init_sequence();
                    xTaskCreate(heartBeat_task,"Heart_beat_task: ",5*512,NULL,1,NULL);
                    xTaskCreate(init_seq_task,"init_seq_task: ",6*512,NULL,1,NULL);
                }
            }
        }
    }
    else
    {
        error = EZPI_FAILED;
    }
    return error;
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
            error = EZPI_SUCCESS;
            if (ezlopi_item_name_thermostat_fan_mode == item->cloud_properties.item_name)
            {
                send_command(CMD_FAN, cj_value->valueint);
                fan_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                send_command(CMD_PWR, cj_value->valueint);
                power_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_child_lock == item->cloud_properties.item_name)
            {
                send_command(CMD_LOCK, cj_value->valueint);
                lock_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name)
            {
                send_command(CMD_FORMAT, cj_value->valueint);
                format_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_undefined == item->cloud_properties.item_name)
            {
                send_command(CMD_PIR, cj_value->valueint);
                power_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_acceleration_x_axis == item->cloud_properties.item_name)
            {
                send_command(CMD_UP_LIMIT, cj_value->valueint);
                up_limit_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_acceleration_y_axis == item->cloud_properties.item_name)
            {
                send_command(CMD_LOW_LIMIT, cj_value->valueint);
                low_limit_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name)
            {
                send_command(CMD_MODE, cj_value->valueint);
                mode_update(cj_value->valueint);
            }
            EZPI_core_device_value_updated_from_device_broadcast(item);
        }
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
    TRACE_I("--------------------------------------------------Get_value.,.,.,.");
    if (ezlopi_item_name_thermostat_fan_mode == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, breeze_fan_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------fan.,.,.,.");
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, breeze_power_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------power.,.,.,.");
    }
    else if (ezlopi_item_name_child_lock == item->cloud_properties.item_name) {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, lock_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------lock.,.,.,.");
    }
    else if (ezlopi_item_name_thermostat_setpoint == item->cloud_properties.item_name) {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, breeze_format_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------format.,.,.,.");
    }
    else if (ezlopi_item_name_program_mode_selection == item->cloud_properties.item_name) {
        EZPI_core_valueformatter_uint32_to_cjson(cj_params, breeze_mode_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------mode.,.,.,.");
    }
    else if (ezlopi_item_name_acceleration_x_axis == item->cloud_properties.item_name) {
        EZPI_core_valueformatter_uint32_to_cjson(cj_params, upper_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------upper limit: %d", upper_value());
    }
    else if (ezlopi_item_name_acceleration_y_axis == item->cloud_properties.item_name) {
        EZPI_core_valueformatter_uint32_to_cjson(cj_params, lower_value(), item->cloud_properties.scale);
        TRACE_I("--------------------------------------------------lower limit: %d", lower_value());
    }
    
    return error;
}

static void __prepare_power_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_power;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_power_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_fan_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_fan_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_fan_mode;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_security_lock_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_child_lock;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_security_lock_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_child_lock;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_temp_format_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_mode_selection;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_temp_format_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_thermostat_setpoint;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}


static void __prepare_programming_mode_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_mode;
    device->cloud_properties.subcategory = subcategory_mode_selection;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_programming_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_program_mode_selection;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_pir_mode_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_pir_mode_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_undefined;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_upper_limit_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_not_defined;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_upper_limit_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_acceleration_x_axis;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_lower_limit_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_not_defined;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_thermostat;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}


static void __prepare_lower_limit_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_acceleration_y_axis;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    item->interface.uart.enable = true;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

// static void __prepare_ambient_temp_device_cloud_properties(l_ezlopi_device_t* device)
// {
//     device->cloud_properties.category = category_hvac;
//     device->cloud_properties.subcategory = subcategory_custom_hvac;
//     device->cloud_properties.device_type = dev_type_thermostat;
//     device->cloud_properties.info = NULL;
//     device->cloud_properties.device_type_id = NULL;
// }

// static void __prepare_ambient_temp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = false;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_name = ezlopi_item_name_temp;
//     item->cloud_properties.value_type = value_type_temperature;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
//     CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
// }

// static void __prepare_temp_sp_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.item_name = ezlopi_item_name_thermostat_setpoint;
//     item->cloud_properties.value_type = value_type_temperature;
//     item->cloud_properties.scale = scales_celsius;

//     item->interface.uart.enable = true;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
//     CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
//     CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
// }



// static ezlopi_error_t __prepare_beca_bht003_ambient_temp_device_item(uint32_t parent_id, cJSON* cj_device) // parent device
// {
//     ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

//     l_ezlopi_device_t *device= EZPI_core_device_add_device(cj_device, " Ambient Temperature");
//     if (device)
//     {
//         error = EZPI_SUCCESS;
//          __prepare_ambient_temp_device_cloud_properties(device);
//          device->cloud_properties.parent_device_id = parent_id;
//         l_ezlopi_item_t *item1 = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
//         if (item1)
//         {
//             __prepare_ambient_temp_item_cloud_properties(item1, cj_device);
//         }
//         l_ezlopi_item_t *item2 = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
//         if (item2)
//         {
//             __prepare_temp_sp_item_cloud_properties(item2, cj_device);
//         }
//         else
//         {
//             error = EZPI_SUCCESS;
//             EZPI_core_device_free_device(device);
//         }
//     }
//     else
//     {
//         error = EZPI_FAILED;
//     }

//     return error;
// }




static ezlopi_error_t __prepare_power_device_item(uint32_t *parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Power");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_power_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_power_item_properties(item, cj_device);
        }
        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }
    }

    return error;
}

static ezlopi_error_t __prepare_fan_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Fan");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_fan_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_fan_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

static ezlopi_error_t __prepare_security_lock_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Security_lock");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_security_lock_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_security_lock_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}


static ezlopi_error_t __prepare_temp_format_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Temp_format");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_temp_format_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_temp_format_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}


static ezlopi_error_t __prepare_pir_mode_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "PIR_Mode");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_pir_mode_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_pir_mode_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

static ezlopi_error_t __prepare_upper_limit_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Upper_Limit");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_upper_limit_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_upper_limit_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

static ezlopi_error_t __prepare_lower_limit_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "Lower_Limit");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_lower_limit_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_lower_limit_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

static ezlopi_error_t __prepare_mode_device_item(uint32_t parent_id, cJSON* cj_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cj_device, "mode");
    if (device)
    {
        error = EZPI_SUCCESS;
        __prepare_programming_mode_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00113_Breeze_Thermostat);
        if (item)
        {
            __prepare_programming_mode_item_properties(item, cj_device);
        }

        else
        {
            error = EZPI_FAILED;
            EZPI_core_device_free_device(device);
        }

    }

    return error;
}

void init_sequence() {
    uint8_t packet2[] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05};
    uint8_t packet[] = {0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00};
    // 55 AA 00 02 00 00 01

    send_command(CMD_HEARTBEAT,0);
    TRACE_I("Heart_beat sent");
    vTaskDelay(50 / portTICK_PERIOD_MS);
    send_command(CMD_HEARTBEAT,0);
    TRACE_I("Heart_beat sent");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    uart_write_bytes(breeze_uart_object_handle->ezlopi_uart.channel, (const char*)packet2, sizeof(packet2));
    TRACE_I("third sent");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    packet2[6]=0x04;packet2[7]=0x07;
    uart_write_bytes(breeze_uart_object_handle->ezlopi_uart.channel, (const char*)packet2, sizeof(packet2));
    TRACE_I("fifth sent");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    
    for (uint8_t i=0;i<38;i++) {
        packet2[3]=0x23;packet2[6]=0x01;packet2[7]=0x24;
        uart_write_bytes(breeze_uart_object_handle->ezlopi_uart.channel, (const char*)packet2, sizeof(packet2));
        TRACE_I("sixth.%d",i+1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    

}

bool is_reset() {
    char *wifi_ssid = EZPI_core_factory_info_v3_get_ssid();
    char *wifi_password = EZPI_core_factory_info_v3_get_password();
    char *ca_certificate = EZPI_core_factory_info_v3_get_ca_certificate();
    char *ssl_private_key = EZPI_core_factory_info_v3_get_ssl_private_key();
    char *ssl_shared_key = EZPI_core_factory_info_v3_get_ssl_shared_key();
    if(wifi_ssid != NULL && wifi_password != NULL && ca_certificate != NULL && ssl_private_key != NULL && ssl_shared_key != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
    
}






























































































































