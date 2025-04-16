#include "device_00114_Moko_Smart_Plug.h"

// #define FACTORY_RESET   (5 * 1000 * 1000)
// #define OTA             (10 * 1000 * 1000)

// static const char* corunsmart_led_modes[] = {
//     "mode_1",
//     "mode_2",
//     "mode_3",
//     "mode_4"
// };

// static const char* child_lock_token[] ={
//     "child_lock_enable",
//     "child_lock_disable",
//     "unknown",
// };

// #define BLUE_LEFT   19
// #define BLUE_RIGHT  21

// TaskHandle_t button_handle = NULL;
// TaskHandle_t ota_led_handle = NULL;

// static uint32_t master_item_id = 0;
// static uint32_t pattern_item_id = 0;
// static uint32_t child_lock_item_id = 0;

// void button_task(void *args);
// static void reset_led_task(void *args);
// static void factory_reset();
// uint8_t is_reset();
// static void OTA_METHOD();
// void ota_led_task(void *args);

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);


static ezlopi_error_t __prepare_smart_plug_device_item(uint32_t *parent_id, cJSON* cjson_device);

ezlopi_error_t device_00114_Moko_Smart_Plug(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
            error = __get_item(item,args);
        }
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
        if (-1 == (error = __prepare_smart_plug_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
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
        if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_out.gpio_num) &&
            (255 != item->interface.gpio.gpio_out.gpio_num))
        {
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
            if (0 == gpio_config(&io_conf))
            {   
                error = EZPI_SUCCESS;
            }
            else
            {
                error = EZPI_FAILED;
            }
        }
    }
    
    return error;
}

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    // TRACE_I("--------------SET VALUE--------------");
    // if (item && arg)
    // {
    //     int value = 0;
    //     cJSON* cjson_params = (cJSON*)arg;
    //     if (NULL != cjson_params)
    //     {
    //         cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
    //         if (cj_value)
    //         {
    //             switch (cj_value->type)
    //             {
    //                 case cJSON_False: {
    //                     value = 0;
    //                     break;
    //                 }
    //                 case cJSON_True: {
    //                     value = 1;
    //                     break;
    //                 }
    //                 case cJSON_Number: {
    //                     value = cj_value->valuedouble;
    //                     break;
    //                 }
    //                 // case cJSON_String: {
    //                 //     // value = cj_value->valuedouble;
    //                 //     break;
    //                 // }
    //                 default: {
    //                     break;
    //                 }
    //             }
    //             // start_time = esp_timer_get_time();
    //             l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
    //             if(device_head)
    //             {
    //                 l_ezlopi_item_t* master_item = device_head->items;
    //                 l_ezlopi_item_t* second_item = device_head->next->items;
    //                 l_ezlopi_item_t* third_item  = device_head->next->next->items;
    //                 l_ezlopi_item_t* pattern_item  = device_head->next->next->next->items;
    //                 if(second_item && third_item && pattern_item)
    //                 {
    //                     if(item->cloud_properties.item_id == master_item_id)
    //                     {   
    //                         item->user_arg = value;
    //                         if (value) 
    //                         {
    //                             gpio_set_level(second_item->interface.gpio.gpio_out.gpio_num, second_item->user_arg);
    //                             gpio_set_level(third_item->interface.gpio.gpio_out.gpio_num,  third_item->user_arg);
    //                             EZPI_core_device_value_updated_from_device_broadcast(item);
    //                             EZPI_core_device_value_updated_from_device_broadcast(third_item);
    //                             EZPI_core_device_value_updated_from_device_broadcast(second_item);
    //                             EZPI_core_device_value_updated_from_device_broadcast(pattern_item);
                                
    //                         } 
    //                         else
    //                         {
    //                             gpio_set_level(second_item->interface.gpio.gpio_out.gpio_num, item->user_arg);
    //                             gpio_set_level(third_item->interface.gpio.gpio_out.gpio_num,  item->user_arg);
    //                             gpio_set_level(pattern_item->interface.gpio.gpio_out.gpio_num,  item->user_arg);
    //                             gpio_set_level(BLUE_LEFT, 0);
    //                             gpio_set_level(BLUE_RIGHT, 0);

    //                             EZPI_core_device_value_updated_from_device_broadcast(item);
    //                             EZPI_core_device_value_updated_from_device_broadcast(third_item);
    //                             EZPI_core_device_value_updated_from_device_broadcast(second_item);
    //                         }          
    //                         error = EZPI_SUCCESS;               
    //                     }
    //                     else if (item->cloud_properties.item_id == pattern_item_id)
    //                     {
    //                         if (!strcmp(cj_value->valuestring, corunsmart_led_modes[0]))
    //                         {
    //                             item->user_arg = corunsmart_led_modes[0];
    //                         }
    //                         else if (!strcmp(cj_value->valuestring, corunsmart_led_modes[1]))
    //                         {
    //                             item->user_arg = corunsmart_led_modes[1];
    //                         }
    //                         else if (!strcmp(cj_value->valuestring, corunsmart_led_modes[2]))
    //                         {
    //                             item->user_arg = corunsmart_led_modes[2];
    //                         }
    //                         else if (!strcmp(cj_value->valuestring, corunsmart_led_modes[3]))
    //                         {
    //                             item->user_arg = corunsmart_led_modes[3];
    //                         }

    //                         TRACE_I(cj_value->valuestring);
    //                         gpio_set_level(item->interface.gpio.gpio_out.gpio_num, 0);
                            
    //                         error = EZPI_SUCCESS;
    //                         EZPI_core_device_value_updated_from_device_broadcast(item);
    //                     }

    //                     else if (ezlopi_item_name_child_lock == item->cloud_properties.item_name)
    //                     {
    //                         if (!strcmp(cj_value->valuestring, child_lock_token[0]))
    //                         {
    //                             item->user_arg = child_lock_token[0];
    //                         }
    //                         else if (!strcmp(cj_value->valuestring, child_lock_token[1]))
    //                         {
    //                             item->user_arg = child_lock_token[1];
    //                         }
    //                         else
    //                         {
    //                             item->user_arg = child_lock_token[2];
    //                         }

    //                         EZPI_core_device_value_updated_from_device_broadcast(item);
    //                     }

    //                     else if (master_item->user_arg)
    //                     {   
    //                         item->user_arg = value;
    //                         gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
    //                         error = EZPI_SUCCESS;
    //                         if (pattern_item == NULL)
    //                         {
    //                            TRACE_I("pattern item null");
    //                         }
    //                         else
    //                         {
    //                             TRACE_I("pattern item not null");
    //                         }

    //                         EZPI_core_device_value_updated_from_device_broadcast(pattern_item);
    //                         EZPI_core_device_value_updated_from_device_broadcast(item);
    //                     }
                        
    //                 }
    //             }
    //         }
    //     }
    // }
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

static ezlopi_error_t __get_item(l_ezlopi_item_t* item, void* arg) {
    ezlopi_error_t ret = EZPI_FAILED;
    // if (item && arg)
    // {
    //     cJSON* cj_result = (cJSON*)arg;
    //     if (pattern_item_id == item->cloud_properties.item_id)
    //     {
    //         uint8_t size = sizeof(corunsmart_led_modes)/ sizeof(corunsmart_led_modes[0]);
    //         __add_enums_to_json(cj_result, corunsmart_led_modes, size);
    //     }
    //     else if (ezlopi_item_name_child_lock == item->cloud_properties.item_name)
    //     {
    //         uint8_t size = sizeof(child_lock_token)/ sizeof(child_lock_token[0]);
    //         __add_enums_to_json(cj_result, child_lock_token, size); 
    //     }
    //     ret = EZPI_SUCCESS;
    // }
    return ret;
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;
    // if (item && args)
    // {
    //     cJSON* cj_properties = (cJSON*)args;
    //     l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
    //     l_ezlopi_item_t* master_item = device_head->items;
    //     l_ezlopi_item_t* second_item = device_head->next->items;
    //     l_ezlopi_item_t* third_item  = device_head->next->next->items;
    //     l_ezlopi_item_t* pattern_item  = device_head->next->next->next->items;

    //     if (item->cloud_properties.item_id == pattern_item_id)
    //     {
    //         if (master_item->user_arg)
    //         {
    //             if (!strcmp(item->user_arg, corunsmart_led_modes[0]))
    //             {
    //                 gpio_set_level(item->interface.gpio.gpio_out.gpio_num, 1);
    //             }
    //             else if (!strcmp(item->user_arg, corunsmart_led_modes[1]))
    //             {
    //                 gpio_set_level(BLUE_LEFT, second_item->user_arg);
    //                 gpio_set_level(BLUE_RIGHT, third_item->user_arg);
    //             }
    //             else if (!strcmp(item->user_arg, corunsmart_led_modes[2]))
    //             {
    //                 gpio_set_level(BLUE_LEFT, !(second_item->user_arg));
    //                 gpio_set_level(BLUE_RIGHT, !(third_item->user_arg));
    //             }
    //             else if (!strcmp(item->user_arg, corunsmart_led_modes[3]))
    //             {
    //                 gpio_set_level(BLUE_LEFT, 0);
    //                 gpio_set_level(BLUE_RIGHT, 0);
    //             }
    //         }
            
    //         cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_valueFormatted_str, item->user_arg);
    //         cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_value_str, item->user_arg);
    //         error = EZPI_SUCCESS;
    //     }

    //     else if(ezlopi_item_name_child_lock == item->cloud_properties.item_name)
    //     {
    //         if(!strcmp(item->user_arg, child_lock_token[0]))
    //         {    
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_valueFormatted_str, child_lock_token[0]);
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_value_str, child_lock_token[0]);
    //         }
    //         else if(!strcmp(item->user_arg, child_lock_token[1]))
    //         {
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_valueFormatted_str, child_lock_token[1]);
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_value_str, child_lock_token[1]);
    //         }
    //         else
    //         {
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_valueFormatted_str, child_lock_token[2]);
    //             cJSON_AddStringToObject(__FUNCTION__, cj_properties, ezlopi_value_str, child_lock_token[2]);
    //         }
    //         error = EZPI_SUCCESS;
    //     }

    //     else
    //     {
    //         if (master_item->user_arg)
    //         {
    //             gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
    //             if (cj_properties)
    //             {
    //                 EZPI_core_valueformatter_bool_to_cjson(cj_properties, item->user_arg, item->cloud_properties.scale);
    //                 EZPI_core_device_value_updated_from_device_broadcast(pattern_item);
    //                 error = EZPI_SUCCESS;
    //             }
    //         }
    //         else
    //         {
    //             if (cj_properties)
    //             {
    //                 EZPI_core_valueformatter_bool_to_cjson(cj_properties, master_item->user_arg, item->cloud_properties.scale);
    //                 error = EZPI_SUCCESS;
    //             }
    //         }
            
    //     }
    // }
    return error;
}

static void __setup_device_cloud_properties(l_ezlopi_device_t* device)
{
    device->cloud_properties.category = category_switch;
    device->cloud_properties.subcategory = subcategory_relay;
    device->cloud_properties.device_type = dev_type_device;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __setup_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
}

static ezlopi_error_t __prepare_smart_plug_device_item(uint32_t *parent_id, cJSON* cjson_device) // parent device
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    l_ezlopi_device_t *device= EZPI_core_device_add_device(cjson_device, "Smart Plug");
    if (device)
    {
        error = EZPI_SUCCESS;
         __setup_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = EZPI_core_device_add_item_to_device(device, device_00114_Moko_Smart_Plug);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            item->user_arg = 1;
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