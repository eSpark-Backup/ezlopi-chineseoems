file(GLOB_RECURSE config_src "*.c")

set(EZLOPI_IDF_COMPONENTS
)

set(EZLOPI_CORE_COMPONENTS 
        ezlopi-core-nvs
        ezlopi-core-cloud 
        ezlopi-core-actions
        ezlopi-core-devices 
        ezlopi-core-errors
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
        device-00112-Corunsmart-Wifi-Smart-Charger-Socket
)


idf_component_register(SRCS "ezlopi_core_devices.c" "ezlopi_core_device_value_updated.c" "${config_src}"
                        INCLUDE_DIRS "."
                        REQUIRES        ${EZLOPI_IDF_COMPONENTS}
                                        ${EZLOPI_CORE_COMPONENTS}
                                        ${EZLOPI_HAL_COMPONENTS}
                                        ${EZLOPI_CLOUD_COMPONENTS}
                                        ${EZLOPI_SENSOR_DEVICE_COMPONENTS} 
                        )


