


idf_component_register(SRCS "rotary_dimmer.c" "device_00111_Rotary_Smart_Dimmer_Switch.c"
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