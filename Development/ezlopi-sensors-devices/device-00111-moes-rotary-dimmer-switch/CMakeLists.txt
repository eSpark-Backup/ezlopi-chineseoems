


idf_component_register(SRCS "moes_dimmer.c" "device_00111_Moes_Rotary_Dimmer_Switch.c" "moes_dimmer.c" "device_00111_Moes_Rotary_Dimmer_Switch.c"
                        INCLUDE_DIRS .
                        REQUIRES    ezlopi-util-trace 
                                    # ezlopi-core-timer 
                                    ezlopi-core-cloud 
                                    ezlopi-core-actions 
                                    ezlopi-core-devices 
                                    ezlopi-core-valueformatter 
                                    
                                    ezlopi-cloud-items 
                                    ezlopi-cloud-constants  
                                    
                                    ezlopi-service-gpioisr
                                    )