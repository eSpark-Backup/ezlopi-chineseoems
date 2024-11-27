#include "esp_sleep.h"
#include "driver/ledc.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_websocket_client.h"
#include "ezlopi_service_gpioisr.h"
#include "device_00101_Cubic_Detector.h"
#include "sensor_0099_SHTC3_temp_humidity.h"

#define SENSOR_ADDR 0x70

#define NEO_BUZZER_PIN 18       // GPIO pin where the buzzer is connected
#define NEO_LEDC_TIMER          LEDC_TIMER_0
#define NEO_LEDC_MODE           LEDC_HIGH_SPEED_MODE
#define NEO_LEDC_OUTPUT_IO      NEO_BUZZER_PIN
#define NEO_LEDC_CHANNEL        LEDC_CHANNEL_0
#define NEO_LEDC_DUTY_RES       LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define NEO_LEDC_FREQUENCY      1000   

static uint64_t start_time = 0;
const char* neo_water_leak_alarm_states[] = {
    "no_water_leak",
    "water_leak_detected",
    "unknown",
};

 ledc_timer_config_t neo_ledc_timer = {
    .speed_mode       = NEO_LEDC_MODE,
    .timer_num        = NEO_LEDC_TIMER,
    .duty_resolution  = NEO_LEDC_DUTY_RES,
    .freq_hz          = NEO_LEDC_FREQUENCY,
    .clk_cfg          = LEDC_AUTO_CLK
};            

ledc_channel_config_t neo_ledc_channel = {
    .speed_mode     = NEO_LEDC_MODE,
    .channel        = NEO_LEDC_CHANNEL,
    .timer_sel      = NEO_LEDC_TIMER,
    .intr_type      = LEDC_INTR_FADE_END,
    .gpio_num       = NEO_LEDC_OUTPUT_IO,
    .duty           = 0, // Set duty to 0%
    .hpoint         = 0
};

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __get_item_list(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);
static void device_cubic_detector_value_updated_from_device_v3(void* arg);
static void device_setup_item_properties(l_ezlopi_item_t* item, cJSON* cj_device);
static void device_setup_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cj_device);

int device_00101_Cubic_Detector(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
    case EZLOPI_ACTION_HUB_GET_ITEM:
    {
        ret = __get_item_list(item, args);
        break;
    }
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __get_value_cjson(item, args);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    default:
    {
        break;
    }
    }

    return ret;
}

static int __get_item_list(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    cJSON* cjson_propertise = (cJSON*)arg;
    if (cjson_propertise)
    {
        cJSON* json_array_enum = cJSON_CreateArray(__FUNCTION__);
        if (NULL != json_array_enum)
        {
            for (uint8_t i = 0; i < NEO_WATERLEAK_MAX; i++)
            {
                cJSON* json_value = cJSON_CreateString(__FUNCTION__, neo_water_leak_alarm_states[i]);
                if (NULL != json_value)
                {
                    cJSON_AddItemToArray(json_array_enum, json_value);
                }
            }
            cJSON_AddItemToObject(__FUNCTION__, cjson_propertise, ezlopi_enum_str, json_array_enum);
        }

        cJSON_AddStringToObject(__FUNCTION__, cjson_propertise, ezlopi_value_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
        cJSON_AddStringToObject(__FUNCTION__, cjson_propertise, ezlopi_valueFormatted_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
        ret = 1;
    }
    return ret;
}

void update_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    if(item)
    {
        l_ezlopi_device_t* device_head = ezlopi_device_get_head();
        if(device_head)
        {
            while(1)
            {
                if(ezlopi_websocket_client_is_connected())
                {
                    __0099_notify(device_head->next->items);
                    __0099_notify(device_head->next->next->items);
                    ezlopi_device_value_updated_from_device_broadcast(item);
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    
                    gpio_pad_select_gpio(item->interface.gpio.gpio_in.gpio_num);
                    gpio_set_direction(item->interface.gpio.gpio_in.gpio_num, GPIO_MODE_INPUT);
                    gpio_pullup_en(item->interface.gpio.gpio_in.gpio_num);
                    gpio_pulldown_dis(item->interface.gpio.gpio_in.gpio_num);

                    esp_sleep_enable_ext0_wakeup(item->interface.gpio.gpio_in.gpio_num, !item->interface.gpio.gpio_in.value);
                    // TRACE_I("------------UPDATED------------");
                    // TRACE_I("GOING TO SLEEP");
                    esp_deep_sleep_start();
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }
}

static int __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    int ret = 0;
    cJSON* cj_result = (cJSON*)args;
    // TRACE_I("------------CUBIC DETECTOR GET VALUE------------",item->interface.gpio.gpio_in.value);
    if (cj_result)
    {   
        const char* curret_value = NULL;
        item->interface.gpio.gpio_in.value = gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
        // TRACE_I("------------CUBIC DETECTOR------------%d",item->interface.gpio.gpio_in.value);
        if (1 == item->interface.gpio.gpio_in.value)
        {
            // TRACE_I("------------LEAK------------");
            curret_value = "water_leak_detected";
        }
        else
        {
            // TRACE_I("------------NO LEAK------------");
            curret_value = "no_water_leak";
        }
        if (curret_value != (char*)item->user_arg)
        {
            item->user_arg = (void*)curret_value;
            cJSON_AddStringToObject(__FUNCTION__, cj_result, ezlopi_value_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
            cJSON_AddStringToObject(__FUNCTION__, cj_result, ezlopi_valueFormatted_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
        }
        ret = 1;
    }
    return ret;
}

static void device_cubic_detector_value_updated_from_device_v3(void* arg)
{
    l_ezlopi_device_t* device_head = ezlopi_device_get_head();
    if(device_head)
    {
        l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
        if (item)
        {
            __0099_notify(device_head->next->items);
            __0099_notify(device_head->next->next->items);
            if(ezlopi_device_value_updated_from_device_broadcast(item))
            {
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                esp_sleep_enable_ext0_wakeup(item->interface.gpio.gpio_in.gpio_num, !item->interface.gpio.gpio_in.value);
                // TRACE_I("GOING TO SLEEP");
                esp_deep_sleep_start();
            }
        }
    }
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;
    if (item)
    {
        if (GPIO_IS_VALID_GPIO(item->interface.gpio.gpio_in.gpio_num))
        {
            const gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << item->interface.gpio.gpio_in.gpio_num),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = item->interface.gpio.gpio_in.interrupt,
            };

            ledc_timer_config(&neo_ledc_timer);
            ledc_channel_config(&neo_ledc_channel);
            ledc_fade_func_install(0);

            if (ESP_OK == gpio_config(&io_conf))
            {
                TRACE_I("Water Leak Sensor initialized successfully.");
                item->interface.gpio.gpio_in.value = gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
                if (1 == item->interface.gpio.gpio_in.value)
                {
                    // TRACE_I("------------LEAK------------");
                    ledc_set_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel, 4096);
                    ledc_update_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel);
                }
                else
                {
                    // TRACE_I("------------NO LEAK------------");
                    ledc_set_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel, 0);
                    ledc_update_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel);
                }
                xTaskCreate(update_task, "Update", 2*2048, item, 5, NULL);
                ezlopi_service_gpioisr_register_v3(item, device_cubic_detector_value_updated_from_device_v3, 200);
                ret = 1;
            }
            else
            {
                TRACE_E("Error initializing Water Leak Sensor, error: %s", esp_err_to_name(ret));
                ret = -1;
            }
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
    return ret;
}

void timer_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    if(item)
    {
        while(1)
        {
            if((esp_timer_get_time() - start_time) > (15*1000*1000)) //15sec
            {
                gpio_pad_select_gpio(item->interface.gpio.gpio_in.gpio_num);
                gpio_set_direction(item->interface.gpio.gpio_in.gpio_num, GPIO_MODE_INPUT);
                gpio_pullup_en(item->interface.gpio.gpio_in.gpio_num);
                gpio_pulldown_dis(item->interface.gpio.gpio_in.gpio_num);

                esp_sleep_enable_ext0_wakeup(item->interface.gpio.gpio_in.gpio_num, !item->interface.gpio.gpio_in.value);
                TRACE_I("GOING TO SLEEP");
                esp_deep_sleep_start();
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

static int __prepare(void* arg)
{
    int ret = 0;
    start_time = esp_timer_get_time();
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;

    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        if (cj_device)
        {
            l_ezlopi_device_t* device = ezlopi_device_add_device(prep_arg->cjson_device, NULL);
            if (device)
            {
                ret = 1;
                device_setup_device_cloud_properties(device, cj_device);
                
                l_ezlopi_item_t* item = ezlopi_device_add_item_to_device(device, device_00101_Cubic_Detector);
                if (item)
                {
                    device_setup_item_properties(item, cj_device);
                    xTaskCreate(timer_task, "Timer", 2*2048, item, 5, NULL);
                }
                else
                {
                    ezlopi_device_free_device(device);
                    ret = -1;
                }
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

static void device_setup_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cj_device)
{
    if (device && cj_device)
    {
        device->cloud_properties.category = category_security_sensor;
        device->cloud_properties.subcategory = subcategory_leak;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.info = NULL;
        device->cloud_properties.device_type_id = NULL;
    }
}

static void device_setup_item_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    int tmp_var = 0;
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_name = ezlopi_item_name_water_leak_alarm;
    item->cloud_properties.value_type = value_type_token;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();

    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);

    item->interface.gpio.gpio_in.enable = true;
    item->interface.gpio.gpio_in.mode = GPIO_MODE_INPUT;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_in_str, item->interface.gpio.gpio_in.gpio_num);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_logic_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_pull_up_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_ANYEDGE;
}