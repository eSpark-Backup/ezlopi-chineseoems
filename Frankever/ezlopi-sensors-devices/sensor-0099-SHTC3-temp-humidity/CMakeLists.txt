file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "shtc3.c" "sensor_0099_SHTC3_temp_humidity.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES    ezlopi-util-trace 
    
                # ezlopi-core-timer 
                ezlopi-core-cloud 
                ezlopi-core-actions 
                ezlopi-core-devices 
                ezlopi-core-valueformatter 
                ezlopi-hal-adc

                ezlopi-cloud-items 
                ezlopi-cloud-constants 
                )