#ifndef _DEVICE_0001_DIGITALOUT_GENERIC_H_
#define _DEVICE_0001_DIGITALOUT_GENERIC_H_

#include "ezlopi_core_device_macros.h"

#if defined(DEVICE_0001_DIGITALOUT_GENERIC)

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

#define BLUE_LED1 GPIO_NUM_22
#define RGB_RED GPIO_NUM_26
#define RGB_BLUE GPIO_NUM_17
#define RGB_GREEN GPIO_NUM_23

typedef struct s_digio_settings
{
    int32_t settings_int_data;
} s_digio_settings_t;

typedef struct s_AC_power_delay
{
    int64_t time_delay;
    int64_t interrupttime;
} s_AC_power_delay_t;

typedef struct s_switch_features
{
    bool single_press_flag;
    bool double_press_flag;
    uint8_t long_press_flag;
    bool button_press_flag;
    bool interrupt_flag;
} s_switch_features_t;

int device_0001_digitalOut_generic(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif

#endif // _DEVICE_0001_DIGITALOUT_GENERIC_H_
