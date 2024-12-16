#include <stdlib.h>
#include <string.h>
#include "dimmer.h"
#include "freertos/queue.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_cloud.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_service_gpioisr.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_valueformatter.h"
#include "device_00104_Frankever_dimmer.h"
#include "ezlopi_core_device_value_updated.h"

//OUTPUTS
#define CONTROL GPIO_NUM_17
#define GREEN GPIO_NUM_21
#define RED GPIO_NUM_23

//INPUTS
#define INCREASE GPIO_NUM_26
#define DECREASE GPIO_NUM_16

#define FACTORY_RESET 20*1000*1000

typedef struct dimmer_data{
    bool switch_value;
    int8_t dimmer_value;
}dimmer_data_t;
bool toggle_status =false;
dimmer_data_t Dimmer_data;

typedef enum {
    EVENT_INCREMENT,
    EVENT_DECREMENT
} event_type_t;

QueueHandle_t event_queue;

typedef struct {
    event_type_t event_type;
} event_t;

static int __prepare(void* arg);
static int __init(l_ezlopi_item_t* item);
static int __set_value(l_ezlopi_item_t* item, void* arg);
static int __get_value_cjson(l_ezlopi_item_t* item, void* arg);

static void __set_led_value(bool value);
static void wifi_status_task(void *args);
static void __interrupt_call(void *arg);
static void task_handler(void* arg);
static void process_dimmer_event(event_type_t event_type);

int device_00104_Frankever_dimmer(e_ezlopi_actions_t action, l_ezlopi_item_t* item, void* args, void* user_arg)
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
        case EZLOPI_ACTION_SET_VALUE:
        {
            ret = __set_value(item, args);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            ret = __get_value_cjson(item, args);
            break;
        }

        default:
        {
            break;
        }
    }

    return ret;
}

void wifi_status_task(void *args)
{
    while (1)
    {
        int wifi_connection_stat = ezlopi_wifi_got_ip();
        if(wifi_connection_stat)
        {    
            gpio_set_level(CONTROL,!wifi_connection_stat); 
            gpio_set_level(GREEN,wifi_connection_stat);
        }
        else
        {
            gpio_set_level(CONTROL,!wifi_connection_stat); 
            gpio_set_level(GREEN,wifi_connection_stat);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void __set_led_value(bool value)
{
    gpio_set_level(RED,!value);
}

static int __set_value(l_ezlopi_item_t* item, void* arg)
{
    int ret = 0;
    if(item && arg)
    {
        cJSON* cjson_params = (cJSON*)arg;
        if (NULL != cjson_params)
        {
            l_ezlopi_device_t* device = ezlopi_device_get_head();
            l_ezlopi_item_t* dimmer_item = device->items;
            l_ezlopi_item_t* switch_item = dimmer_item->next;
            cJSON* cj_value = cJSON_GetObjectItem(__FUNCTION__, cjson_params, ezlopi_value_str);
            ret = 1;
            if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
            {
                send_command_dimmer(cj_value->valueint);
                Dimmer_data.dimmer_value = cj_value->valueint;
                Dimmer_data.switch_value = Dimmer_data.dimmer_value;
                __set_led_value(Dimmer_data.switch_value);
            }
            else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
            {
                send_command_switch(cj_value->valueint);
                Dimmer_data.switch_value = cj_value->valueint;
                Dimmer_data.dimmer_value = Dimmer_data.switch_value ? fetch_packet() : 0x00;
                __set_led_value(Dimmer_data.switch_value);
            }
            ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
            ezlopi_device_value_updated_from_device_broadcast(switch_item);
        }
    }
    return ret;
}

static int __get_value_cjson(l_ezlopi_item_t* item, void* args)
{
    int ret = 0;

    cJSON* cj_params = (cJSON*)args;
    if(!cj_params)
    {
        return ret;
    }

    ret = 1;
    if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_uint32_to_cjson(cj_params, Dimmer_data.dimmer_value, item->cloud_properties.scale);
    }
    else if (ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
        ezlopi_valueformatter_bool_to_cjson(cj_params, Dimmer_data.switch_value, item->cloud_properties.scale);
    }
    
    return ret;
}

static void __interrupt_call(void *arg)
{
    uint64_t difference = 0;
    uint64_t interrupttime = esp_timer_get_time();
    l_ezlopi_item_t* item = (l_ezlopi_item_t*)arg;
    l_ezlopi_device_t* device = ezlopi_device_get_head();
    l_ezlopi_item_t* dimmer_item = device->items;
    l_ezlopi_item_t* switch_item = dimmer_item->next;
    while(!gpio_get_level(item->interface.gpio.gpio_in.gpio_num) && difference < (FACTORY_RESET)) 
    {
        difference = esp_timer_get_time() - interrupttime;
        vTaskDelay(10/ portTICK_RATE_MS);
    }
    if(difference >= (FACTORY_RESET))
    {
        if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
        {
        }
        if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
        {
            if(ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY));
        }
        esp_restart();
    }
    else
    {
        Dimmer_data.switch_value = !Dimmer_data.switch_value;
        Dimmer_data.dimmer_value = Dimmer_data.switch_value ? fetch_packet() : 0x00;
        TRACE_I("SWITCH VALUEEEEEEEEEEEE: %d",Dimmer_data.switch_value);
        send_command_switch(Dimmer_data.switch_value);
        __set_led_value(!toggle_status);
        toggle_status = !toggle_status;
    }
    ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
    ezlopi_device_value_updated_from_device_broadcast(switch_item);
}

static void process_dimmer_event(event_type_t event_type)
{
    l_ezlopi_device_t* device = ezlopi_device_get_head();
    l_ezlopi_item_t* dimmer_item = device->items;
    l_ezlopi_item_t* switch_item = dimmer_item->next;

    if (event_type == EVENT_INCREMENT) {
        Dimmer_data.dimmer_value += 5;
        if (Dimmer_data.dimmer_value > 100)
            Dimmer_data.dimmer_value = 100;
    } else if (event_type == EVENT_DECREMENT) {
        Dimmer_data.dimmer_value -= 5;
        if (Dimmer_data.dimmer_value < 0)
            Dimmer_data.dimmer_value = 0;
    }

    Dimmer_data.switch_value = Dimmer_data.dimmer_value;
    send_command_dimmer(Dimmer_data.dimmer_value);
    __set_led_value(Dimmer_data.dimmer_value);
    ezlopi_device_value_updated_from_device_broadcast(dimmer_item);
    ezlopi_device_value_updated_from_device_broadcast(switch_item);
}

static void task_handler(void* arg)
{
    event_t event;
    while (1)
    {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            process_dimmer_event(event.event_type);
        }
    }
}

void IRAM_ATTR gpio_isr_inc_handler(void* arg)
{
    event_t event = { .event_type = EVENT_INCREMENT };
    xQueueSendFromISR(event_queue, &event, NULL);

}

void IRAM_ATTR gpio_isr_dec_handler(void* arg)
{
    event_t event = { .event_type = EVENT_DECREMENT };
    xQueueSendFromISR(event_queue, &event, NULL);
}

static int __init(l_ezlopi_item_t* item)
{
    int ret = 0;

    if (ezlopi_item_name_dimmer == item->cloud_properties.item_name)
    {
        ret = 1;
        gpio_set_direction(CONTROL,GPIO_MODE_OUTPUT);
        gpio_set_direction(GREEN,GPIO_MODE_OUTPUT);
        gpio_set_direction(RED,GPIO_MODE_OUTPUT);

        if (item->interface.uart.enable)
        {
            if (false == dimmer_init(&item->interface.uart))
            {
                ret = -1;
            }
            else
            {
                send_command_switch(false);
                update_packet(BRIGHTNESS);
                __set_led_value(false);
            }
        }
    }
    else if(ezlopi_item_name_switch == item->cloud_properties.item_name)
    {
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
                                ? GPIO_INTR_NEGEDGE
                                : GPIO_INTR_POSEDGE,
            };

            TRACE_D("enabling interrup for pin: %d", item->interface.gpio.gpio_in.gpio_num);

            gpio_config(&io_conf);
            ezlopi_service_gpioisr_register_v3(item, __interrupt_call, 350);
            ret = 1;
        }
        else
        {
            ret = -1;
        }
        event_queue = xQueueCreate(20, sizeof(event_t));
        if (event_queue == NULL) {
            return;
        }
        gpio_config_t io_conf_inc = {
            .intr_type = GPIO_INTR_NEGEDGE,   
            .mode = GPIO_MODE_INPUT,          
            .pin_bit_mask = (1ULL << INCREASE),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_ENABLE
            };
        gpio_config(&io_conf_inc);
        
        gpio_config_t io_conf_dec = {
            .intr_type = GPIO_INTR_NEGEDGE,    
            .mode = GPIO_MODE_INPUT,           
            .pin_bit_mask = (1ULL << DECREASE),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_ENABLE
        };
        gpio_config(&io_conf_dec);

        gpio_isr_handler_add(INCREASE, gpio_isr_inc_handler, (void*) INCREASE);
        gpio_isr_handler_add(DECREASE, gpio_isr_dec_handler, (void*) DECREASE);
        xTaskCreatePinnedToCore(task_handler, "dimmer_task", 2048, NULL, 10, NULL, 1);
    }
    return ret;
}

static void __prepare_device_cloud_properties(l_ezlopi_device_t* device, cJSON* cj_device)
{
    device->cloud_properties.category = category_dimmable_light;
    device->cloud_properties.subcategory = subcategory_dimmable_bulb;
    device->cloud_properties.device_type = dev_type_dimmer_outlet;
    device->cloud_properties.info = NULL;
    device->cloud_properties.device_type_id = NULL;
}

static void __prepare_dimmer_item_properties(l_ezlopi_item_t* item, cJSON* cj_device)
{
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_dev_type_str, item->interface_type);
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_name = ezlopi_item_name_dimmer;
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();

    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->interface.uart.enable = true;
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx); //GPIO_NUM_18
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx); //GPIO_NUM_5
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate); //9600
}

static void __prepare_dimmer_switch_item_properties(l_ezlopi_item_t* item, cJSON* cjson_device)
{
    int tmp_var = 0;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.show = true;
    item->cloud_properties.item_name = ezlopi_item_name_switch;
    item->cloud_properties.value_type = value_type_bool;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;

    CJSON_GET_VALUE_GPIO(cjson_device, ezlopi_gpio1_str, item->interface.gpio.gpio_in.gpio_num);
    CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_is_ip_str, item->interface.gpio.gpio_in.enable);
    CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_ip_inv_str, item->interface.gpio.gpio_in.invert);
    CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_val_ip_str, item->interface.gpio.gpio_in.value);
    CJSON_GET_VALUE_BOOL(cjson_device, ezlopi_pullup_ip_str, tmp_var);
    item->interface.gpio.gpio_in.pull = tmp_var ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    item->interface.gpio.gpio_in.interrupt = GPIO_INTR_DISABLE;
}

static int __prepare(void* arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t* prep_arg = (s_ezlopi_prep_arg_t*)arg;
    xTaskCreate(wifi_status_task,"wifi_status",1024 * 3,NULL,5,NULL);
    if (prep_arg)
    {
        cJSON* cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t* device = ezlopi_device_add_device(cj_device, NULL);
        if (device)
        {
            ret = 1;
            __prepare_device_cloud_properties(device, cj_device);

            l_ezlopi_item_t* dimmer_item = ezlopi_device_add_item_to_device(device, device_00104_Frankever_dimmer);
            if (dimmer_item)
            {
                __prepare_dimmer_item_properties(dimmer_item, cj_device);
            }

            l_ezlopi_item_t* toggle_item = ezlopi_device_add_item_to_device(device, device_00104_Frankever_dimmer);
            if (toggle_item)
            {
                __prepare_dimmer_switch_item_properties(toggle_item, cj_device);
            }

            if ((NULL == dimmer_item) || (NULL == toggle_item))
            {
                ezlopi_device_free_device(device);
                ret = -1;
            }
        }
    }

    return ret;
}