
file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "pca9536.c" "${config_src}"
                    INCLUDE_DIRS "."
                    REQUIRES    ezlopi-util-trace                                 
                                ezlopi-hal-i2c
                    )