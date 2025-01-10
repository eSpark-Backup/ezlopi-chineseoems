#include "shtc3.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/ledc.h"
#include "ezlopi_core_ota.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00101_Cubic_Detector.h"
#include "sensor_0099_SHTC3_temp_humidity.h"
#include "ezlopi_core_device_value_updated.h"

#define SWITCH                  GPIO_NUM_34
#define LED_RED                 GPIO_NUM_13
#define LED_BLUE                GPIO_NUM_12

#define RESET                   (10*1000*1000)
#define PAIRING_MODE            (5*1000*1000)
#define OTA_MODE                (15*1000*1000)
#define PAIRING_TIME            (600*1000*1000)
#define WAKEUP_TIME             (50*1000*1000)
#define SENSOR_ADDR             0x70

#define NEO_BUZZER_PIN          GPIO_NUM_18
#define NEO_LEDC_TIMER          LEDC_TIMER_0
#define NEO_LEDC_MODE           LEDC_HIGH_SPEED_MODE
#define NEO_LEDC_OUTPUT_IO      NEO_BUZZER_PIN
#define NEO_LEDC_CHANNEL        LEDC_CHANNEL_0
#define NEO_LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define NEO_LEDC_FREQUENCY      1000   

static uint64_t start_time = 0;
uint64_t sleep_time = (15*1000*1000);
TaskHandle_t  pairing_task_handle = NULL;

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
    .duty           = 0,
    .hpoint         = 0
};

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __get_item_list(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);
static void device_cubic_detector_value_updated_from_device_v3(void* arg);
static void device_setup_item_properties(l_ezlopi_item_t* item, cJSON* cj_device);
static void device_setup_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cj_device);
static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device);

ezlopi_error_t device_00101_Cubic_Detector(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
    case EZLOPI_ACTION_HUB_GET_ITEM:
    {
        error = __get_item_list(item, args);
        break;
    }
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        error = __get_value_cjson(item, args);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    default:
    {
        break;
    }
    }

    return error;
}

static ezlopi_error_t __get_item_list(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    if(ezlopi_item_name_water_leak_alarm == item->cloud_properties.item_name)
    {
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
            error = EZPI_SUCCESS;
        }
    }
    return error;
}
void output_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*) arg;
    while(1)
    {
        if(item)
        {
            item->interface.gpio.gpio_in.value = gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
            if (1 == item->interface.gpio.gpio_in.value)
            {
                ledc_set_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel, 4096);
                ledc_update_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel);
                gpio_set_level(LED_RED, 0);
                vTaskDelay(200 / portTICK_PERIOD_MS);
                ledc_set_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel, 0);
                ledc_update_duty(neo_ledc_channel.speed_mode, neo_ledc_channel.channel);
                gpio_set_level(LED_RED, 1);
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
            else
            {
                vTaskDelete(NULL);
            }
        }
    }
}

void update_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    if(item)
    {
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            while(1)
            {
                if(EZPI_service_webprov_is_connected())
                {
                    __0099_notify(device_head->next->items);
                    __0099_notify(device_head->next->next->items);
                    EZPI_core_device_value_updated_from_device_broadcast(item);
                    EZPI_core_device_value_updated_from_device_broadcast(item->next);
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                    vTaskDelete(NULL);
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }
}

void pairing_led_task(void* arg)
{
    uint8_t count = *(uint8_t *) arg;
    while(1)
    {
        gpio_set_level(LED_BLUE, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(LED_BLUE, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void ota_led_task(void* arg)
{
    while(1)
    {
        switch(EPZI_core_ota_get_state())
        {
            case 0:
            {
                gpio_set_level(LED_RED, 0);
                gpio_set_level(LED_BLUE, 0);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                vTaskDelete(NULL);
            }
            break;
            case 1:
            {
                gpio_set_level(LED_RED, 0);
                gpio_set_level(LED_BLUE, 1);
                vTaskDelay(200 / portTICK_PERIOD_MS);
                gpio_set_level(LED_RED, 1);
                gpio_set_level(LED_BLUE, 0);
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
            break;
            case 2:
            {
                gpio_set_level(LED_RED, 0);
                gpio_set_level(LED_BLUE, 1);
                vTaskDelay(200 / portTICK_PERIOD_MS);
                gpio_set_level(LED_RED, 1);
                gpio_set_level(LED_BLUE, 0);
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
            break;
            case 3:
            {
                gpio_set_level(LED_RED, 0);
                gpio_set_level(LED_BLUE, 0);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
            }
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void long_press_task(void* arg)
{
    while(1)
    {
        uint64_t press_time = esp_timer_get_time();
        uint8_t count =0;
        while(gpio_get_level(SWITCH))
        {
            sleep_time += PAIRING_MODE;
            if((esp_timer_get_time() - press_time) >= (PAIRING_MODE) && (esp_timer_get_time() - press_time) < (RESET) && count != 1)
            {
                count = 1;
                // TRACE_E("I'm IN PAIRINGGGGGGGGGGGGGGGGGG---------------%d",count);
                gpio_set_level(LED_BLUE, 1);
                gpio_set_level(LED_RED, 1);
                xTaskCreate(pairing_led_task, "pairing_led", 2*2048, NULL, 5, &pairing_task_handle);
            }
            else if((esp_timer_get_time() - press_time) >= (RESET) && (esp_timer_get_time() - press_time) < (OTA_MODE) && count != 2)
            {
                count = 2;
                vTaskDelete(pairing_task_handle);
                for(size_t i = 0; i < 10; i++)
                {
                    gpio_set_level(LED_BLUE, 0);
                    gpio_set_level(LED_RED, 0);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    gpio_set_level(LED_BLUE, 1);
                    gpio_set_level(LED_RED, 1);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                }
                // TRACE_E("I'm IN RESETTTTTTTTTTTTTTTTTTTTTT---------------%d",count);
            }
            else if((esp_timer_get_time() - press_time) >= (OTA_MODE) && count != 3)
            {
                count = 3;
                gpio_set_level(LED_BLUE, 1);
                gpio_set_level(LED_RED, 1);
                xTaskCreate(ota_led_task, "ota_led", 2*2048, NULL, 5, NULL);
                // TRACE_E("I'm IN OTAAAAAAAAAAAAAAAAAAAAAAAA---------------%d",count);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        switch(count)
        {
            case 1:
            {
                // TRACE_E("I'm IN PAIRINGGGGGGGGGGGGGGGGGG");
                sleep_time = sleep_time + (PAIRING_TIME);
            }
            break;
            case 2:
            {
                // TRACE_E("I'm IN RESETTTTTTTTTTTTTTTTTTTTTT");
                if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                }
                if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY))
                    {

                    }
                }
                for(size_t i = 0; i < 10; i++)
                {
                    gpio_set_level(LED_BLUE, 0);
                    gpio_set_level(LED_RED, 0);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    gpio_set_level(LED_BLUE, 1);
                    gpio_set_level(LED_RED, 1);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                }
                esp_restart();
            }
            break;
            case 3:
            {
                cJSON *cj_data = cJSON_CreateObject(__FUNCTION__);
                if (cj_data)
                {
                    cJSON_AddStringToObject(__FUNCTION__, cj_data, "method", "hub.firmware.update.start");
                    cJSON_AddStringToObject(__FUNCTION__, cj_data, "id", "_ID_");
                    cJSON *params = cJSON_CreateObject(__FUNCTION__);
                    if (params)
                    {
                        cJSON_AddItemToObject(__FUNCTION__, cj_data, "params", params);
                        if(__send_cjson_data_to_nma_websocket(cj_data) == EZPI_SUCCESS)
                        {
                            // TRACE_E("OTA METHOD SENT");
                        }
                        else{
                            // TRACE_E("OTA METHOD NOT SENT");
                        }
                    }
                    cJSON_Delete(__FUNCTION__, cj_data);
                }
            }
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    ezlopi_error_t error = EZPI_FAILED;
    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return error;
    }

    if(ezlopi_item_name_water_leak_alarm == item->cloud_properties.item_name)
    {
        const char* curret_value = NULL;
        item->interface.gpio.gpio_in.value = gpio_get_level(item->interface.gpio.gpio_in.gpio_num);
        if (1 == item->interface.gpio.gpio_in.value)
        {
            curret_value = "water_leak_detected";
            xTaskCreate(output_task, "Output", 2*2048, item, 5, NULL);
        }
        else
        {
            curret_value = "no_water_leak";
        }
        item->user_arg = (void*)curret_value;
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_value_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
        cJSON_AddStringToObject(__FUNCTION__, cj_params, ezlopi_valueFormatted_str, (char*)item->user_arg ? item->user_arg : "no_water_leak");
        error = EZPI_SUCCESS;
    }
    else if(ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            EZPI_core_valueformatter_bool_to_cjson(cj_params, device_head->items->interface.gpio.gpio_in.value, item->cloud_properties.scale);
        }
        error = EZPI_SUCCESS;
    }
    return error;
}

static void device_cubic_detector_value_updated_from_device_v3(void* arg)
{
    l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
    if(device_head)
    {
        l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
        if (item)
        {
            __0099_notify(device_head->next->items);
            __0099_notify(device_head->next->next->items);
            if(EZPI_core_device_value_updated_from_device_broadcast(item) && EZPI_core_device_value_updated_from_device_broadcast(item->next))
            {
                sleep_time = sleep_time +(PAIRING_MODE);
            }
        }
    }
}

static ezlopi_error_t __init(l_ezlopi_item_t* item)
{
    ezlopi_error_t error = EZPI_ERR_INIT_DEVICE_FAILED;
    if (ezlopi_item_name_water_leak_alarm == item->cloud_properties.item_name)
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
            
            const gpio_config_t io_conf_sw = {
                .pin_bit_mask = (1ULL << SWITCH),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
            };
            
            const gpio_config_t io_conf_blue = {
                .pin_bit_mask = (1ULL << LED_BLUE),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
            };
            
            const gpio_config_t io_conf_red = {
                .pin_bit_mask = (1ULL << LED_RED),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
            };

            ledc_timer_config(&neo_ledc_timer);
            ledc_channel_config(&neo_ledc_channel);
            ledc_fade_func_install(0);

            if (ESP_OK == gpio_config(&io_conf) && ESP_OK == gpio_config(&io_conf_red) && ESP_OK == gpio_config(&io_conf_blue) && ESP_OK == gpio_config(&io_conf_sw))
            {
                TRACE_I("Water Leak Sensor initialized successfully.");
                gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);    
                gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
                gpio_set_direction(SWITCH, GPIO_MODE_INPUT);
                gpio_set_level(LED_RED, 1);
                gpio_set_level(LED_BLUE, 1); 
                xTaskCreate(output_task, "Output", 2*2048, item, 5, NULL);
                xTaskCreate(update_task, "Update", 2*2048, item, 5, NULL);
                xTaskCreate(long_press_task, "long_press", 2*2048, NULL, 5, NULL);
                EZPI_service_gpioisr_register_v3(item, device_cubic_detector_value_updated_from_device_v3, 200);
                error = EZPI_SUCCESS;
            }
            else
            {
                TRACE_E("Error initializing Water Leak Sensor, error: %s", esp_err_to_name(error));
                error = EZPI_FAILED;
            }
        }
        else
        {
            error = EZPI_FAILED;
        } 
    }
    else if(ezlopi_item_name_security_threat == item->cloud_properties.item_name)
    {
        error = EZPI_SUCCESS;
    }
    else
    {
        error = EZPI_FAILED;
    }
    return error;
}

void timer_task(void* arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    if(item)
    {
        while(1)
        {
            if(EPZI_core_ota_get_state() == 1)
            {
                vTaskDelete(NULL);
            }
            else if((esp_timer_get_time() - start_time) > sleep_time) //15sec
            {
                if(gpio_get_level(SWITCH))
                {
                    // TRACE_E("SLEEP TIMEEEEEEEE");
                    sleep_time = sleep_time + PAIRING_MODE;
                }
                else
                {
                    l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
                    if(device_head)
                    {
                        if(item->interface.gpio.gpio_in.value == 1)
                        {
                            esp_sleep_enable_timer_wakeup(WAKEUP_TIME); // 50 seconds
                        }
                        esp_sleep_enable_ext0_wakeup((item->interface.gpio.gpio_in.gpio_num), !item->interface.gpio.gpio_in.value);
                        esp_sleep_enable_ext1_wakeup((1ULL << SWITCH), ESP_EXT1_WAKEUP_ANY_HIGH);
                        gpio_reset_pin(LED_BLUE);
                        gpio_reset_pin(LED_RED);
                        gpio_pullup_dis(NEO_BUZZER_PIN);
                        esp_wifi_stop();
                        esp_bt_controller_disable();
                        shtc3_sleep_sensor(&device_head->next->items->interface.i2c_master);
                        TRACE_I("GOING TO SLEEP");
                        esp_deep_sleep_start();
                    }
                }
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;

    start_time = esp_timer_get_time();
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;

    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        if (cj_device)
        {
            l_ezlopi_device_t* device = EZPI_core_device_add_device(prep_arg->cjson_device, NULL);
            if (device)
            {
                error = EZPI_SUCCESS;
                device_setup_device_cloud_properties(device, cj_device);
                
                l_ezlopi_item_t* item1 = EZPI_core_device_add_item_to_device(device, device_00101_Cubic_Detector);
                if (item1)
                {
                    device_setup_item_properties(item1, cj_device);
                    xTaskCreate(timer_task, "Timer", 2*2048, item1, 5, NULL);
                }
                else
                {
                    EZPI_core_device_free_device(device);
                    error = EZPI_FAILED;
                }
                l_ezlopi_item_t *item2 = EZPI_core_device_add_item_to_device(device, device_00101_Cubic_Detector);
                if (item2)
                {
                    __prepare_security_threat_item_cloud_properties(item2, cj_device);
                }
                else
                {
                    error = EZPI_FAILED;
                    EZPI_core_device_free_device(device);
                }
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
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();

    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);

    item->interface.gpio.gpio_in.enable = true;
    item->interface.gpio.gpio_in.mode = GPIO_MODE_INPUT;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_in_str, item->interface.gpio.gpio_in.gpio_num);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_logic_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_pull_up_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_ANYEDGE;
}

static void __prepare_security_threat_item_cloud_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    item->cloud_properties.show = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_security_threat;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.scale = NULL;
}