#include "TWAI.h"
#include "ESP32S3.h"
#include "driver/twai.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "TWAI";
static bool driverReady = false;

void TWAI_Init(void)
{
    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(
        APP_CAN_TX_PIN,
        APP_CAN_RX_PIN,
        TWAI_MODE_NORMAL);

    twai_timing_config_t timing = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&general, &timing, &filter) != ESP_OK)
    {
        ESP_LOGE(TAG, "TWAI driver installation failed");
        return;
    }

    if (twai_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "TWAI start failed");
        twai_driver_uninstall();
        return;
    }

    driverReady = true;
    ESP_LOGI(TAG, "TWAI started at 250 kbit/s");
}

bool TWAI_Send(const TWAI_Message_t *message)
{
    if (!driverReady || message == NULL || message->dlc > 8)
        return false;

    twai_message_t frame = {0};
    frame.identifier = message->id;
    frame.data_length_code = message->dlc;

    if (message->extended)
        frame.flags |= TWAI_MSG_FLAG_EXTD;

    memcpy(frame.data, message->data, message->dlc);

    return twai_transmit(&frame, pdMS_TO_TICKS(100)) == ESP_OK;
}

bool TWAI_Receive(TWAI_Message_t *message)
{
    if (!driverReady || message == NULL)
        return false;

    twai_message_t frame;

    if (twai_receive(&frame, 0) != ESP_OK)
        return false;

    message->id = frame.identifier;
    message->dlc = frame.data_length_code;
    message->extended = (frame.flags & TWAI_MSG_FLAG_EXTD) != 0;
    memcpy(message->data, frame.data, message->dlc);

    return true;
}

bool TWAI_IsReady(void)
{
    return driverReady;
}

void TWAI_Task(void)
{
    if (!driverReady)
        return;
}
