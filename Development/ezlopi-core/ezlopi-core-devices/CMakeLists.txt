file(GLOB_RECURSE config_src "*.c")

set(EZLOPI_CORE_COMPONENTS 
        ezlopi-core-cloud 
        ezlopi-core-actions
        ezlopi-core-devices 
)
set(EZLOPI_HAL_COMPONENTS 
        ezlopi-hal-gpio
        ezlopi-hal-adc
        ezlopi-hal-pwm
        ezlopi-hal-onewire     
        ezlopi-hal-uart                           
        ezlopi-hal-i2c
        ezlopi-hal-spi
)
set(EZLOPI_CLOUD_COMPONENTS 
        ezlopi-cloud-settings
        ezlopi-cloud-items
)
set(EZLOPI_SENSOR_DEVICE_COMPONENTS 
        device-00102-Beca-BHT003-GCLW
)


idf_component_register(SRCS "ezlopi_core_devices.c" "ezlopi_core_device_group.c" "${config_src}"
                        INCLUDE_DIRS "."
                        REQUIRES        ${EZLOPI_CORE_COMPONENTS}
                                        ${EZLOPI_HAL_COMPONENTS}
                                        ${EZLOPI_CLOUD_COMPONENTS}
                                        ${EZLOPI_SENSOR_DEVICE_COMPONENTS} 
                        )


