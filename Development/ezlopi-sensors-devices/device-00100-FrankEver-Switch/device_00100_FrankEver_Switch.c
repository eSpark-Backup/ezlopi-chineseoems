#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00100_FrankEver_Switch.h"
#include "ezlopi_core_device_value_updated.h"

#define BLUE_LED1 GPIO_NUM_22
#define RGB_RED GPIO_NUM_26
#define RGB_BLUE GPIO_NUM_17
#define RGB_GREEN GPIO_NUM_23
#define WiFi_LED GPIO_NUM_27

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void __interrupt_call(void* arg);
static void __toggle_gpio(l_ezlopi_item_t* item);
static void __write_gpio_value(l_ezlopi_item_t* item);
static void __set_gpio_value(l_ezlopi_item_t* item, int value);

ezlopi_error_t device_00100_FrankEver_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    ezlopi_error_t error = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        error = __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        error = __init(item);
        break;
    }
    case EZLOPI_ACTION_SET_VALUE:
    {
        error = __set_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        error = __get_value_cjson(item, arg);
        break;
    }

    default:
    {
        break;
    }
    }

    return error;
}

static void __setup_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cjson_device)
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
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_in_str, item->interface.gpio.gpio_in.gpio_num);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_ip_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_ip_str, item->interface.gpio.gpio_in.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_ip_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_DISABLE;

    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio_out_str, item->interface.gpio.gpio_out.gpio_num);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_op_inv_str, item->interface.gpio.gpio_out.invert);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_val_op_str, item->interface.gpio.gpio_out.value);
    CJSON_GET_VALUE_DOUBLE(cjson_device, ezlopi_pullup_op_str, tmp_var);
    item->interface.gpio.gpio_out.interrupt = GPIO_INTR_DISABLE;
    item->interface.gpio.gpio_out.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;

    TRACE_D("item->interface.gpio.gpio_in.gpio_num:  %u", item->interface.gpio.gpio_in.gpio_num);
    TRACE_D("item->interface.gpio.gpio_out.gpio_num: %u", item->interface.gpio.gpio_out.gpio_num);
}

void wifi_status_task(void *args)
{
    ezlopi_wifi_status_t *wifi_status = (ezlopi_wifi_status_t *) args;
    while (1)
    {
        int wifi_connection_stat = EZPI_core_wifi_got_ip();
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

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    s_AC_power_delay_t* power_delay = ezlopi_malloc(__FUNCTION__, sizeof(s_AC_power_delay_t));
    xTaskCreate(wifi_status_task,"wifi_status",1024 * 3,NULL,5,NULL);
    
    if (arg)
    {
        cJSON* cjson_device = prep_arg->cjson_device;
        if (cjson_device)
        {
            l_ezlopi_device_t* device = EZPI_core_device_add_device(cjson_device, NULL);
            if (device)
            {
                __setup_device_cloud_properties(device, cjson_device);
                l_ezlopi_item_t* item = EZPI_core_device_add_item_to_device(device, device_00100_FrankEver_Switch);
                if (item)
                {
                    __setup_item_properties(item, cjson_device);
                    item->user_arg = power_delay;
                    error = EZPI_SUCCESS;
                }
                else
                {
                    EZPI_core_device_free_device(device);
                    error = EZPI_FAILED;
                }
            }
        }
    }

    return error;
}

static ezlopi_error_t __init(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_ERR_INIT_DEVICE_FAILED;
    if (item)
    {
        gpio_set_direction(BLUE_LED1,GPIO_MODE_OUTPUT);
        gpio_set_direction(RGB_BLUE,GPIO_MODE_OUTPUT);
        gpio_set_direction(RGB_GREEN,GPIO_MODE_OUTPUT);
        gpio_set_direction(RGB_RED,GPIO_MODE_OUTPUT);
        gpio_set_direction(WiFi_LED,GPIO_MODE_OUTPUT);

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
                __write_gpio_value(item);
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
        EZPI_service_gpioisr_register_v3(item, __interrupt_call, 350);
        error = EZPI_SUCCESS;
    }

    if (0 == error)
    {
        error = EZPI_FAILED;
        if (item->user_arg)
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

            if (0 == gpio_config(&io_conf))
            {
                EZPI_service_gpioisr_register_v3(item, __interrupt_call, 1000);
                error = EZPI_SUCCESS;
            }
            else
            {
                error = EZPI_FAILED;
            }
        }
        else
        {
            error = EZPI_FAILED;
        }
    }

    return error;
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if (item && arg)
    {
        cJSON* cj_propertise = (cJSON*)arg;
        if (cj_propertise)
        {
            EZPI_core_valueformatter_bool_to_cjson(cj_propertise, item->interface.gpio.gpio_out.value, item->cloud_properties.scale);
            error = EZPI_SUCCESS;
        }
    }
    return error;
}

static void __set_gpio_value(l_ezlopi_item_t* item, int value)
{
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, value);
    if(value == 1)
        gpio_set_level(BLUE_LED1,0);
    else
        gpio_set_level(BLUE_LED1,1);
    item->interface.gpio.gpio_out.value = value;
}

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    ((s_AC_power_delay_t*) item->user_arg)->time_delay = esp_timer_get_time();
    TRACE_I("--------------SET VALUE--------------%lld",((s_AC_power_delay_t*) item->user_arg)->time_delay);
    if (item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;

        if (NULL != cjson_params)
        {
            int value = 0;
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
            }

            if (255 != item->interface.gpio.gpio_out.gpio_num)
            {
                if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_out.gpio_num))
                {
                    __set_gpio_value(item, value);
                    EZPI_core_device_value_updated_from_device_broadcast(item);
                    error = EZPI_SUCCESS;
                }
            }
            else
            {
                l_ezlopi_device_t* curr_device = EZPI_core_device_get_head();
                while (curr_device)
                {
                    l_ezlopi_item_t* curr_item = curr_device->items;
                    while (curr_item)
                    {
                        if ((EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT == curr_item->interface_type) && (255 != curr_item->interface.gpio.gpio_out.gpio_num))
                        {
                            TRACE_D("GPIO-pin: %d", curr_item->interface.gpio.gpio_out.gpio_num);
                            TRACE_D("value: %d", value);
                            __set_gpio_value(curr_item, value);
                            EZPI_core_device_value_updated_from_device_broadcast(curr_item);
                            error = EZPI_SUCCESS;
                        }
                        curr_item = curr_item->next;
                    }
                    curr_device = curr_device->next;
                }
                item->interface.gpio.gpio_out.value = value;
                EZPI_core_device_value_updated_from_device_broadcast(item);
            }
        }
    }
    return error;
}

static void __write_gpio_value(l_ezlopi_item_t* item)
{
    uint32_t write_value = (0 == item->interface.gpio.gpio_out.invert) ? item->interface.gpio.gpio_out.value : (item->interface.gpio.gpio_out.value ? 0 : 1);
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, write_value);
    gpio_set_level(BLUE_LED1,1);
}

void some_task(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    switch(((s_AC_power_delay_t*) item->user_arg)->count)
    {
        case 0:
            TRACE_I("-------------NO COUNTS!--------------");
            break;
        case 1:
            TRACE_I("-------------COUNT--------------%d",((s_AC_power_delay_t*) item->user_arg)->count);
            __toggle_gpio(item);
            break;
        case 2:
            ((s_AC_power_delay_t*) item->user_arg)->state = !((s_AC_power_delay_t*) item->user_arg)->state;
            TRACE_I("-------------COUNT--------------%d",((s_AC_power_delay_t*) item->user_arg)->count);
            break;
        case 3:
            TRACE_I("-------------20 Seconds!--------------");
            if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
            {
                TRACE_I("-------------DEVICE INFO ERASED FROM NVS--------------");
            }
            if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
            {
                if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY))
                TRACE_I("-------------CLOUD INFO ERASED FROM NVS--------------");
            }
            esp_restart();
            return;
    }
    if(((s_AC_power_delay_t*) item->user_arg)->state == 0 && (((s_AC_power_delay_t*) item->user_arg)->load_state == 0 || ((s_AC_power_delay_t*) item->user_arg)->load_state == 1) )
    {
        TRACE_I("-------------Purple HERE--------------");
        gpio_set_level(RGB_RED, 0);
        gpio_set_level(RGB_BLUE, 0);
        gpio_set_level(RGB_GREEN, 0);
    }
    else if(((s_AC_power_delay_t*) item->user_arg)->state == 1 && ((s_AC_power_delay_t*) item->user_arg)->load_state == 0)
    {
        TRACE_I("-------------Green HERE--------------");
        gpio_set_level(RGB_RED, ((s_AC_power_delay_t*) item->user_arg)->state);
        gpio_set_level(RGB_BLUE, 0);
        gpio_set_level(RGB_GREEN, ((s_AC_power_delay_t*) item->user_arg)->state);
    }
    else
    {
        TRACE_I("-------------Purple HERE--------------");
        gpio_set_level(RGB_RED, ((s_AC_power_delay_t*) item->user_arg)->state);
        gpio_set_level(RGB_BLUE, ((s_AC_power_delay_t*) item->user_arg)->state);
        gpio_set_level(RGB_GREEN, 0);
    }
    ((s_AC_power_delay_t*) item->user_arg)->count=0;
}

void push_button_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    bool level = 1;
    uint64_t difference = 0;
    while (level && (difference < (20*1000*1000)))
    {
        level = !gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
        vTaskDelay(100/ portTICK_RATE_MS);
        difference = esp_timer_get_time() - ((s_AC_power_delay_t*) item->user_arg)->time_first;
    }
    if (difference > (20*1000*1000))
    {
        ((s_AC_power_delay_t*) item->user_arg)->count = 3;
        some_task(item);
    }
    else if (difference < (3*1000*1000))
    {
        uint32_t delay_time = 3000000 - difference;
        vTaskDelay((delay_time/1000)/portTICK_RATE_MS);
        if (((s_AC_power_delay_t*) item->user_arg)->count == 1)
        {
            some_task(item);
        } 
    }
    else
    {
        some_task(item);
    } 
    vTaskDelete(NULL);
}

static void __interrupt_call(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    ((s_AC_power_delay_t*) item->user_arg)->interrupttime = esp_timer_get_time();
    TRACE_I("-------------ESPARK--------------%lld",((s_AC_power_delay_t*) item->user_arg)->interrupttime);
    if (item && (((s_AC_power_delay_t*) item->user_arg)->interrupttime >= (((s_AC_power_delay_t*) item->user_arg)->time_delay+30000)))//delay of 30ms for AC power issue
    {
        if ((((s_AC_power_delay_t*) item->user_arg)->count) && (esp_timer_get_time() - ((s_AC_power_delay_t*) item->user_arg)->time_first < (3 * 1000 * 1000)))
        {
            ((s_AC_power_delay_t*) item->user_arg)->count = 2;
            some_task(item);
        }
        else
        {
            ((s_AC_power_delay_t*) item->user_arg)->count = 1;
            ((s_AC_power_delay_t*) item->user_arg)->time_first = esp_timer_get_time();
            xTaskCreate(push_button_task,"push_button",2*2048,item,5,NULL);
        }
    }
}

static void __toggle_gpio(l_ezlopi_item_t* item)
{
    uint32_t write_value = !(item->interface.gpio.gpio_out.value);
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, write_value);
    if(write_value == 1)
    { 
        gpio_set_level(BLUE_LED1,0);
        ((s_AC_power_delay_t*) item->user_arg)->load_state = 1;
    }
    else
    {
        gpio_set_level(BLUE_LED1,1);
        ((s_AC_power_delay_t*) item->user_arg)->load_state = 0;
    }
    item->interface.gpio.gpio_out.value = write_value;
}