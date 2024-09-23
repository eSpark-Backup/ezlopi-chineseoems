#pragma once
#include <stdint.h>
#include <stdbool.h>

uint32_t ezlopi_airbnb_get_parent_device_id(uint32_t l_parent_device_id);
void ezlopi_airbnb_set_rgb_led_status(bool status);
bool ezlopi_airbnb_get_rgb_led_status(void);
void ezlopi_airbnb_set_device(void *device);
void *ezlopi_airbnb_get_device();