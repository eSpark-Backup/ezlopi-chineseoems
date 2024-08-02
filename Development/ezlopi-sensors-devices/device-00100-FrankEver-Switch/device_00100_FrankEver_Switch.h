#ifndef _DEVICE_00100_FRANKEVER_SWITCH_H_
#define _DEVICE_00100_FRANKEVER_SWITCH_H_

#include "ezlopi_core_device_macros.h"

#if defined(DEVICE_00100_FRANKEVER_SWITCH)

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

typedef struct s_frankever_digio_settings
{
    int32_t settings_int_data;
} s_frankever_digio_settings_t;

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

int device_00100_FrankEver_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif

#endif // _DEVICE_00100_FRANKEVER_SWITCH_H_
