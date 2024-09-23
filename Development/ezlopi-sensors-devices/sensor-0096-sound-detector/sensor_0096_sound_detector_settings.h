

#ifndef _SENSOR_ENS160_GAS_SENSOR_SETTINGS_H_
#define _SENSOR_ENS160_GAS_SENSOR_SETTINGS_H_

#include "cjext.h"

#include "ezlopi_util_trace.h"

#include "ezlopi_core_devices.h"

#define BROADCAST_INTERVAL_DEFAULT 30
#define BROADCAST_INTERVAL_MAX 120
#define BROADCAST_INTERVAL_MIN 5

#define SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_LABEL_TEXT "Sound Sensor value update interval"
#define SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_DESCRIPTION_TEXT "Select detected sound level update interval in seconds."

#define SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_LABEL_LANG_TAG "ezlopi_sound_sensor_broadcast_interval_tag_label"
#define SENSOR_SOUND_DETECTOR_SETTING_BROADCAST_INTERVAL_DESCRIPTION_LANG_TAG "ezlopi_sound_sensor_broadcast_interval_description"

typedef struct s_sensor_sound_detector_setting_broadcast_interval
{
    uint32_t broadcast_interval;
    void *user_arg;
} s_sensor_sound_detector_setting_broadcast_interval_t;

int sensor_0096_sound_detector_settings_initialize(l_ezlopi_device_t *devices, void *user_arg);
float sound_detector_get_broadcast_interval_setting();
bool sound_detector_has_setting_changed();
void sound_detector_set_setting_changed_to_false();
s_sensor_sound_detector_setting_broadcast_interval_t *sound_detector_get_broadcast_iterval_setting(void);

static inline cJSON *__setting_add_text_and_lang_tag(const char *const object_text, const char *const object_lang_tag)
{

    cJSON *cj_object = cJSON_CreateObject(__FUNCTION__);
    if (cj_object)
    {
        cJSON_AddStringToObject(__FUNCTION__, cj_object, ezlopi_text_str, object_text);
        cJSON_AddStringToObject(__FUNCTION__, cj_object, ezlopi_lang_tag_str, object_lang_tag);
    }
    else
    {
        ezlopi_free(__FUNCTION__, cj_object);
        cj_object = NULL;
    }

    return cj_object;
}

#endif // _SENSOR_ENS160_GAS_SENSOR_SETTINGS_H_
