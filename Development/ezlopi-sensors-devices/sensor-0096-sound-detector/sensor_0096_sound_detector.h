

#ifndef _SENSOR_0096_SOUND_DETECTOR_H_
#define _SENSOR_0096_SOUND_DETECTOR_H_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

#define HEADER 0xAA
#define PAYLOAD_LENGTH 4
#define PACKET_SIZE 7 // Header (1) + Length (1) + Payload (4) + Checksum (1)

#define DEBUG_STM32_SOUND 0
    
typedef struct stm32_sound_intensity
{
    float dbSPL;
} stm32_sound_intensity_t;

int sensor_0096_sound_detector(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // _SENSOR_0096_SOUND_DETECTOR_H_
