#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ESP32S3.h"
#include "GPIO.h"
#include "TIMER.h"
#include "TWAI.h"
#include "DISPLAY.h"
#include "RELAY.h"
#include "LOAD_CELL.h"
#include "NVS.h"
#include "WIFI.h"
#include "DNS.h"
#include "WEB_SERVER.h"
#include "SYSTEM.h"

static const char *TAG = "MAIN";

static void DisplayTask(void *argument)
{
    (void)argument;
    TickType_t lastWake = xTaskGetTickCount();

    while (1)
    {
        Display_Task();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2));
    }
}

static void ControlTask(void *argument)
{
    (void)argument;

    while (1)
    {
        TWAI_Message_t message;

        while (TWAI_Receive(&message))
            System_ProcessCANMessage(&message);

        System_Task();
        System_SafetyTask();
        TWAI_Task();
        Relay_Task();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void NetworkTask(void *argument)
{
    (void)argument;

    while (1)
    {
        WiFi_Task();
        DNS_Task();
        WebServer_Task();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Application start");

    System_Init();
    NVS_Init();
    System_LoadConfiguration();

    GPIO_Init();
    Timer_Init();
    Relay_Init();
    Display_Init();
    LoadCell_Init();
    TWAI_Init();

    WiFi_Init();
    DNS_Init();
    DNS_Start();
    WebServer_Init();

    xTaskCreatePinnedToCore(
        DisplayTask,
        "display_task",
        4096,
        NULL,
        10,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        ControlTask,
        "control_task",
        6144,
        NULL,
        8,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        NetworkTask,
        "network_task",
        6144,
        NULL,
        4,
        NULL,
        0);

    while (1)
    {
        Timer_Task();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
