#include "NVS.h"
#include "ESP32S3.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS";
static const char *NAMESPACE = "controller";

void NVS_Init(void)
{
    esp_err_t result = nvs_flash_init();

    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        result = nvs_flash_init();
    }

    if (result != ESP_OK)
        ESP_LOGE(TAG, "NVS initialization failed");
}

static bool Open(nvs_handle_t *handle, nvs_open_mode_t mode)
{
    return nvs_open(NAMESPACE, mode, handle) == ESP_OK;
}

#define GET_BOOL(h,k,d) ({ bool _v=(d); nvs_get_u8((h),(k),(uint8_t*)&_v); _v; })

bool NVS_LoadConfiguration(SystemConfig_t *config)
{
    if (!config) return false;

    nvs_handle_t h;
    if (!Open(&h, NVS_READONLY)) return false;

    size_t size = sizeof(*config);
    esp_err_t result = nvs_get_blob(h, "config", config, &size);
    nvs_close(h);

    return result == ESP_OK && size == sizeof(*config);
}

bool NVS_SaveConfiguration(const SystemConfig_t *config)
{
    if (!config) return false;

    nvs_handle_t h;
    if (!Open(&h, NVS_READWRITE)) return false;

    esp_err_t result = nvs_set_blob(h, "config", config, sizeof(*config));
    if (result == ESP_OK) result = nvs_commit(h);
    nvs_close(h);

    return result == ESP_OK;
}

bool NVS_LoadCalibration(SystemState_t *state)
{
    (void)state;
    return true;
}

bool NVS_SaveCalibration(const SystemState_t *state)
{
    (void)state;
    return true;
}

void NVS_Reset(void)
{
    nvs_handle_t h;
    if (!Open(&h, NVS_READWRITE)) return;
    nvs_erase_all(h);
    nvs_commit(h);
    nvs_close(h);
}
