


idf_component_register(SRCS "device_00109_Xuguang_Wifi_Dimmer_Switch.c" "xuguang_dimmer.c"
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