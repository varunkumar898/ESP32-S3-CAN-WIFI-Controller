idf_component_register(
    SRCS
        "../src/main.c"
        "../src/GPIO.c"
        "../src/TIMER.c"
        "../src/TWAI.c"
        "../src/DISPLAY.c"
        "../src/RELAY.c"
        "../src/LOAD_CELL.c"
        "../src/NVS.c"
        "../src/WIFI.c"
        "../src/DNS.c"
        "../src/WEB_SERVER.c"
        "../src/SYSTEM.c"
    INCLUDE_DIRS "../inc"
    EMBED_FILES
        "../web/dashboard.html"
        "../web/settings.html"
    REQUIRES driver esp_wifi esp_event esp_netif esp_http_server nvs_flash
)
