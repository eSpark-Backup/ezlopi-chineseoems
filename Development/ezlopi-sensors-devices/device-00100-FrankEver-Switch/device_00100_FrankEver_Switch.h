#ifndef _DEVICE_00100_FRANKEVER_SWITCH_H_
#define _DEVICE_00100_FRANKEVER_SWITCH_H_

#include "ezlopi_core_device_macros.h"

#if defined(DEVICE_00100_FRANKEVER_SWITCH)

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

ezlopi_error_t device_00100_FrankEver_Switch(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif

#endif // _DEVICE_00100_FRANKEVER_SWITCH_H_
