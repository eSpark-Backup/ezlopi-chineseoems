
idf_component_register(SRCS "device_00108_DADB_smart_scent_diffuser.c" "scent_diffuser.c"
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