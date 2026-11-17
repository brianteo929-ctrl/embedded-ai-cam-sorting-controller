idf_component_register(SRCS "main.c" "controller.c" "web.c"
                       INCLUDE_DIRS "."
                       REQUIRES motor fsm vision nvs_flash esp_wifi esp_netif esp_event esp_http_server esp_timer esp_driver_gpio bootloader_support)
