
#include <stdlib.h>

#include "esp_err.h"

#include "ezlopi_core_cloud.h"
#include "ezlopi_core_nvs.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_cloud_settings.h"

#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_device_value_updated.h"

#include "sensor_0096_sound_detector_settings.h"
#include "EZLOPI_USER_CONFIG.h"

static const char *nvs_key_sound_detector_sensor_broadcast_interval_setting = "enstemp";
static bool setting_changed = false;

l_ezlopi_device_settings_v3_t *device_setting;

static int __settings_callback(e_ezlopi_settings_action_t action, struct l_ezlopi_device_settings_v3 *setting, void *arg, void *user_arg);
static int __settings_get(void *arg, l_ezlopi_device_settings_v3_t *setting);
static int __settings_set(void *arg, l_ezlopi_device_settings_v3_t *setting);
static int __settings_reset(void *arg, l_ezlopi_device_settings_v3_t *setting);
static int __settings_update(void *arg, l_ezlopi_device_settings_v3_t *setting);

// ********************************************* Setting initialization related start ********************************************* //

static int __settings_sensor_0096_sound_detector_setting_initialize_broadcast_interval_setting(l_ezlopi_device_t *device, void *user_arg)
{
    int ret = 0;

    if (device)
    {
        device_setting = ezlopi_device_add_settings_to_device_v3(device, __settings_callback);
        if (device_setting)
        {
            s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)ezlopi_malloc(__FUNCTION__, sizeof(s_sensor_sound_detector_setting_broadcast_interval_t));
            if (broadcast_interval)
            {
                memset(broadcast_interval, 0, sizeof(s_sensor_sound_detector_setting_broadcast_interval_t));
                float read_value = 0;
                uint8_t error = ezlopi_nvs_read_uint32(&read_value, nvs_key_sound_detector_sensor_broadcast_interval_setting);
                if (1 == error)
                {
                    TRACE_D("Setting already exist");
                    broadcast_interval->broadcast_interval = read_value;
                }
                else
                {
                    TRACE_W("Not found saved setting for broadcast interval.");
                    broadcast_interval->broadcast_interval = BROADCAST_INTERVAL_DEFAULT;
                    if (!ezlopi_nvs_write_uint32(broadcast_interval->broadcast_interval, nvs_key_sound_detector_sensor_broadcast_interval_setting))
                    {
                        TRACE_E("Failed to write to NVS");
                        ret = 1;
                    }
                }
                broadcast_interval->user_arg = user_arg;
                device_setting->user_arg = broadcast_interval;
            }
            else
            {
                ezlopi_free(__FUNCTION__, device_setting);
                ret = 1;
            }
        }
        else
        {
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

int sensor_0096_sound_detector_settings_initialize(l_ezlopi_device_t *devices, void *user_arg)
{
    int ret = 0;

    ESP_ERROR_CHECK(__settings_sensor_0096_sound_detector_setting_initialize_broadcast_interval_setting(devices, user_arg));

    return ret;
}

static int __settings_callback(e_ezlopi_settings_action_t action, struct l_ezlopi_device_settings_v3 *setting, void *arg, void *user_arg)
{
    int ret = 0;

    switch (action)
    {
    case EZLOPI_SETTINGS_ACTION_GET_SETTING:
    {
        TRACE_D("EZLOPI_SETTINGS_ACTION_GET_SETTING");
        __settings_get(arg, setting);
        break;
    }
    case EZLOPI_SETTINGS_ACTION_SET_SETTING:
    {
        TRACE_D("EZLOPI_SETTINGS_ACTION_SET_SETTING");
        __settings_set(arg, setting);
        break;
    }
    case EZLOPI_SETTINGS_ACTION_RESET_SETTING:
    {
        TRACE_D("EZLOPI_SETTINGS_ACTION_RESET_SETTING");
        __settings_reset(arg, setting);
        break;
    }
    case EZLOPI_SETTINGS_ACTION_UPDATE_SETTING:
    {
        TRACE_D("EZLOPI_SETTINGS_ACTION_UPDATE_SETTING");
        __settings_update(arg, setting);
        break;
    }

    default:
    {
        break;
    }
    }

    return ret;
}

// ********************************************* Setting initialization related end ********************************************* //

// ********************************************* Setting getter related start ********************************************* //

static int __settings_get_sound_detector_sensor_broadcast_interval_get(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = 0;

    cJSON *cj_properties = (cJSON *)arg;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)setting->user_arg;
    if (cj_properties && broadcast_interval)
    {
        cJSON_AddItemToObject(__FUNCTION__, cj_properties, "label", __setting_add_text_and_lang_tag(SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_LABEL_TEXT, SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_LABEL_LANG_TAG));
        cJSON_AddItemToObject(__FUNCTION__, cj_properties, "description", __setting_add_text_and_lang_tag(SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_DESCRIPTION_TEXT, SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_DESCRIPTION_LANG_TAG));
        cJSON_AddStringToObject(__FUNCTION__, cj_properties, "status", "synced");
        cJSON_AddStringToObject(__FUNCTION__, cj_properties, "valueType", value_type_int);

        cJSON_AddNumberToObject(__FUNCTION__, cj_properties, ezlopi_value_str, broadcast_interval->broadcast_interval);
        cJSON_AddNumberToObject(__FUNCTION__, cj_properties, ezlopi_valueDefault_str, BROADCAST_INTERVAL_DEFAULT);
        cJSON_AddNumberToObject(__FUNCTION__, cj_properties, ezlopi_valueMax_str, BROADCAST_INTERVAL_MAX);
        cJSON_AddNumberToObject(__FUNCTION__, cj_properties, ezlopi_valueMin_str, BROADCAST_INTERVAL_MIN);
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static int __settings_get(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = ret = 0;
    ESP_ERROR_CHECK(__settings_get_sound_detector_sensor_broadcast_interval_get(arg, setting));
    return ret;
}
// ********************************************* Setting getter related end ********************************************* //

// ********************************************* Setting setter related start ********************************************* //

static int __settings_set_sound_detector_sensor_broadcast_interval_get(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = 0;

    cJSON *cj_properties = (cJSON *)arg;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)setting->user_arg;
    if (cj_properties && broadcast_interval)
    {
        float l_broadcast_interval = 0;
        CJSON_GET_VALUE_DOUBLE(cj_properties, ezlopi_value_str, l_broadcast_interval);
        if ((l_broadcast_interval >= BROADCAST_INTERVAL_MIN) && (l_broadcast_interval <= BROADCAST_INTERVAL_MAX))
        {
            broadcast_interval->broadcast_interval = l_broadcast_interval;
            if (!ezlopi_nvs_write_uint32(broadcast_interval->broadcast_interval, nvs_key_sound_detector_sensor_broadcast_interval_setting))
            {
                TRACE_E("Failed to write to NVS");
                ret = 1;
            }
        }
        else
        {
            TRACE_E("Broadcast interval out of range");
        }
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int __settings_set(void *arg, l_ezlopi_device_settings_v3_t *setting)
{

    int ret = 0;
    ESP_ERROR_CHECK(__settings_set_sound_detector_sensor_broadcast_interval_get(arg, setting));
    setting_changed = true;

    ezlopi_core_device_value_updated_settings_broadcast(setting);
    // ezlopi_cloud_settings_updated_from_devices_v3(NULL, setting);
    return ret;
}

// ********************************************* Setting setter related end ********************************************* //

// ********************************************* Setting updater related start ********************************************* //

static int __settings_update_sound_detector_sensor_broadcast_interval_get(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = 0;

    cJSON *cj_properties = (cJSON *)arg;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)setting->user_arg;
    if (cj_properties && broadcast_interval)
    {
        cJSON_AddNumberToObject(__FUNCTION__, cj_properties, ezlopi_value_str, broadcast_interval->broadcast_interval);
        // cJSON_AddStringToObject(__FUNCTION__, cj_properties, "scale", scales_celsius);
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int __settings_update(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = ret = 0;
    ESP_ERROR_CHECK(__settings_update_sound_detector_sensor_broadcast_interval_get(arg, setting));
    return ret;
}

// ********************************************* Setting updater related end ********************************************* //

// ********************************************* Setting resetter related start ********************************************* //

static int __settings_reset_sound_detector_sensor_broadcast_interval_get(void *arg, l_ezlopi_device_settings_v3_t *setting)
{
    int ret = 0;

    cJSON *cj_properties = (cJSON *)arg;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)setting->user_arg;
    if (cj_properties && broadcast_interval)
    {
        broadcast_interval->broadcast_interval = BROADCAST_INTERVAL_DEFAULT;
        if (!ezlopi_nvs_write_uint32(broadcast_interval->broadcast_interval, nvs_key_sound_detector_sensor_broadcast_interval_setting))
        {
            TRACE_E("Failed to write to NVS");
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int __settings_reset(void *arg, l_ezlopi_device_settings_v3_t *setting)
{

    int ret = ret = 0;
    ESP_ERROR_CHECK(__settings_reset_sound_detector_sensor_broadcast_interval_get(arg, setting));
    setting_changed = true;

    ezlopi_core_device_value_updated_settings_broadcast(setting);
    // ezlopi_cloud_settings_updated_from_devices_v3(NULL, setting);
    return ret;
}

// ********************************************* Setting resetter related end ********************************************* //

float sound_detector_get_broadcast_interval_setting()
{
    float ret = 0.0f;
    s_sensor_sound_detector_setting_broadcast_interval_t *broadcast_interval = (s_sensor_sound_detector_setting_broadcast_interval_t *)device_setting->user_arg;
    if (broadcast_interval)
    {
        ret = broadcast_interval->broadcast_interval;
    }
    return ret;
}

bool sound_detector_has_setting_changed()
{
    return setting_changed;
}

void sound_detector_set_setting_changed_to_false()
{
    setting_changed = false;
}

s_sensor_sound_detector_setting_broadcast_interval_t *sound_detector_get_broadcast_iterval_setting(void)
{
    return device_setting->user_arg;
}
