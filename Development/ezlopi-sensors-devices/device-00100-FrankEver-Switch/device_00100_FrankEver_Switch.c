#include "../../build/config/sdkconfig.h"
#include "ezlopi_util_trace.h"
#include "driver/timer.h"
#include <sys/time.h>
#include "ezlopi_core_nvs.h"
#include "esp_timer.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_hal_gpio.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_service_gpioisr.h"
#include "device_00100_FrankEver_Switch.h"
#include "EZLOPI_USER_CONFIG.h"

#define BLUE_LED1 GPIO_NUM_22
#define RGB_RED GPIO_NUM_26
#define RGB_BLUE GPIO_NUM_17
#define RGB_GREEN GPIO_NUM_23
#define WiFi_LED GPIO_NUM_27

static int sensor_00100_FrankEver_Switch_prepare(void* arg);
static int sensor_00100_FrankEver_Switch_init(l_ezlopi_item_t* item);
static int sensor_00100_FrankEver_Switch_set_value(l_ezlopi_item_t* item, void* arg);
static int sensor_00100_FrankEver_Switch_get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void sensor_00100_FrankEver_Switch_interrupt_upcall(void* arg);
static void sensor_00100_FrankEver_Switch_toggle_gpio(l_ezlopi_item_t* item);
static void sensor_00100_FrankEver_Switch_write_gpio_value(l_ezlopi_item_t* item);
static void sensor_00100_FrankEver_Switch_set_gpio_value(l_ezlopi_item_t* item, int value);

TaskHandle_t some_task_Handle = NULL;
TaskHandle_t Frankever_Wifi_Handle = NULL;
static bool wifi_state =0;
static uint8_t count =0;
static bool load_state = 0;
static bool state =false;
uint32_t time_first = 0;
s_AC_power_delay_t power_delay;
s_switch_features_t Switch_feature;

int device_00100_FrankEver_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* arg, void* user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = sensor_00100_FrankEver_Switch_prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = sensor_00100_FrankEver_Switch_init(item);
        break;
    }
    case EZLOPI_ACTION_SET_VALUE:
    {
        ret = sensor_00100_FrankEver_Switch_set_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = sensor_00100_FrankEver_Switch_get_value_cjson(item, arg);
        break;
    }

    default:
    {
        break;
    }
    }

    return ret;
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
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();

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
        int wifi_connection_stat = ezlopi_wifi_got_ip();
        if(wifi_connection_stat)
        {    
            // TRACE_I("---------------WIFI----------%d",wifi_connection_stat);
            gpio_set_level(WiFi_LED,wifi_state);
            wifi_state = !wifi_state;
        }
        else
        {
            // TRACE_I("---------------WIFI-NOT----------%d",wifi_connection_stat);
            gpio_set_level(WiFi_LED,0);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

static int sensor_00100_FrankEver_Switch_prepare(void* arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    if(Frankever_Wifi_Handle == NULL)
        xTaskCreate(wifi_status_task,"wifi_status",1024 * 3,NULL,5,&Frankever_Wifi_Handle);
    
    if (arg)
    {
        cJSON* cjson_device = prep_arg->cjson_device;
        if (cjson_device)
        {
            l_ezlopi_device_t* device = ezlopi_device_add_device(cjson_device, NULL);
            if (device)
            {
                __setup_device_cloud_properties(device, cjson_device);
                l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, device_00100_FrankEver_Switch);
                if (item)
                {
                    __setup_item_properties(item, cjson_device);
                    ret = 1;
                }
                else
                {
                    ezlopi_device_free_device(device);
                    ret = -1;
                }
            }
        }
    }

    return ret;
}

static int sensor_00100_FrankEver_Switch_init(l_ezlopi_item_t* item)
{
    int ret = 0;
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
                sensor_00100_FrankEver_Switch_write_gpio_value(item);
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
        ezlopi_service_gpioisr_register_v3(item, sensor_00100_FrankEver_Switch_interrupt_upcall, 350);
        ret = 1;
    }

    if (0 == ret)
    {
        ret = -1;
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
                ezlopi_service_gpioisr_register_v3(item, sensor_00100_FrankEver_Switch_interrupt_upcall, 1000);
                ret = 1;
            }
            else
            {
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}

static int sensor_00100_FrankEver_Switch_get_value_cjson(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if (item && arg)
    {
        cJSON* cj_propertise = (cJSON*)arg;
        if (cj_propertise)
        {
            ezlopi_valueformatter_bool_to_cjson(cj_propertise, item->interface.gpio.gpio_out.value, item->cloud_properties.scale);
            ret = 1;
        }
    }
    return ret;
}

static void sensor_00100_FrankEver_Switch_set_gpio_value(l_ezlopi_item_t* item, int value)
{
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, value);
    if(value == 1)
        gpio_set_level(BLUE_LED1,0);
    else
        gpio_set_level(BLUE_LED1,1);
    item->interface.gpio.gpio_out.value = value;
}

static int sensor_00100_FrankEver_Switch_set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    power_delay.time_delay = esp_timer_get_time();
    TRACE_I("--------------SET VALUE--------------%lld",power_delay.time_delay);
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
                    sensor_00100_FrankEver_Switch_set_gpio_value(item, value);
                    ezlopi_device_value_updated_from_device_broadcast(item);
                    ret = 1;
                }
            }
            else
            {
                l_ezlopi_device_t* curr_device = ezlopi_device_get_head();
                while (curr_device)
                {
                    l_ezlopi_item_t* curr_item = curr_device->items;
                    while (curr_item)
                    {
                        if ((EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT == curr_item->interface_type) && (255 != curr_item->interface.gpio.gpio_out.gpio_num))
                        {
                            TRACE_D("GPIO-pin: %d", curr_item->interface.gpio.gpio_out.gpio_num);
                            TRACE_D("value: %d", value);
                            sensor_00100_FrankEver_Switch_set_gpio_value(curr_item, value);
                            ezlopi_device_value_updated_from_device_broadcast(curr_item);
                            ret = 1;
                        }
                        curr_item = curr_item->next;
                    }
                    curr_device = curr_device->next;
                }

                item->interface.gpio.gpio_out.value = value;
                ezlopi_device_value_updated_from_device_broadcast(item);
            }
        }
    }
    return ret;
}

static void sensor_00100_FrankEver_Switch_write_gpio_value(l_ezlopi_item_t* item)
{
    uint32_t write_value = (0 == item->interface.gpio.gpio_out.invert) ? item->interface.gpio.gpio_out.value : (item->interface.gpio.gpio_out.value ? 0 : 1);
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, write_value);
    gpio_set_level(BLUE_LED1,1);
}

void some_task(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    bool fresh_state_reset=0;
    switch(count)
    {
        case 0:
            TRACE_I("-------------NO COUNTS!--------------");
            break;
        case 1:
            TRACE_I("-------------COUNT--------------%d",count);
            sensor_00100_FrankEver_Switch_toggle_gpio(item);
            break;
        case 2:
            state = !state;
            TRACE_I("-------------COUNT--------------%d",count);
            break;
        case 3:
            TRACE_I("-------------20 Seconds!--------------");
            fresh_state_reset =1;
            ezlopi_factory_info_v3_get_ssid(fresh_state_reset);
            ezlopi_factory_info_v3_get_password(fresh_state_reset);
            fresh_state_reset =0;
            esp_restart();
            return;
    }
    if(state == 0 && (load_state == 0 || load_state == 1) )
    {
        TRACE_I("-------------Purple HERE--------------");
        gpio_set_level(RGB_RED, 0);
        gpio_set_level(RGB_BLUE, 0);
        gpio_set_level(RGB_GREEN, 0);
    }
    else if(state == 1 && load_state == 0)
    {
        TRACE_I("-------------Green HERE--------------");
        gpio_set_level(RGB_RED, state);
        gpio_set_level(RGB_BLUE, 0);
        gpio_set_level(RGB_GREEN, state);
    }
    else
    {
        TRACE_I("-------------Purple HERE--------------");
        gpio_set_level(RGB_RED, state);
        gpio_set_level(RGB_BLUE, state);
        gpio_set_level(RGB_GREEN, 0);
    }
    count=0;
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
        difference = esp_timer_get_time() - time_first;
    }
    if (difference > (20*1000*1000))
    {
        count = 3;
        some_task(item);
    }
    else if (difference < (3*1000*1000))
    {
        uint32_t delay_time = 3000000 - difference;
        vTaskDelay((delay_time/1000)/portTICK_RATE_MS);
        if (count == 1)
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

static void sensor_00100_FrankEver_Switch_interrupt_upcall(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    power_delay.interrupttime = esp_timer_get_time();
    TRACE_I("-------------ESPARK--------------%lld",power_delay.interrupttime);
    if (item && (power_delay.interrupttime >= (power_delay.time_delay+30000)))//delay of 30ms for AC power issue
    {
        
        if (count && (esp_timer_get_time() - time_first) < (3 * 1000 * 1000))
        {
            count = 2;
            some_task(item);
        }
        else
        {
            count = 1;
            time_first = esp_timer_get_time();
            xTaskCreate(push_button_task,"push_button",2*2048,item,5,NULL);
        }
    }
}

static void sensor_00100_FrankEver_Switch_toggle_gpio(l_ezlopi_item_t* item)
{
    uint32_t write_value = !(item->interface.gpio.gpio_out.value);
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, write_value);
    if(write_value == 1)
    { 
        gpio_set_level(BLUE_LED1,0);
        load_state = 1;
    }
    else
    {
        gpio_set_level(BLUE_LED1,1);
        load_state = 0;
    }
    item->interface.gpio.gpio_out.value = write_value;
}