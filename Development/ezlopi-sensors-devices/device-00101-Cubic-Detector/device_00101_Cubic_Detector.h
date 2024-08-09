
#ifndef _DEVICE_00101_CUBIC_DETECTOR_H_
#define _DEVICE_00101_CUBIC_DETECTOR_H_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

typedef enum
{
    NEO_WATERLEAK_NO_WATER_LEAK_OK,
    NEO_WATERLEAK_WATER_LEAK_DETECTED,
    NEO_WATERLEAK_UNKNOWN,
    NEO_WATERLEAK_MAX
} e_neo_water_leak_alarm_states_t;

int device_00101_Cubic_Detector(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *args, void *user_arg);

#endif // _DEVICE_00101_CUBIC_DETECTOR_H_