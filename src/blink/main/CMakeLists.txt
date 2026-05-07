idf_component_register(SRCS "cloudiness.c" "bh1750.c" "upload_manager.c" "measurement_scheduler.c" "measurement.c" "time_manager.c" "wifi_manager.c" "ens160_aht21.c" "bmp280.c" "display.c" "blink_example_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_timer nvs_flash esp_http_client mbedtls esp-tls)
