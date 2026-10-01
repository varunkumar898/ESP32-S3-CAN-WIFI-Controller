#include "DNS.h"
#include "ESP32S3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <stdint.h>

static int dnsSocket = -1;
static bool running = false;
static TaskHandle_t dnsTaskHandle = NULL;

static void DNS_Worker(void *argument)
{
    (void)argument;
    uint8_t buffer[512];
    uint8_t response[512];

    while (running)
    {
        struct sockaddr_in client;
        socklen_t clientLength = sizeof(client);
        int length = recvfrom(dnsSocket, buffer, sizeof(buffer), MSG_DONTWAIT,
                              (struct sockaddr *)&client, &clientLength);

        if (length < 12)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if ((size_t)length > sizeof(response) - 16)
            continue;

        memcpy(response, buffer, length);

        response[2] = 0x81;
        response[3] = 0x80;
        response[4] = 0x00;
        response[5] = 0x01;
        response[6] = 0x00;
        response[7] = 0x00;
        response[8] = 0x00;
        response[9] = 0x00;
        response[10] = 0x00;
        response[11] = 0x00;

        int pos = length;
        response[pos++] = 0xC0;
        response[pos++] = 0x0C;
        response[pos++] = 0x00;
        response[pos++] = 0x01;
        response[pos++] = 0x00;
        response[pos++] = 0x01;
        response[pos++] = 0x00;
        response[pos++] = 0x00;
        response[pos++] = 0x00;
        response[pos++] = 0x3C;
        response[pos++] = 0x00;
        response[pos++] = 0x04;
        response[pos++] = 192;
        response[pos++] = 168;
        response[pos++] = 4;
        response[pos++] = 1;

        sendto(dnsSocket, response, pos, 0,
               (struct sockaddr *)&client, clientLength);
    }

    vTaskDelete(NULL);
}

void DNS_Init(void)
{
    dnsSocket = -1;
    running = false;
}

void DNS_Start(void)
{
    if (running)
        return;

    dnsSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dnsSocket < 0)
        return;

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(APP_DNS_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(dnsSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        close(dnsSocket);
        dnsSocket = -1;
        return;
    }

    running = true;
    xTaskCreate(DNS_Worker, "dns_task", 3072, NULL, 3, &dnsTaskHandle);
}

void DNS_Stop(void)
{
    running = false;
    if (dnsSocket >= 0)
    {
        close(dnsSocket);
        dnsSocket = -1;
    }
}

void DNS_Task(void)
{
}

bool DNS_IsRunning(void)
{
    return running;
}
