


idf_component_register(SRCS "device-00114-Moko-Smart-Plug-MK110Plus01.c"
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