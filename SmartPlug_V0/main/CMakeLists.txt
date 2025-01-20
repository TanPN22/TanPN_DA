idf_component_register(SRCS "smartconfig_main.c" "gpio_config.c" "send_data.c" "ADE7753.c" "wifi_station.c" "app_main.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "tb-cloud-root-ca.pem")
