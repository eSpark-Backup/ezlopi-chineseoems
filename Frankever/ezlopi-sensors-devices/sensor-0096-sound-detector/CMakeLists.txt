file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "sensor_0096_sound_detector_setttings.c" "sensor_0096_sound_detector.c" "${config_src}"
                    INCLUDE_DIRS "."
                    REQUIRES    ezlopi-util-trace 
                                ezlopi-core-devices
                                ezlopi-core-actions
                                ezlopi-core-valueformatter
                                
                                ezlopi-hal-uart
                                device-airbnb
                    )