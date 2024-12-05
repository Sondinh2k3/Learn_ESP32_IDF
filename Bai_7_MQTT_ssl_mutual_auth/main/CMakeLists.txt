idf_component_register(SRCS "http_server_app.c" "app_config.c" "app_main.c" "app_mqtt.c"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
