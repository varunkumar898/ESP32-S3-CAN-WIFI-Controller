#include "WIFI.h"
#include "ESP32S3.h"
#include "TIMER.h"
#include "SYSTEM.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WIFI";
static bool running = false;
static uint8_t timeoutMinutes = 2;
static uint32_t lastClientTime = 0;
static bool timeoutPending = false;
static esp_netif_t *apNetif = NULL;

void WiFi_Init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();

    apNetif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init);

    WiFi_StartAP();
}

void WiFi_StartAP(void)
{
    wifi_config_t config = {0};

    strncpy((char *)config.ap.ssid, APP_DEFAULT_WIFI_SSID, sizeof(config.ap.ssid));
    strncpy((char *)config.ap.password, APP_DEFAULT_WIFI_PASS, sizeof(config.ap.password));
    config.ap.ssid_len = strlen(APP_DEFAULT_WIFI_SSID);
    config.ap.channel = APP_AP_CHANNEL;
    config.ap.max_connection = 1;
    config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(APP_DEFAULT_WIFI_PASS) == 0)
        config.ap.authmode = WIFI_AUTH_OPEN;

    esp_netif_ip_info_t ip = {0};
    IP4_ADDR(&ip.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip.netmask, 255, 255, 255, 0);

    if (apNetif)
    {
        esp_netif_dhcps_stop(apNetif);
        esp_netif_set_ip_info(apNetif, &ip);
        esp_netif_dhcps_start(apNetif);
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &config);
    esp_wifi_start();

    running = true;
    timeoutPending = false;
    lastClientTime = Timer_Millis();

    ESP_LOGI(TAG, "Access point started");
}

void WiFi_StopAP(void)
{
    esp_wifi_stop();
    running = false;
}

bool WiFi_IsRunning(void)
{
    return running;
}

uint8_t WiFi_GetStationCount(void)
{
    wifi_sta_list_t list = {0};
    if (!running) return 0;
    if (esp_wifi_ap_get_sta_list(&list) != ESP_OK) return 0;
    return (uint8_t)list.num;
}

void WiFi_SetTimeoutMinutes(uint8_t minutes)
{
    if (minutes < 1) minutes = 1;
    if (minutes > 60) minutes = 60;
    timeoutMinutes = minutes;
}

uint8_t WiFi_GetTimeoutMinutes(void)
{
    return timeoutMinutes;
}

void WiFi_Task(void)
{
    if (!running)
        return;

    uint8_t stations = WiFi_GetStationCount();

    if (stations > 0)
        lastClientTime = Timer_Millis();

    if (timeoutMinutes == 0 || stations > 0)
        return;

    if (Timer_Millis() - lastClientTime >=
        (uint32_t)timeoutMinutes * 60000U)
    {
        timeoutPending = true;
    }

    if (timeoutPending)
    {
        WiFi_StopAP();
        timeoutPending = false;
        ESP_LOGI(TAG, "Access point stopped by timeout");
    }
}
