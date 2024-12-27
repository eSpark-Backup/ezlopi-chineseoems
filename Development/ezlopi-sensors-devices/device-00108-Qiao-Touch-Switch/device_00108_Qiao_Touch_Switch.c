#include "ezlopi_hal_gpio.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00108_Qiao_Touch_Switch.h"
#include "ezlopi_core_device_value_updated.h"

#define WiFi_LED   GPIO_NUM_17
#define CTRL_LED   GPIO_NUM_5
#define INDICATOR1 GPIO_NUM_2
#define INDICATOR2 GPIO_NUM_4
#define INDICATOR3 GPIO_NUM_16

static uint64_t start_time = 0;
static bool wifi_state =0;
static uint32_t master_item_id = 0;
TaskHandle_t __wifi_handle = NULL;

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void __interrupt_upcall(void* arg);
static void indicator_led_task(void *args);
static void wifi_status_task(void *args);
static int __prepare_master_control_device_item(uint32_t *parent_id, cJSON* cj_device); //parent device
static int __prepare_switch_one_device_item(uint32_t parent_id, cJSON* cj_device);
static int __prepare_switch_two_device_item(uint32_t parent_id, cJSON* cj_device);
static int __prepare_switch_three_device_item(uint32_t parent_id, cJSON* cj_device);

int device_00108_Qiao_Touch_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __init(item);
        break;
    }
    case EZLOPI_ACTION_SET_VALUE:
    {
        ret = __set_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __get_value_cjson(item, arg);
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
    if(__wifi_handle == NULL)
        xTaskCreate(wifi_status_task, "wifi_status", 1024 * 3, NULL, 5, &__wifi_handle);
    
    if (prep_arg)
    {
        if (-1 == (ret = __prepare_master_control_device_item(&parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE PARENT DEVICE");
        }
        if (-1 == (ret = __prepare_switch_one_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE FIRST SWITCH DEVICE");
        }
        if (-1 == (ret = __prepare_switch_two_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE SECOND SWITCH DEVICE");
        }
        if (-1 == (ret = __prepare_switch_three_device_item(parent_id, prep_arg->cjson_device)))
        {
            TRACE_E("UNABLE TO PREPARE THIRD SWITCH DEVICE");
        }
    }

    return ret;
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        gpio_set_direction(WiFi_LED  , GPIO_MODE_OUTPUT);
        gpio_set_direction(CTRL_LED  , GPIO_MODE_OUTPUT);
        gpio_set_direction(INDICATOR1, GPIO_MODE_OUTPUT);
        gpio_set_direction(INDICATOR2, GPIO_MODE_OUTPUT);
        gpio_set_direction(INDICATOR3, GPIO_MODE_OUTPUT);

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
                item->user_arg = 0;
                gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
                ret = 1;
            }
            else
            {
                ret = -1;
            }
        }
    }

    if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_in.gpio_num) &&
        (-1 != item->interface.gpio.gpio_in.gpio_num) &&
        (255 != item->interface.gpio.gpio_in.gpio_num))
    {
        const gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << item->interface.gpio.gpio_in.gpio_num),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = ((item->interface.gpio.gpio_in.pull == GPIO_PULLUP_ONLY) ||
                           (item->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
                              ? GPIO_PULLUP_ENABLE
                              : GPIO_PULLUP_DISABLE,
            .pull_down_en = ((item->interface.gpio.gpio_in.pull == GPIO_PULLDOWN_ONLY) ||
                             (item->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
                                ? GPIO_PULLDOWN_ENABLE
                                : GPIO_PULLDOWN_DISABLE,
            .intr_type = (GPIO_PULLUP_ONLY == item->interface.gpio.gpio_in.pull)
                             ? GPIO_INTR_POSEDGE
                             : GPIO_INTR_NEGEDGE,
        };

        TRACE_D("enabling interrup for pin: %d", item->interface.gpio.gpio_in.gpio_num);

        gpio_config(&io_conf);
        ezlopi_service_gpioisr_register_v3(item, __interrupt_upcall, 350);
        xTaskCreate(indicator_led_task, "indicator_led", 1024 * 3, NULL, 5, NULL);
        ret = 1;
    }

    return ret;
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
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();

    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_dev_type_str, item->interface_type);

    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_is_ip_str, item->interface.gpio.gpio_in.enable);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_ip_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_ip_str, item->interface.gpio.gpio_in.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_ip_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_DISABLE;

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_op_str, tmp_var);
    item->interface.gpio.gpio_out.interrupt = GPIO_INTR_DISABLE;
    item->interface.gpio.gpio_out.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
}

static void indicator_led_task(void *args)
{
    l_ezlopi_device_t* device_head = ezlopi_device_get_head();
    if(device_head)
    {
        l_ezlopi_item_t* second_item = device_head->next->items;
        l_ezlopi_item_t* third_item  = device_head->next->next->items;
        l_ezlopi_item_t* fourth_item = device_head->next->next->next->items;
        while(1)
        {
            gpio_set_level(CTRL_LED, 1);
            gpio_set_level(INDICATOR1, second_item->user_arg);
            gpio_set_level(INDICATOR2,third_item->user_arg);
            gpio_set_level(INDICATOR3,fourth_item->user_arg);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

static void wifi_status_task(void *args)
{
    ezlopi_wifi_status_t *wifi_status = (ezlopi_wifi_status_t *) args;
    while (1)
    {
        int wifi_connection_stat = ezlopi_wifi_got_ip();
        if(wifi_connection_stat)
        {    
            // TRACE_I("---------------WIFI----------%d",wifi_connection_stat);
            gpio_set_level(WiFi_LED,0);
        }
        else
        {
            // TRACE_I("---------------WIFI-NOT----------%d",wifi_connection_stat);
            gpio_set_level(WiFi_LED,1);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

static int __get_value_cjson(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_propertise = (cJSON*)arg;
        if (cj_propertise)
        {
            ezlopi_valueformatter_bool_to_cjson(cj_propertise, item->user_arg, item->cloud_properties.scale);
            ret = 1;
        }
    }
    return ret;
}

static int __set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
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
                item->user_arg = value;
                start_time = esp_timer_get_time();
                l_ezlopi_device_t* device_head = ezlopi_device_get_head();
                if(device_head)
                {
                    l_ezlopi_item_t* second_item = device_head->next->items;
                    l_ezlopi_item_t* third_item  = device_head->next->next->items;
                    l_ezlopi_item_t* fourth_item = device_head->next->next->next->items;
                    if(second_item && third_item && fourth_item)
                    {
                        if(item->cloud_properties.item_id == master_item_id)
                        {   
                            gpio_set_level(second_item->interface.gpio.gpio_out.gpio_num, item->user_arg);
                            gpio_set_level(third_item->interface.gpio.gpio_out.gpio_num,  item->user_arg);
                            gpio_set_level(fourth_item->interface.gpio.gpio_out.gpio_num, item->user_arg);
                            second_item->user_arg = item->user_arg;
                            third_item->user_arg  = item->user_arg;
                            fourth_item->user_arg = item->user_arg;
                            ezlopi_device_value_updated_from_device_broadcast(item);
                            ezlopi_device_value_updated_from_device_broadcast(second_item);
                            ezlopi_device_value_updated_from_device_broadcast(third_item);
                            ezlopi_device_value_updated_from_device_broadcast(fourth_item);
                        }
                        else
                        {
                            gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
                            ezlopi_device_value_updated_from_device_broadcast(item);
                            if(second_item->user_arg && third_item->user_arg && fourth_item->user_arg)
                            {
                                device_head->items->user_arg = 1;
                                ezlopi_device_value_updated_from_device_broadcast(device_head->items);
                            }
                            else
                            {
                                device_head->items->user_arg = 0;
                                ezlopi_device_value_updated_from_device_broadcast(device_head->items);
                            }
                        }
                    }
                }
            }
        }
    }
    return ret;
}

static void factory_reset_task(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    while(1)
    {
        if(item)
        {
            if(((esp_timer_get_time() - start_time) > (20*1000*1000)) && (!gpio_get_level(item->interface.gpio.gpio_in.gpio_num)))
            {
                TRACE_I("FACTORY RESET");
                if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                }
                if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY));
                }
                esp_restart();
            }
            else if(gpio_get_level(item->interface.gpio.gpio_in.gpio_num))
            {
                TRACE_I("NO RESET-------------%d",item->interface.gpio.gpio_in.gpio_num);
                vTaskDelete(NULL);
            }
            else
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }
}

static void __interrupt_upcall(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    TRACE_I("-------------ESPARK--------------");
    if (esp_timer_get_time() - start_time > (500*1000))
    {
        xTaskCreate(factory_reset_task, "factory_reset", 1024 * 3, arg, 5, NULL);
        start_time = esp_timer_get_time();
        item->user_arg = !item->user_arg;
        gpio_set_level(item->interface.gpio.gpio_out.gpio_num,item->user_arg);
        ezlopi_device_value_updated_from_device_broadcast(item);
    }
}

static int __prepare_master_control_device_item(uint32_t *parent_id, cJSON* cjson_device) // parent device
{
    int ret = 0;

    l_ezlopi_device_t *device= ezlopi_device_add_device(cjson_device, " Master Switch");
    if (device)
    {
        ret = 1;
         __setup_device_cloud_properties(device);
        *parent_id = device->cloud_properties.device_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            master_item_id = item->cloud_properties.item_id;
            item->interface.gpio.gpio_in.gpio_num  = 14;
            item->interface.gpio.gpio_out.gpio_num = 27;
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

static int __prepare_switch_one_device_item(uint32_t parent_id, cJSON* cjson_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cjson_device, " Switch 1");
    if (device)
    {
        __setup_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_in_1_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_1_str, item->interface.gpio.gpio_out.gpio_num);
            TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
            TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
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

static int __prepare_switch_two_device_item(uint32_t parent_id, cJSON* cjson_device)
{
    int ret = 0;

    l_ezlopi_device_t *device = ezlopi_device_add_device(cjson_device, " Switch 2");
    if (device)
    {
        __setup_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_in_2_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_2_str, item->interface.gpio.gpio_out.gpio_num);
            TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
            TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
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

static int __prepare_switch_three_device_item(uint32_t parent_id, cJSON* cjson_device)
{
    int ret = 0;
    

    l_ezlopi_device_t *device = ezlopi_device_add_device(cjson_device, " Switch 3");
    if (device)
    {
        __setup_device_cloud_properties(device);
        device->cloud_properties.parent_device_id = parent_id;
        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, device_00108_Qiao_Touch_Switch);
        if (item)
        {
            item->user_arg = malloc(sizeof(bool));
            __setup_item_properties(item, cjson_device);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_in_3_str, item->interface.gpio.gpio_in.gpio_num);
            CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_3_str, item->interface.gpio.gpio_out.gpio_num);
            TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
            TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
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