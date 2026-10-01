#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stdint.h>

void WiFi_Init(void);
void WiFi_Task(void);
void WiFi_StartAP(void);
void WiFi_StopAP(void);
bool WiFi_IsRunning(void);
uint8_t WiFi_GetStationCount(void);
void WiFi_SetTimeoutMinutes(uint8_t minutes);
uint8_t WiFi_GetTimeoutMinutes(void);

#endif
