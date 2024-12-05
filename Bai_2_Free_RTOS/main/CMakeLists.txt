# Register the main component
idf_component_register(SRCS "app_main.c"
                            "../common/input_iot/input_iot.c"
                            "../common/output_iot/output_iot.c"
                    INCLUDE_DIRS "."
                    "../common/input_iot"
                    "../common/output_iot")