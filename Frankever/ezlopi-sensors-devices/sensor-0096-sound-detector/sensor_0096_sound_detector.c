
#include <stdlib.h>
#include <string.h>

#include "ezlopi_util_trace.h"

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices_list.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_device_value_updated.h"

#include "ezlopi_cloud_constants.h"
#include "device_airbnb.h"
#include "sensor_0096_sound_detector.h"
#include "sensor_0096_sound_detector_settings.h"
#include "EZLOPI_USER_CONFIG.h"

static int __prepare(void *arg, void *user_arg);
static int __init(l_ezlopi_item_t *item);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __notify(l_ezlopi_item_t *item);

static int __prepare_sound_detector_sensor(cJSON *cj_device, void *user_arg);

static void __prepare_sound_detector_device_cloud_properties(l_ezlopi_device_t *device);
static void __prepare_sound_detector_item_cloud_properties(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg);

/*
{\
    \"dev_type\": 6,\
    \"dev_name\": \"Sound Sensor\",\
    \"id_room\": 35,\
    \"id_item\": 69,\
    \"gpio_tx\": 38,\
    \"gpio_rx\": 9,\
    \"baud\": 115200\
}\
*/

int sensor_0096_sound_detector(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
{
    int ret = 0;
    // TRACE_D("%s", ezlopi_actions_to_string(action));
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __prepare(arg, user_arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __init(item);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __get_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    {
        ret = __notify(item);
        break;
    }

    default:
    {
        break;
    }
    }
    return ret;
}

float parse_sound_packet(uint8_t *buffer, uint32_t output_len)
{
    if (output_len >= PACKET_SIZE)
    {
        if (buffer[0] == HEADER && buffer[1] == PAYLOAD_LENGTH)
        {
            uint8_t checksum = buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];

            if (checksum == buffer[6])
            {
                union
                {
                    float db;
                    uint8_t bytes[4];
                } dbValue;

                dbValue.bytes[0] = buffer[2];
                dbValue.bytes[1] = buffer[3];
                dbValue.bytes[2] = buffer[4];
                dbValue.bytes[3] = buffer[5];

                return dbValue.db;
            }
        }
    }

    return 0.0;
}
static void __data_upcall(uint8_t *buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    stm32_sound_intensity_t *data = (stm32_sound_intensity_t *)uart_object_handle->arg;

#if DEBUG_STM32_SOUND
    for (size_t i = 0; i < output_len; i++)
    {
        printf("0x%X ", buffer[i]);
    }
    printf("\n");
#endif
    data->dbSPL = parse_sound_packet(buffer, output_len);
    // TRACE_E("Sound : %0.2fdb SPL", data->dbSPL);
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *cj_params = (cJSON *)arg;
    stm32_sound_intensity_t *data = (stm32_sound_intensity_t *)item->user_arg;
    if (!data || !cj_params)
    {
        return ret;
    }

    ezlopi_valueformatter_float_to_cjson(cj_params, data->dbSPL, item->cloud_properties.scale);

    return ret;
}

static int __notify(l_ezlopi_item_t *item)
{
    int ret = 0;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_settings = sound_detector_get_broadcast_iterval_setting();
    static uint32_t sec_count = 0;
    if (++sec_count >= broadcast_settings->broadcast_interval)
    {
        sec_count = 0;
        ezlopi_device_value_updated_from_device_broadcast(item);
    }
    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    ezlopi_uart_init(item->interface.uart.baudrate, item->interface.uart.tx, item->interface.uart.rx, __data_upcall, item->user_arg);
    return ret;
}

static void __prepare_sound_detector_device_cloud_properties(l_ezlopi_device_t *device)
{
    device->cloud_properties.category = category_level_sensor;
    device->cloud_properties.subcategory = subcategory_sound;
    device->cloud_properties.device_type = dev_type_sensor;
    device->cloud_properties.device_type_id = NULL;
    device->cloud_properties.info = NULL;
    device->cloud_properties.parent_device_id = ezlopi_airbnb_get_parent_device_id(device->cloud_properties.device_id);
}

static void __prepare_sound_detector_item_cloud_properties(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg)
{
    item->interface_type = EZLOPI_DEVICE_INTERFACE_UART;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_loudness;
    item->cloud_properties.value_type = value_type_loudness;
    item->cloud_properties.scale = scales_decibel;

    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_tx_str, item->interface.uart.tx);
    CJSON_GET_VALUE_GPIO(cj_device, ezlopi_gpio_rx_str, item->interface.uart.rx);
    CJSON_GET_VALUE_DOUBLE(cj_device, ezlopi_baud_str, item->interface.uart.baudrate);
    item->interface.uart.enable = true;
    item->user_arg = user_arg;
}

static int __prepare(void *arg, void *user_arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        stm32_sound_intensity_t *data = (stm32_sound_intensity_t *)ezlopi_malloc(__FUNCTION__, sizeof(stm32_sound_intensity_t));
        if (data)
        {
            memset(data, 0, sizeof(stm32_sound_intensity_t));
            ret = __prepare_sound_detector_sensor(prep_arg->cjson_device, data);
        }
    }

    return ret;
}

static int __prepare_sound_detector_sensor(cJSON *cj_device, void *user_arg)
{
    int ret = 0;
    l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device, NULL);

    if (device)
    {
        ret = 1;
        __prepare_sound_detector_device_cloud_properties(device);

        l_ezlopi_item_t *item = ezlopi_device_add_item_to_device(device, sensor_0096_sound_detector);
        if (item)
        {
            __prepare_sound_detector_item_cloud_properties(item, cj_device, user_arg);
            sensor_0096_sound_detector_settings_initialize(device, user_arg);
        }
        else
        {
            ret = -1;
            ezlopi_device_free_device(device);
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}
