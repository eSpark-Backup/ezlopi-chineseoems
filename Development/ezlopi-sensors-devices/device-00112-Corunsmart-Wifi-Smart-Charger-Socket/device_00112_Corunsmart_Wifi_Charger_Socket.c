#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "device_00112_Corunsmart_Wifi_Charger_Socket.h"

static uint32_t master_item_id = 0;

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);
void button_task(void *args);

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
    uint32_t parent_id = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    
    if (prep_arg)
    {
        if (-1 == (error = __prepare_master_control_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (-1 == (error = __prepare_switch_one_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE FIRST SWITCH DEVICE");
        }
        if (-1 == (error = __prepare_switch_two_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE SECOND SWITCH DEVICE");
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
        // gpio_set_direction(WiFi_LED  , GPIO_MODE_OUTPUT);
        // gpio_set_direction(CTRL_LED  , GPIO_MODE_OUTPUT);
        // gpio_set_direction(INDICATOR1, GPIO_MODE_OUTPUT);
        // gpio_set_direction(INDICATOR2, GPIO_MODE_OUTPUT);
        // gpio_set_direction(INDICATOR3, GPIO_MODE_OUTPUT);

        if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_out.gpio_num) &&
            (255 != item->interface.gpio.gpio_out.gpio_num))
        {
            // if (perimanGetPinBus(item->interface.gpio.gpio_out.gpio_num, ESP32_BUS_TYPE_GPIO) == NULL) {
            //     perimanSetBusDeinit(ESP32_BUS_TYPE_GPIO, gpioDetachBus);
            //     if (!perimanClearPinBus(item->interface.gpio.gpio_out.gpio_num)) {
            //       log_e("Deinit of previous bus from IO %i failed", item->interface.gpio.gpio_out.gpio_num);
            //       return;
            //     }
            //   }
            const gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << item->interface.gpio.gpio_out.gpio_num),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = ((item->interface.gpio.gpio_out.pull == GPIO_PULLUP_ONLY) ||
                               (item->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
                                  ? GPIO_PULLUP_ENABLE
                                  : GPIO_PULLUP_DISABLE,
                .pull_down_en = ((item->interface.gpio.gpio_out.pull == GPIO_PULLDOWN_ONLY) ||
                                 (item->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
                                    ? GPIO_PULLDOWN_ENABLE
                                    : GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };

            xTaskCreate(button_task, "button_task", 1024 * 3, NULL, 1, NULL);
            if (0 == gpio_config(&io_conf))
            {
                item->user_arg = 0;
                gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
                error = EZPI_SUCCESS;
            }
            else
            {
                error = EZPI_FAILED;
            }
        }
    }

    if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_in.gpio_num) &&
        (-1 != item->interface.gpio.gpio_in.gpio_num) &&
        (255 != item->interface.gpio.gpio_in.gpio_num))
    {
        // if (perimanGetPinBus(interface.gpio.gpio_in.gpio_num, ESP32_BUS_TYPE_GPIO) == NULL) {
        //     perimanSetBusDeinit(ESP32_BUS_TYPE_GPIO, gpioDetachBus);
        //     if (!perimanClearPinBus(item->interface.gpio.gpio_in.gpio_num)) {
        //       log_e("Deinit of previous bus from IO %i failed", item->interface.gpio.gpio_in.gpio_num);
        //       return;
        //     }
        //   }
        const gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << item->interface.gpio.gpio_in.gpio_num),
            .mode = GPIO_MODE_INPUT,//};
            .pull_up_en = ((item->interface.gpio.gpio_in.pull == GPIO_PULLUP_ONLY) ||
                           (item->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
                              ? GPIO_PULLUP_ENABLE
                              : GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            // .pull_down_en = ((item->interface.gpio.gpio_in.pull == GPIO_PULLDOWN_ONLY) ||
            //                  (item->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
            //                     ? GPIO_PULLDOWN_ENABLE
            //                     : GPIO_PULLDOWN_DISABLE,
            .intr_type = (GPIO_PULLUP_ONLY == item->interface.gpio.gpio_in.pull)
                             ? GPIO_INTR_POSEDGE
                             : GPIO_INTR_NEGEDGE,
        };

        TRACE_D("enabling interrup for pin: %d", item->interface.gpio.gpio_in.gpio_num);
        TRACE_D("Pull for pin: %d", item->interface.gpio.gpio_in.pull);

        gpio_config(&io_conf);
        // EZPI_service_gpioisr_register_v3(item, __interrupt_upcall, 350);
        // if(__led_handle == NULL)
        // {
        //     xTaskCreate(indicator_led_task, "indicator_led", 1024 * 3, NULL, 5, &__led_handle);
        // }
        error = EZPI_SUCCESS;
    }

    return error;
}

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    TRACE_I("--------------SET VALUE--------------");
    if (item && arg)
    {
        int value = 0;
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            if (cj_value)
            {
                switch (cj_value->type)
                {
                    case cJSON_False: {
                        value = 0;
                        break;
                    }
                    case cJSON_True: {
                        value = 1;
                        break;
                    }
                    case cJSON_Number: {
                        value = cj_value->valuedouble;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                // start_time = esp_timer_get_time();
                l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
                if(device_head)
                {
                    l_ezlopi_item_t* master_item = device_head->items;
                    l_ezlopi_item_t* second_item = device_head->next->items;
                    l_ezlopi_item_t* third_item  = device_head->next->next->items;
                    if(second_item && third_item)
                    {
                        if(item->cloud_properties.item_id == master_item_id)
                        {   
                            item->user_arg = value;
                            if (value) 
                            {
                                gpio_set_level(second_item->interface.gpio.gpio_out.gpio_num, second_item->user_arg);
                                gpio_set_level(third_item->interface.gpio.gpio_out.gpio_num,  third_item->user_arg);
                                // second_item->user_arg = item->user_arg;
                                // third_item->user_arg  = item->user_arg;
                                EZPI_core_device_value_updated_from_device_broadcast(item);
                                EZPI_core_device_value_updated_from_device_broadcast(third_item);
                                EZPI_core_device_value_updated_from_device_broadcast(second_item);
                            } 
                            else
                            {
                                gpio_set_level(second_item->interface.gpio.gpio_out.gpio_num, item->user_arg);
                                gpio_set_level(third_item->interface.gpio.gpio_out.gpio_num,  item->user_arg);
                                // second_item->user_arg = item->user_arg;
                                // third_item->user_arg  = item->user_arg;
                                EZPI_core_device_value_updated_from_device_broadcast(item);
                                EZPI_core_device_value_updated_from_device_broadcast(third_item);
                                EZPI_core_device_value_updated_from_device_broadcast(second_item);
                            }                         
                        }
                        else if (master_item->user_arg)
                        {   
                            item->user_arg = value;
                            gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
                            EZPI_core_device_value_updated_from_device_broadcast(item);
                            // if(second_item->user_arg && third_item->user_arg)
                            // {
                            //     device_head->items->user_arg = 1;
                            //     EZPI_core_device_value_updated_from_device_broadcast(device_head->items);
                            // }
                            // else
                            // {
                            //     device_head->items->user_arg = 0;
                            //     EZPI_core_device_value_updated_from_device_broadcast(device_head->items);
                            // }
                        }
                    }
                }
            }
        }
    }
    return error;
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;
    if (item && args)
    {
        cJSON* cj_properties = (cJSON*)args;
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        l_ezlopi_item_t* master_item = device_head->items;
        if (master_item->user_arg)
        {
            gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
        }
        if (cj_properties)
        {
            EZPI_core_valueformatter_bool_to_cjson(cj_properties, item->user_arg, item->cloud_properties.scale);
            error = EZPI_SUCCESS;
        }
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

static void __setup_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    int tmp_var = 0;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);

    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_is_ip_str, item->interface.gpio.gpio_in.enable);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_ip_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_ip_str, item->interface.gpio.gpio_in.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_ip_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    TRACE_I("%u",item->interface.gpio.gpio_in.pull);
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_DISABLE;

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_op_str, tmp_var);
    item->interface.gpio.gpio_out.interrupt = GPIO_INTR_DISABLE;
    item->interface.gpio.gpio_out.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
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
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00112_Corunsmart_Wifi_Charger_Socket);
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
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00112_Corunsmart_Wifi_Charger_Socket);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio1_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio3_str, item->interface.gpio.gpio_out.gpio_num);
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
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00112_Corunsmart_Wifi_Charger_Socket);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio2_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio4_str, item->interface.gpio.gpio_out.gpio_num);
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

// void indicator_led_task(void *args)
// {
//     l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
//     if(device_head)
//     {
//         l_ezlopi_item_t* second_item = device_head->next->items;
//         l_ezlopi_item_t* third_item  = device_head->next->next->items;
//         while(1)
//         {
//             gpio_set_level(CTRL_LED, 1);
//             gpio_set_level(INDICATOR1, second_item->user_arg);
//             gpio_set_level(INDICATOR2,third_item->user_arg);
//             vTaskDelay(50 / portTICK_PERIOD_MS);
//         }
//     }
// }

void button_task(void *args) {
    l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
    if(device_head)
    {
        l_ezlopi_item_t* second_item = device_head->next->items;
        l_ezlopi_item_t* third_item  = device_head->next->next->items;
        while(1)
        {
            if (gpio_get_level(second_item->interface.gpio.gpio_in.gpio_num) == 0) {
                second_item->user_arg = !(second_item->user_arg);
                EZPI_core_device_value_updated_from_device_broadcast(second_item);
            }
            if (gpio_get_level(third_item->interface.gpio.gpio_in.gpio_num) == 0) {
                third_item->user_arg = !(third_item->user_arg);
                EZPI_core_device_value_updated_from_device_broadcast(third_item);
            }
            
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}