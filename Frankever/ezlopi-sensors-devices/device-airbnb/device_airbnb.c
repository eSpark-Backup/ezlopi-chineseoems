#include <stddef.h>
#include "device_airbnb.h"

static uint32_t parent_device_id = 0;
static bool rgb_led_status = 0;
void *parent_device = NULL;

uint32_t ezlopi_airbnb_get_parent_device_id(uint32_t l_parent_device_id)
{
    if (!parent_device_id)
    {
        parent_device_id = l_parent_device_id;
        return 0;
    }
    return parent_device_id;
}

void ezlopi_airbnb_set_rgb_led_status(bool status)
{
    rgb_led_status = status;
}

bool ezlopi_airbnb_get_rgb_led_status(void)
{
    return rgb_led_status;
}

void *ezlopi_airbnb_get_device()
{
    return parent_device;
}

void ezlopi_airbnb_set_device(void *device)
{
    parent_device = device;
}