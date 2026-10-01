#include "WEB_SERVER.h"
#include "SYSTEM.h"
#include "LOAD_CELL.h"
#include "RELAY.h"
#include "WIFI.h"
#include "NVS.h"
#include "DISPLAY.h"
#include "ESP32S3.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern const char dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_html_end[]   asm("_binary_dashboard_html_end");
extern const char settings_html_start[]  asm("_binary_settings_html_start");
extern const char settings_html_end[]    asm("_binary_settings_html_end");

static const char *TAG = "WEB";
static httpd_handle_t server = NULL;

static void SendText(httpd_req_t *request, const char *text, const char *type)
{
    httpd_resp_set_type(request, type);
    httpd_resp_sendstr(request, text);
}

static bool GetQuery(httpd_req_t *request, const char *name, char *value, size_t size)
{
    size_t length = httpd_req_get_url_query_len(request);
    if (length == 0) return false;

    char *query = malloc(length + 1);
    if (!query) return false;

    if (httpd_req_get_url_query_str(request, query, length + 1) != ESP_OK)
    {
        free(query);
        return false;
    }

    esp_err_t result = httpd_query_key_value(query, name, value, size);
    free(query);
    return result == ESP_OK;
}

static esp_err_t RootHandler(httpd_req_t *request)
{
    size_t length = dashboard_html_end - dashboard_html_start;
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, dashboard_html_start, length);
}

static esp_err_t SettingsHandler(httpd_req_t *request)
{
    size_t length = settings_html_end - settings_html_start;
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, settings_html_start, length);
}

static esp_err_t SwitchHandler(httpd_req_t *request)
{
    char name[24];
    char stateText[8];

    if (!GetQuery(request, "name", name, sizeof(name)) ||
        !GetQuery(request, "state", stateText, sizeof(stateText)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_OK;
    }

    bool state = atoi(stateText) != 0;
    SystemConfig_t *config = System_GetConfig();

    if (strcmp(name, "rope") == 0) config->ropeSwitch = state;
    else if (strcmp(name, "anemo") == 0) config->anemoSwitch = state;
    else if (strcmp(name, "atp") == 0) config->atpSwitch = state;
    else if (strcmp(name, "anemoDisplay") == 0) config->anemoDisplayEnable = state;
    else
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid switch");
        return ESP_OK;
    }

    System_SaveConfiguration();
    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t SwitchStateHandler(httpd_req_t *request)
{
    SystemConfig_t *c = System_GetConfig();
    char json[256];

    snprintf(json, sizeof(json),
             "{\"rope\":%s,\"anemo\":%s,\"anemoDisplay\":%s,\"atp\":%s}",
             c->ropeSwitch ? "true" : "false",
             c->anemoSwitch ? "true" : "false",
             c->anemoDisplayEnable ? "true" : "false",
             c->atpSwitch ? "true" : "false");

    SendText(request, json, "application/json");
    return ESP_OK;
}

static esp_err_t LiveWeightHandler(httpd_req_t *request)
{
    SystemState_t state;
    System_GetState(&state);

    char text[32];
    snprintf(text, sizeof(text), "%.2f", state.displayWeight);
    SendText(request, text, "text/plain");
    return ESP_OK;
}

static esp_err_t ConfigHandler(httpd_req_t *request, bool aux)
{
    SystemConfig_t *c = System_GetConfig();
    char json[256];

    if (aux)
    {
        snprintf(json, sizeof(json),
                 "{\"enable\":%s,\"alarm\":%s,\"capacity\":%.3f,\"percent\":%.3f}",
                 c->auxLoadEnable ? "true" : "false",
                 c->auxOverloadEnable ? "true" : "false",
                 c->auxCapacity,
                 c->auxOverloadPercent);
    }
    else
    {
        snprintf(json, sizeof(json),
                 "{\"enable\":%s,\"alarm\":%s,\"capacity\":%.3f,\"percent\":%.3f}",
                 c->mainLoadEnable ? "true" : "false",
                 c->mainOverloadEnable ? "true" : "false",
                 c->mainCapacity,
                 c->mainOverloadPercent);
    }

    SendText(request, json, "application/json");
    return ESP_OK;
}

static esp_err_t MainConfigGet(httpd_req_t *request) { return ConfigHandler(request, false); }
static esp_err_t AuxConfigGet(httpd_req_t *request) { return ConfigHandler(request, true); }

static esp_err_t SaveConfigHandler(httpd_req_t *request, bool aux)
{
    char enable[8], alarm[8], capacity[24], percent[24];

    if (!GetQuery(request, "enable", enable, sizeof(enable)) ||
        !GetQuery(request, "alarm", alarm, sizeof(alarm)) ||
        !GetQuery(request, "capacity", capacity, sizeof(capacity)) ||
        !GetQuery(request, "percent", percent, sizeof(percent)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_OK;
    }

    if (aux)
    {
        LoadCell_SetAuxConfig(atoi(enable) != 0, atoi(alarm) != 0,
                              strtof(capacity, NULL), strtof(percent, NULL));
    }
    else
    {
        LoadCell_SetMainConfig(atoi(enable) != 0, atoi(alarm) != 0,
                               strtof(capacity, NULL), strtof(percent, NULL));
    }

    SystemConfig_t *c = System_GetConfig();
    LoadCell_GetMainConfig(&c->mainLoadEnable, &c->mainOverloadEnable,
                           &c->mainCapacity, &c->mainOverloadPercent);
    LoadCell_GetAuxConfig(&c->auxLoadEnable, &c->auxOverloadEnable,
                          &c->auxCapacity, &c->auxOverloadPercent);
    System_SaveConfiguration();

    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t SaveMainConfig(httpd_req_t *r) { return SaveConfigHandler(r, false); }
static esp_err_t SaveAuxConfig(httpd_req_t *r) { return SaveConfigHandler(r, true); }

static esp_err_t NoLoadGet(httpd_req_t *request)
{
    char hook[8];
    if (!GetQuery(request, "hook", hook, sizeof(hook)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing hook");
        return ESP_OK;
    }

    char text[32];
    snprintf(text, sizeof(text), "%ld",
             strcmp(hook, "aux") == 0 ?
             (long)LoadCell_GetAuxNoLoad() :
             (long)LoadCell_GetMainNoLoad());
    SendText(request, text, "text/plain");
    return ESP_OK;
}

static esp_err_t NoLoadSave(httpd_req_t *request)
{
    char hook[8], value[32];
    if (!GetQuery(request, "hook", hook, sizeof(hook)) ||
        !GetQuery(request, "value", value, sizeof(value)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_OK;
    }

    if (strcmp(hook, "aux") == 0)
        LoadCell_SetAuxNoLoad(strtol(value, NULL, 10));
    else
        LoadCell_SetMainNoLoad(strtol(value, NULL, 10));

    System_SaveConfiguration();
    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t RelayConfigGet(httpd_req_t *request)
{
    char json[64];
    snprintf(json, sizeof(json), "[%u,%u,%u]",
             Relay_GetConfig(0), Relay_GetConfig(1), Relay_GetConfig(2));
    SendText(request, json, "application/json");
    return ESP_OK;
}

static esp_err_t RelayConfigSave(httpd_req_t *request)
{
    char relay[8], parameter[8], stateText[8];
    if (!GetQuery(request, "relay", relay, sizeof(relay)) ||
        !GetQuery(request, "param", parameter, sizeof(parameter)) ||
        !GetQuery(request, "state", stateText, sizeof(stateText)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_OK;
    }

    int r = atoi(relay);
    int p = atoi(parameter);
    bool active = atoi(stateText) != 0;

    if (r < 0 || r >= 3 || p < 0 || p >= 5)
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid index");
        return ESP_OK;
    }

    uint8_t mask = Relay_GetConfig((uint8_t)r);
    if (active) mask |= (1U << p);
    else mask &= ~(1U << p);

    Relay_SetConfig((uint8_t)r, mask);
    System_GetConfig()->relayConfig[r] = mask;
    System_SaveConfiguration();

    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t AlphaGet(httpd_req_t *request)
{
    char text[32];
    snprintf(text, sizeof(text), "%.3f", System_GetConfig()->alphaValue);
    SendText(request, text, "text/plain");
    return ESP_OK;
}

static esp_err_t AlphaSave(httpd_req_t *request)
{
    char pass[16], alpha[16];
    if (!GetQuery(request, "pass", pass, sizeof(pass)) ||
        !GetQuery(request, "alpha", alpha, sizeof(alpha)))
    {
        httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Missing passcode");
        return ESP_OK;
    }

    if (strcmp(pass, APP_ALPHA_PASSCODE) != 0)
    {
        httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Invalid passcode");
        return ESP_OK;
    }

    System_GetConfig()->alphaValue = strtof(alpha, NULL);
    if (System_GetConfig()->alphaValue < 0.0f) System_GetConfig()->alphaValue = 0.0f;
    if (System_GetConfig()->alphaValue > 1.0f) System_GetConfig()->alphaValue = 1.0f;
    System_SaveConfiguration();

    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t WifiTimeoutGet(httpd_req_t *request)
{
    char text[8];
    snprintf(text, sizeof(text), "%u", WiFi_GetTimeoutMinutes());
    SendText(request, text, "text/plain");
    return ESP_OK;
}

static esp_err_t WifiTimeoutSave(httpd_req_t *request)
{
    char value[8];
    if (!GetQuery(request, "minutes", value, sizeof(value)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing minutes");
        return ESP_OK;
    }

    int minutes = atoi(value);
    if (minutes < 1 || minutes > 60)
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Range 1-60");
        return ESP_OK;
    }

    WiFi_SetTimeoutMinutes((uint8_t)minutes);
    System_GetConfig()->wifiTimeoutMinutes = (uint8_t)minutes;
    System_SaveConfiguration();

    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t DisplayThresholdGet(httpd_req_t *request)
{
    SystemConfig_t *c = System_GetConfig();
    char json[128];
    snprintf(json, sizeof(json),
             "{\"threshold\":%.3f,\"delay\":%u,\"unit\":%u}",
             c->displayThreshold, c->displayOffDelay, c->displayUnit);
    SendText(request, json, "application/json");
    return ESP_OK;
}

static esp_err_t DisplayThresholdSave(httpd_req_t *request)
{
    char threshold[16], delay[8], unit[8];
    if (!GetQuery(request, "threshold", threshold, sizeof(threshold)) ||
        !GetQuery(request, "delay", delay, sizeof(delay)) ||
        !GetQuery(request, "unit", unit, sizeof(unit)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_OK;
    }

    SystemConfig_t *c = System_GetConfig();
    c->displayThreshold = strtof(threshold, NULL);
    c->displayOffDelay = (uint8_t)atoi(delay);
    c->displayUnit = (uint8_t)atoi(unit);

    if (c->displayThreshold < 0.0f) c->displayThreshold = 0.0f;
    if (c->displayOffDelay < 1) c->displayOffDelay = 1;
    if (c->displayOffDelay > 60) c->displayOffDelay = 60;
    if (c->displayUnit > 1) c->displayUnit = 1;

    System_SaveConfiguration();
    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static esp_err_t DecimalHandler(httpd_req_t *request)
{
    char mode[8];
    if (GetQuery(request, "mode", mode, sizeof(mode)))
    {
        System_GetConfig()->decimalMode = atoi(mode) == 2 ? 2 : 1;
        Display_SetDecimalMode(System_GetConfig()->decimalMode);
        System_SaveConfiguration();
    }

    char text[8];
    snprintf(text, sizeof(text), "%u", System_GetConfig()->decimalMode);
    SendText(request, text, "text/plain");
    return ESP_OK;
}

static esp_err_t SensorMapGet(httpd_req_t *request)
{
    char json[256];
    bool swap = LoadCell_GetSwap();
    snprintf(json, sizeof(json),
             "{\"swap\":%s,\"mainSensor\":\"%s\",\"auxSensor\":\"%s\"}",
             swap ? "true" : "false",
             swap ? "HX717" : "ADS1232",
             swap ? "ADS1232" : "HX717");
    SendText(request, json, "application/json");
    return ESP_OK;
}

static esp_err_t SensorMapSave(httpd_req_t *request)
{
    char value[8];
    if (!GetQuery(request, "swap", value, sizeof(value)))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing swap");
        return ESP_OK;
    }

    bool swap = atoi(value) != 0;
    LoadCell_SetSwap(swap);
    System_GetConfig()->swapSensors = swap;
    System_SaveConfiguration();
    SendText(request, "OK", "text/plain");
    return ESP_OK;
}

static void Register(httpd_uri_t *uri, const char *path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *))
{
    uri->uri = path;
    uri->method = method;
    uri->handler = handler;
    uri->user_ctx = NULL;
    httpd_register_uri_handler(server, uri);
}

void WebServer_Init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = APP_HTTP_PORT;
    config.max_uri_handlers = 40;
    config.stack_size = 8192;

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP server start failed");
        return;
    }

    static httpd_uri_t root = {0};
    static httpd_uri_t settings = {0};
    static httpd_uri_t switches = {0};
    static httpd_uri_t switchStates = {0};
    static httpd_uri_t live = {0};
    static httpd_uri_t mainConfig = {0};
    static httpd_uri_t auxConfig = {0};
    static httpd_uri_t saveMain = {0};
    static httpd_uri_t saveAux = {0};
    static httpd_uri_t noLoad = {0};
    static httpd_uri_t saveNoLoad = {0};
    static httpd_uri_t relayGet = {0};
    static httpd_uri_t relaySave = {0};
    static httpd_uri_t alphaGet = {0};
    static httpd_uri_t alphaSave = {0};
    static httpd_uri_t wifiGet = {0};
    static httpd_uri_t wifiSave = {0};
    static httpd_uri_t displayGet = {0};
    static httpd_uri_t displaySave = {0};
    static httpd_uri_t decimal = {0};
    static httpd_uri_t sensorGet = {0};
    static httpd_uri_t sensorSave = {0};

    Register(&root, "/", HTTP_GET, RootHandler);
    Register(&settings, "/settings", HTTP_GET, SettingsHandler);
    Register(&switches, "/saveSwitch", HTTP_GET, SwitchHandler);
    Register(&switchStates, "/getSwitchStates", HTTP_GET, SwitchStateHandler);
    Register(&live, "/liveWeight", HTTP_GET, LiveWeightHandler);
    Register(&mainConfig, "/getMainConfig", HTTP_GET, MainConfigGet);
    Register(&auxConfig, "/getAuxConfig", HTTP_GET, AuxConfigGet);
    Register(&saveMain, "/saveMainConfig", HTTP_GET, SaveMainConfig);
    Register(&saveAux, "/saveAuxConfig", HTTP_GET, SaveAuxConfig);
    Register(&noLoad, "/getNoLoad", HTTP_GET, NoLoadGet);
    Register(&saveNoLoad, "/saveNoLoad", HTTP_GET, NoLoadSave);
    Register(&relayGet, "/getRelayConfig", HTTP_GET, RelayConfigGet);
    Register(&relaySave, "/saveRelayConfig", HTTP_GET, RelayConfigSave);
    Register(&alphaGet, "/getAlpha", HTTP_GET, AlphaGet);
    Register(&alphaSave, "/saveAlpha", HTTP_GET, AlphaSave);
    Register(&wifiGet, "/getWifiTimeout", HTTP_GET, WifiTimeoutGet);
    Register(&wifiSave, "/saveWifiTimeout", HTTP_GET, WifiTimeoutSave);
    Register(&displayGet, "/getDispThreshold", HTTP_GET, DisplayThresholdGet);
    Register(&displaySave, "/saveDispThreshold", HTTP_GET, DisplayThresholdSave);
    Register(&decimal, "/decimal", HTTP_GET, DecimalHandler);
    Register(&sensorGet, "/getSensorMap", HTTP_GET, SensorMapGet);
    Register(&sensorSave, "/saveSensorMap", HTTP_GET, SensorMapSave);

    ESP_LOGI(TAG, "HTTP server started");
}

void WebServer_Task(void)
{
}

void WebServer_Stop(void)
{
    if (server)
    {
        httpd_stop(server);
        server = NULL;
    }
}

bool WebServer_IsRunning(void)
{
    return server != NULL;
}
