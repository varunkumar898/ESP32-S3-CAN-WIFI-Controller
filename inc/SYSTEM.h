#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "TWAI.h"

#define SYSTEM_LOAD_COUNT 4

typedef struct
{
    float variation;
    float setX;
    float setY;
    float offsetX;
    float offsetY;
    float windLimit;
    float alphaValue;
    uint8_t decimalMode;
    bool ropeSwitch;
    bool anemoSwitch;
    bool atpSwitch;
    bool anemoDisplayEnable;
    bool mainLoadEnable;
    bool mainOverloadEnable;
    float mainCapacity;
    float mainOverloadPercent;
    bool auxLoadEnable;
    bool auxOverloadEnable;
    float auxCapacity;
    float auxOverloadPercent;
    bool swapSensors;
    uint8_t relayConfig[3];
    uint8_t wifiTimeoutMinutes;
    float displayThreshold;
    uint8_t displayOffDelay;
    uint8_t displayUnit;
} SystemConfig_t;

typedef struct
{
    float pitchX;
    float pitchY;
    float dacX;
    float dacY;
    float windSpeed;
    float windFiltered;
    float displayWeight;
    bool canValid;
    bool canPhysicalError;
    bool anemoError;
    bool mainOverloadActive;
    bool auxOverloadActive;
    bool ropeActive;
    bool atpActive;
    bool windActive;
} SystemState_t;

void System_Init(void);
void System_Task(void);
void System_ProcessCANMessage(const TWAI_Message_t *message);
void System_SafetyTask(void);
void System_ProcessWiFiTimeout(void);
void System_LoadConfiguration(void);
void System_SaveConfiguration(void);
void System_GetState(SystemState_t *state);
SystemConfig_t *System_GetConfig(void);
void System_SetSwitches(bool rope, bool anemo, bool atp, bool anemoDisplay);
void System_ResetAngles(void);
void System_ResetAll(void);

#endif
