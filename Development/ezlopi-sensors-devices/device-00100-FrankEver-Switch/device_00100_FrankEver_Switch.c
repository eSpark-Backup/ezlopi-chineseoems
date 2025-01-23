#include "ezlopi_core_wifi.h"
#include "ezlopi_core_ota.h"
#include "ezlopi_core_reset.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00100_FrankEver_Switch.h"
#include "ezlopi_core_device_value_updated.h"

#define BLUE_LED1 GPIO_NUM_22
#define RGB_RED   GPIO_NUM_26
#define RGB_BLUE  GPIO_NUM_17
#define RGB_GREEN GPIO_NUM_23
#define WiFi_LED  GPIO_NUM_27

#define OTA       (10*1000*1000)
#define RESET     (20*1000*1000)
#define PULSE      (3*1000*1000)

#define OFF 0
#define  ON 1

volatile bool flag = 0;
bool ota_flag = 0;
typedef struct s_AC_power_delay
{
    bool state;
    uint8_t count;
    bool load_state;
    uint64_t time_delay;
    uint64_t time_first;
} s_AC_power_delay_t;

s_AC_power_delay_t power_delay;

static ezlopi_error_t __prepare(void* arg);
static ezlopi_error_t __init(l_ezlopi_item_t* item);
static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg);
static ezlopi_error_t __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void __interrupt_call(void* arg);
static void __toggle_gpio(l_ezlopi_item_t* item);
static void __write_gpio_value(l_ezlopi_item_t* item);
static void __set_gpio_value(l_ezlopi_item_t* item);

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
    item->cloud_properties.show = true;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = EZPI_core_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;


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
            gpio_set_level(WiFi_LED, OFF);
        }
        else
        {
            // TRACE_I("---------------WIFI-NOT----------%d",wifi_connection_stat);
            gpio_set_level(WiFi_LED, ON);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

static ezlopi_error_t __prepare(void* arg)
{
    ezlopi_error_t error = EZPI_ERR_PREP_DEVICE_PREP_FAILED;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    memset(&power_delay, 0, sizeof(s_AC_power_delay_t));
    xTaskCreatePinnedToCore(wifi_status_task, "wifi_status", 1024, NULL, 5, NULL, tskNO_AFFINITY);
    
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

        gpio_config(&io_conf);
        EZPI_service_gpioisr_register_v3(item, __interrupt_call, 350);
        error = EZPI_SUCCESS;
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

static void __set_gpio_value(l_ezlopi_item_t* item)
{
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, item->interface.gpio.gpio_out.value);
    gpio_set_level(BLUE_LED1, !item->interface.gpio.gpio_out.value);
}

static ezlopi_error_t __set_value(l_ezlopi_item_t* item, void* arg)
{
    ezlopi_error_t error = EZPI_FAILED;
    power_delay.time_delay = esp_timer_get_time();
    // TRACE_I("--------------SET VALUE--------------%lld",power_delay.time_delay);
    if (item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (cjson_params)
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
                    value = cj_value->valueint;
                    break;
                }
                default: {
                    break;
                }
                }
            }
            item->interface.gpio.gpio_out.value = value;
            __set_gpio_value(item);
            EZPI_core_device_value_updated_from_device_broadcast(item);
            error = EZPI_SUCCESS;
        }
        else
        {
            error = EZPI_FAILED;
        }
    }
    return error;
}

static void __write_gpio_value(l_ezlopi_item_t* item)
{
    uint32_t write_value = (0 == item->interface.gpio.gpio_out.invert) ? item->interface.gpio.gpio_out.value : (item->interface.gpio.gpio_out.value ? 0 : 1);
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, write_value);
    gpio_set_level(BLUE_LED1, ON);
}

void ota_led_task(void* arg)
{
    while(1)
    {
        ota_flag = 1;
        switch(EPZI_core_ota_get_state())
        {
            case 0: //White on finished
            {
                gpio_set_level(RGB_RED, 1);
                gpio_set_level(RGB_BLUE, 1);
                gpio_set_level(RGB_GREEN, 1);
            }
            break;
            case 1: //Cyan blink
            {
                gpio_set_level(RGB_RED,   OFF);
                for (size_t i = 0; i < 10; i++)
                {
                    gpio_set_level(RGB_BLUE,  ON);
                    gpio_set_level(RGB_GREEN, ON);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(RGB_BLUE,  OFF);
                    gpio_set_level(RGB_GREEN, OFF);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            }
            break;
            case 2: //Cyan blink
            {
                gpio_set_level(RGB_RED,   OFF);
                for (size_t i = 0; i < 10; i++)
                {
                    gpio_set_level(RGB_BLUE,  ON);
                    gpio_set_level(RGB_GREEN, ON);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(RGB_BLUE,  OFF);
                    gpio_set_level(RGB_GREEN, OFF);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            }
            break;
            case 3: //Red Blink
            {
                gpio_set_level(RGB_BLUE,  OFF);
                gpio_set_level(RGB_GREEN, OFF);
                for (size_t i = 0; i < 10; i++)
                {
                    gpio_set_level(RGB_RED, ON);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(RGB_RED, OFF);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                EZPI_core_reset_reboot();
            }
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void some_task(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    switch(power_delay.count)
    {
        case 1:
            power_delay.count = 0;
            __toggle_gpio(item);
            if(power_delay.state)
            {
                if(power_delay.load_state)
                {
                    // TRACE_I("-------------Red HERE--------------");
                    gpio_set_level(RGB_RED, 1);
                    gpio_set_level(RGB_BLUE, 0);
                    gpio_set_level(RGB_GREEN, 0);
                }
                else
                {
                    // TRACE_I("-------------Yellow HERE--------------");
                    gpio_set_level(RGB_RED, 1);
                    gpio_set_level(RGB_BLUE, 0);
                    gpio_set_level(RGB_GREEN, 1);
                }
            }
        break;
        case 2:
            power_delay.count = 0;
            power_delay.state = !power_delay.state;
            if(power_delay.state)
            {
                if(power_delay.load_state)
                {
                    // TRACE_I("-------------Red HERE--------------");
                    gpio_set_level(RGB_RED, 1);
                    gpio_set_level(RGB_BLUE, 0);
                    gpio_set_level(RGB_GREEN, 0);
                }
                else
                {
                    // TRACE_I("-------------Yellow HERE--------------");
                    gpio_set_level(RGB_RED, 1);
                    gpio_set_level(RGB_BLUE, 0);
                    gpio_set_level(RGB_GREEN, 1);
                }
            }
            else
            {
                gpio_set_level(RGB_RED, 0);
                gpio_set_level(RGB_BLUE, 0);
                gpio_set_level(RGB_GREEN, 0);
            }
        break;
        case 3:
            // TRACE_I("-------------20 Seconds!--------------");
            if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
            {
                // TRACE_I("-------------DEVICE INFO ERASED FROM NVS--------------");
            }
            if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
            {
                if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY))
                {
                    // TRACE_I("-------------CLOUD INFO ERASED FROM NVS--------------");
                }
            }
            gpio_set_level(RGB_GREEN, OFF);
            for (size_t i = 0; i < 10; i++)
            {
                gpio_set_level(RGB_RED, ON);
                gpio_set_level(RGB_BLUE,ON);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                gpio_set_level(RGB_RED, OFF);
                gpio_set_level(RGB_BLUE,OFF);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            esp_restart();
            return;
        case 4:
            // TRACE_I("-------------10 Seconds!--------------");
            power_delay.count = 0;
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
        break;
        default:
            // TRACE_I("-------------NO COUNTS!--------------");
        break;
    }
}

void push_button_task(void* arg)
{
    flag = 1;
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    uint64_t difference = 0;
    while (!gpio_get_level(item->interface.gpio.gpio_in.gpio_num))
    {
        difference = esp_timer_get_time() - power_delay.time_first;
        if(difference >= (OTA) && (difference < (RESET)))
        {
            if(ota_flag)
                xTaskCreate(ota_led_task, "ota_led", 1024, NULL, 5, NULL);
        }
        vTaskDelay(10/ portTICK_RATE_MS);
    }
    if (difference >= (RESET))
    {
        power_delay.count = 3;
        some_task(item);
    }
    else if(difference >= (OTA) && (difference < (RESET)))
    {
        power_delay.count = 4;
        some_task(item);
    }
    else if (difference < PULSE)
    {
        uint32_t delay_time = PULSE - difference;
        vTaskDelay((1000)/portTICK_RATE_MS);
        if (power_delay.count == 1)
        {
            some_task(item);
        } 
    }
    flag = 0;
    vTaskDelete(NULL);
}

static void __interrupt_call(void *arg)
{
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    if (item && (esp_timer_get_time() >= (power_delay.time_delay+30000)))//delay of 30ms for AC power issue
    {
        if ((power_delay.count) && (esp_timer_get_time() - power_delay.time_first < PULSE))
        {
            power_delay.count = 2;
            some_task(item);
        }
        else
        {
            if(!flag)
            {
                power_delay.count = 1;
                power_delay.time_first = esp_timer_get_time();
                EZPI_core_device_value_updated_from_device_broadcast(item);
                xTaskCreate(push_button_task, "push_button", (3*1024), item, 5, NULL);
            }
        }
    }
}

static void __toggle_gpio(l_ezlopi_item_t* item)
{
    item->interface.gpio.gpio_out.value = !item->interface.gpio.gpio_out.value;
    gpio_set_level(item->interface.gpio.gpio_out.gpio_num, item->interface.gpio.gpio_out.value);
    gpio_set_level(BLUE_LED1,!item->interface.gpio.gpio_out.value);
    power_delay.load_state = item->interface.gpio.gpio_out.value;
}