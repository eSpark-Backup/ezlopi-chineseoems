
file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "sensor-0098-I2C-tamper-switch.c" "${config_src}"
                    INCLUDE_DIRS "."
                    REQUIRES    ezlopi-util-trace 
                                ezlopi-core-devices
                                ezlopi-core-actions
                                ezlopi-core-valueformatter
                                ezlopi-core-nvs

                                ezlopi-cloud-constants
                                pca9536-io-expander
                                ezlopi-hal-i2c
                                device-airbnb
                    )