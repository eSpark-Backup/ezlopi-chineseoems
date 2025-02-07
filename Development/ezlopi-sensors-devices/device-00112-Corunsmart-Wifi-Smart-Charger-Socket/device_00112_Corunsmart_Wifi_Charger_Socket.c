#include "xuguang_dimmer.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "device_00112_Corunsmart_Wifi_Charger_Socket.h"

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static ezlopi_error_t __prepare_master_control_device_item(uint32_t *parent_id, cJSON* cjson_device);
static ezlopi_error_t __prepare_switch_one_device_item(uint32_t parent_id, cJSON* cjson_device);
static ezlopi_error_t __prepare_switch_two_device_item(uint32_t parent_id, cJSON* cjson_device);

ezlopi_error_t device_00112_Corunsmart_Wifi_Charger_Socket(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t* device = EZPI_core_device_add_device(cj_device, NULL);
        if (device)
        {
            error = EZPI_SUCCESS;
            __prepare_device_cloud_properties(device);

            l_ezlopi_item_t* dimmer_item = EZPI_core_device_add_item_to_device(device, device_00112_Corunsmart_Wifi_Charger_Socket);
            if (dimmer_item)
            {
                __prepare_dimmer_item_properties(dimmer_item, cj_device);
            }

            l_ezlopi_item_t* toggle_item = EZPI_core_device_add_item_to_device(device, device_00112_Corunsmart_Wifi_Charger_Socket);
            if (toggle_item)
            {
                __prepare_dimmer_switch_item_properties(toggle_item, cj_device);
            }

            if ((NULL == dimmer_item) || (NULL == toggle_item))
            {
                EZPI_core_device_free_device(device);
                error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
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
            if (false == corunsmart_init(&item->interface.uart))
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
            if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
            {
                send_command(CMD_INTENSITY, cj_value->valueint);
                intensity_update(cj_value->valueint);
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                send_command(CMD_PWR, cj_value->valueint);
                power_update(cj_value->valueint);
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

    if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_uint32_to_cjson(cj_params, xuguang_amb_temp_value(), item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        EZPI_core_valueformatter_bool_to_cjson(cj_params, xuguang_power_value(), item->cloud_properties.scale);
    }
    
    return error;
}

static void __setup_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_in_wall;
    device->cloud_properties.device_type = dev_type_switch_inwall;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

// static void __prepare_device_cloud_properties(l_ezlopi_device_t* device)
// {
//     device->cloud_properties.category = category_dimmable_light;
//     device->cloud_properties.subcategory = subcategory_dimmable_bulb;
//     device->cloud_properties.device_type = dev_type_dimmer_outlet;
//     device->cloud_properties.info = NULL;
//     device->cloud_properties.device_type_id = NULL;
// }

// static void __prepare_dimmer_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
// {
//     int tmp_var = 0;
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.item_name = ezlopi_item_name_dimmer;
//     item->cloud_properties.value_type = value_type_int;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.scale = NULL;

//     item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio1_str, item->interface.gpio.gpio_in.gpio_num);
//     CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_is_ip_str, item->interface.gpio.gpio_in.enable);
//     CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_ip_inv_str, item->interface.gpio.gpio_in.invert);
//     CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_val_ip_str, item->interface.gpio.gpio_in.value);
//     CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_pullup_ip_str, tmp_var);
//     item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
//     item->interface.gpio.gpio_in.interrupt = GPIO_INTR_DISABLE;
// }

// static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
// {
//     item->cloud_properties.show = true;
//     item->cloud_properties.has_getter = true;
//     item->cloud_properties.has_setter = true;
//     item->cloud_properties.item_name = ezlopi_item_name_switch;
//     item->cloud_properties.value_type = value_type_bool;
//     item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
//     item->cloud_properties.scale = NULL;

//     item->interface.uart.enable = false;
//     item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
//     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio2_str, item->interface.gpio.gpio_out.gpio_num); //GPIO_NUM_18
// //     CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
// //     CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
// }

static ezlopi_error_t __prepare_master_control_device_item(uint32_t *parent_id, cJSON* cjson_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cjson_device, " Master Switch");
    if (device)
    {
        error = EZPI_SUCCESS;
         __setup_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            master_item_id = item->cloud_properties.item_id;
            item->interface.gpio.gpio_in.gpio_num  = 14;
            item->interface.gpio.gpio_out.gpio_num = 21;
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

static ezlopi_error_t __prepare_switch_one_device_item(uint32_t parent_id, cJSON* cjson_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cjson_device, " Switch 1");
    if (device)
    {
        __setup_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio1_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_1_str, item->interface.gpio.gpio_out.gpio_num);
            // TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
            // TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
        }

        if (!item)
        {
            EZPI_core_device_free_device(device);
        }
        else
        {
            error = EZPI_SUCCESS;
        }
    }

    return error;
}

static ezlopi_error_t __prepare_switch_two_device_item(uint32_t parent_id, cJSON* cjson_device)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device = EZPI_core_device_add_device(cjson_device, " Switch 2");
    if (device)
    {
        __setup_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio2_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_2_str, item->interface.gpio.gpio_out.gpio_num);
            // TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
            // TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
        }

        if (!item)
        {
            EZPI_core_device_free_device(device);
        }
        else
        {
            error = EZPI_SUCCESS;
        }
    }

    return error;
}