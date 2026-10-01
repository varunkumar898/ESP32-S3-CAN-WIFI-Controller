#include "SYSTEM.h"
#include "ESP32S3.h"
#include "TIMER.h"
#include "LOAD_CELL.h"
#include "DISPLAY.h"
#include "RELAY.h"
#include "NVS.h"
#include "WIFI.h"
#include <math.h>
#include <string.h>

static SystemConfig_t config;
static SystemState_t state;

static uint32_t lastCANTime = 0;
static uint32_t lastWeightUpdate = 0;
static uint32_t lastWindUpdate = 0;
static uint32_t windZeroTime = 0;
static uint32_t mainAlarmTime = 0;
static uint32_t auxAlarmTime = 0;
static uint32_t ropeAlarmTime = 0;
static uint32_t atpAlarmTime = 0;

static bool previousMainAlarm = false;
static bool previousAuxAlarm = false;
static bool previousRopeAlarm = false;
static bool previousATPAlarm = false;

static void SetDefaultConfiguration(void)
{
    memset(&config, 0, sizeof(config));

    config.variation = 0.0f;
    config.setX = 0.0f;
    config.setY = 0.0f;
    config.offsetX = 0.0f;
    config.offsetY = 0.0f;
    config.windLimit = 20.0f;
    config.alphaValue = 0.30f;
    config.decimalMode = 1;

    config.ropeSwitch = false;
    config.anemoSwitch = false;
    config.atpSwitch = false;
    config.anemoDisplayEnable = false;

    config.mainLoadEnable = true;
    config.mainOverloadEnable = true;
    config.mainCapacity = 0.0f;
    config.mainOverloadPercent = 0.0f;

    config.auxLoadEnable = true;
    config.auxOverloadEnable = true;
    config.auxCapacity = 0.0f;
    config.auxOverloadPercent = 0.0f;

    config.swapSensors = false;
    config.relayConfig[0] = 0;
    config.relayConfig[1] = 0;
    config.relayConfig[2] = 0;

    config.wifiTimeoutMinutes = 2;
    config.displayThreshold = 90.0f;
    config.displayOffDelay = 5;
    config.displayUnit = 0;
}

void System_Init(void)
{
    SetDefaultConfiguration();
    memset(&state, 0, sizeof(state));
    lastCANTime = Timer_Millis();
    lastWeightUpdate = lastCANTime;
    lastWindUpdate = lastCANTime;
    windZeroTime = lastCANTime;
}

void System_LoadConfiguration(void)
{
    SystemConfig_t saved;

    if (!NVS_LoadConfiguration(&saved))
        return;

    config = saved;

    LoadCell_SetSwap(config.swapSensors);
    LoadCell_SetMainConfig(config.mainLoadEnable,
                           config.mainOverloadEnable,
                           config.mainCapacity,
                           config.mainOverloadPercent);
    LoadCell_SetAuxConfig(config.auxLoadEnable,
                          config.auxOverloadEnable,
                          config.auxCapacity,
                          config.auxOverloadPercent);

    Display_SetDecimalMode(config.decimalMode);
    Display_SetThreshold(config.displayThreshold, config.displayOffDelay);
    WiFi_SetTimeoutMinutes(config.wifiTimeoutMinutes);

    for (uint8_t i = 0; i < 3; i++)
        Relay_SetConfig(i, config.relayConfig[i]);
}

void System_SaveConfiguration(void)
{
    config.swapSensors = LoadCell_GetSwap();
    NVS_SaveConfiguration(&config);
}

SystemConfig_t *System_GetConfig(void)
{
    return &config;
}

void System_GetState(SystemState_t *output)
{
    if (output)
        *output = state;
}

void System_SetSwitches(bool rope, bool anemo, bool atp, bool anemoDisplay)
{
    config.ropeSwitch = rope;
    config.anemoSwitch = anemo;
    config.atpSwitch = atp;
    config.anemoDisplayEnable = anemoDisplay;
}

void System_ResetAngles(void)
{
    config.offsetX = state.pitchX;
    config.offsetY = state.pitchY;
    System_SaveConfiguration();
}

void System_ResetAll(void)
{
    NVS_Reset();
    SetDefaultConfiguration();
    LoadCell_Reset(false);
    LoadCell_Reset(true);
    System_SaveConfiguration();
}

void System_ProcessCANMessage(const TWAI_Message_t *message)
{
    if (!message)
        return;

    if (message->id == APP_CAN_ID_MAIN_LOAD && message->dlc == 4)
    {
        int32_t value;
        memcpy(&value, message->data, sizeof(value));

        if (!config.swapSensors)
            LoadCell_ProcessMainRaw(value);
        else
            LoadCell_ProcessAuxRaw(value);

        lastCANTime = Timer_Millis();
        state.canValid = true;
        state.canPhysicalError = false;
        return;
    }

    if (message->id == APP_CAN_ID_ANGLES && message->dlc == 8)
    {
        memcpy(&state.pitchX, &message->data[0], sizeof(float));
        memcpy(&state.pitchY, &message->data[4], sizeof(float));

        lastCANTime = Timer_Millis();
        state.canValid = true;
        state.canPhysicalError = false;
        return;
    }

    if (message->id == APP_CAN_ID_AUX_LOAD && message->dlc == 4)
    {
        int32_t value;
        memcpy(&value, message->data, sizeof(value));

        if (!config.swapSensors)
            LoadCell_ProcessAuxRaw(value);
        else
            LoadCell_ProcessMainRaw(value);

        lastCANTime = Timer_Millis();
        state.canValid = true;
        state.canPhysicalError = false;
        return;
    }

    /* Optional CSV-style CAN payload. */
    char buffer[9] = {0};
    uint8_t length = message->dlc > 8 ? 8 : message->dlc;
    memcpy(buffer, message->data, length);

    char *save = NULL;
    char *token = strtok_r(buffer, ",", &save);
    if (!token) return;

    int32_t first = (int32_t)strtol(token, NULL, 10);
    token = strtok_r(NULL, ",", &save);
    if (!token) return;

    int32_t second = (int32_t)strtol(token, NULL, 10);

    if (!config.swapSensors)
    {
        LoadCell_ProcessMainRaw(first);
        LoadCell_ProcessAuxRaw(second);
    }
    else
    {
        LoadCell_ProcessMainRaw(second);
        LoadCell_ProcessAuxRaw(first);
    }

    token = strtok_r(NULL, ",", &save);
    if (token) state.pitchX = strtof(token, NULL);
    token = strtok_r(NULL, ",", &save);
    if (token) state.pitchY = strtof(token, NULL);
    token = strtok_r(NULL, ",", &save);
    if (token) state.dacX = strtof(token, NULL);
    token = strtok_r(NULL, ",", &save);
    if (token) state.dacY = strtof(token, NULL);
    token = strtok_r(NULL, ",", &save);
    if (token) state.windSpeed = strtof(token, NULL);

    lastCANTime = Timer_Millis();
    state.canValid = true;
    state.canPhysicalError = false;
}

static void UpdateWeights(void)
{
    LoadCell_Task();

    float mainWeight = LoadCell_GetMainWeight();
    float auxWeight = LoadCell_GetAuxWeight();

    if (config.mainLoadEnable && config.auxLoadEnable)
        state.displayWeight = mainWeight + auxWeight;
    else if (config.mainLoadEnable)
        state.displayWeight = mainWeight;
    else if (config.auxLoadEnable)
        state.displayWeight = auxWeight;
    else
        state.displayWeight = 0.0f;

    Display_SetWeight(state.displayWeight);
}

static void UpdateWind(void)
{
    uint32_t now = Timer_Millis();

    if (now - lastWindUpdate < APP_WIND_UPDATE_INTERVAL_MS)
        return;

    lastWindUpdate = now;
    state.windFiltered = state.windFiltered * 0.7f + state.windSpeed * 0.3f;
    Display_SetWind(state.windFiltered);
}

static void CheckOverload(void)
{
    static uint32_t mainStart = 0;
    static uint32_t auxStart = 0;
    uint32_t now = Timer_Millis();

    bool mainActive = false;
    bool auxActive = false;

    if (config.mainLoadEnable && config.mainOverloadEnable &&
        config.mainCapacity > 0.0f && config.mainOverloadPercent > 0.0f)
    {
        float limit = config.mainCapacity * config.mainOverloadPercent / 100.0f;
        if (LoadCell_GetMainWeight() >= limit)
        {
            if (mainStart == 0) mainStart = now;
            if (now - mainStart >= 1500U) mainActive = true;
        }
        else mainStart = 0;
    }
    else mainStart = 0;

    if (config.auxLoadEnable && config.auxOverloadEnable &&
        config.auxCapacity > 0.0f && config.auxOverloadPercent > 0.0f)
    {
        float limit = config.auxCapacity * config.auxOverloadPercent / 100.0f;
        if (LoadCell_GetAuxWeight() >= limit)
        {
            if (auxStart == 0) auxStart = now;
            if (now - auxStart >= 1500U) auxActive = true;
        }
        else auxStart = 0;
    }
    else auxStart = 0;

    state.mainOverloadActive = mainActive;
    state.auxOverloadActive = auxActive;

    if (mainActive && !previousMainAlarm) mainAlarmTime = now;
    if (auxActive && !previousAuxAlarm) auxAlarmTime = now;

    previousMainAlarm = mainActive;
    previousAuxAlarm = auxActive;
}

static void CheckRope(void)
{
    static uint32_t start = 0;
    uint32_t now = Timer_Millis();

    if (!config.ropeSwitch)
    {
        state.ropeActive = false;
        start = 0;
        return;
    }

    float mainDiff = fabsf((float)LoadCell_GetMainNoLoad() -
                           (float)LoadCell_GetMainNoLoad());
    float auxDiff = fabsf((float)LoadCell_GetAuxNoLoad() -
                          (float)LoadCell_GetAuxNoLoad());
    (void)mainDiff;
    (void)auxDiff;

    /* Rope/slack evaluation is performed against the live filtered load. */
    bool mainSlack = config.mainLoadEnable &&
                     (LoadCell_GetMainWeight() < config.variation);
    bool auxSlack = config.auxLoadEnable &&
                    (LoadCell_GetAuxWeight() < config.variation);

    if (mainSlack || auxSlack)
    {
        if (start == 0) start = now;
        if (now - start >= 1500U) state.ropeActive = true;
    }
    else
    {
        start = 0;
        state.ropeActive = false;
    }

    if (state.ropeActive && !previousRopeAlarm)
        ropeAlarmTime = now;

    previousRopeAlarm = state.ropeActive;
}

static void CheckATP(void)
{
    static uint32_t start = 0;
    uint32_t now = Timer_Millis();

    if (!config.atpSwitch)
    {
        state.atpActive = false;
        start = 0;
        return;
    }

    float x = state.pitchX - config.offsetX;
    float y = state.pitchY - config.offsetY;
    bool condition = fabsf(x) >= fabsf(config.setX) ||
                     fabsf(y) >= fabsf(config.setY);

    if (condition)
    {
        if (start == 0) start = now;
        if (now - start >= 1500U) state.atpActive = true;
    }
    else
    {
        start = 0;
        state.atpActive = false;
    }

    if (state.atpActive && !previousATPAlarm)
        atpAlarmTime = now;

    previousATPAlarm = state.atpActive;
}

static void CheckAnemometer(void)
{
    static uint32_t alarmStart = 0;
    uint32_t now = Timer_Millis();

    if (!config.anemoSwitch)
    {
        state.anemoError = false;
        state.windActive = false;
        windZeroTime = now;
        alarmStart = 0;
        return;
    }

    if (state.windSpeed > 0.0f)
    {
        state.anemoError = false;
        windZeroTime = now;
    }
    else if (now - windZeroTime >= 3000U)
    {
        state.anemoError = true;
    }

    if (!state.anemoError && state.windSpeed >= config.windLimit)
    {
        if (alarmStart == 0) alarmStart = now;
        if (now - alarmStart >= 1500U) state.windActive = true;
    }
    else
    {
        alarmStart = 0;
        state.windActive = false;
    }
}

void System_SafetyTask(void)
{
    uint32_t now = Timer_Millis();

    state.canPhysicalError = (now - lastCANTime >= APP_CAN_PHYSICAL_TIMEOUT_MS);
    state.canValid = !state.canPhysicalError;

    CheckOverload();
    CheckRope();
    CheckATP();
    CheckAnemometer();

    Relay_UpdateSafety(state.mainOverloadActive,
                       state.auxOverloadActive,
                       state.ropeActive,
                       state.atpActive,
                       state.windActive);

    if (state.canPhysicalError)
    {
        Display_SetMode(DISPLAY_ER1);
    }
    else if (state.anemoError)
    {
        Display_SetMode(DISPLAY_ANEMO_ERROR);
    }
    else if (state.windActive)
    {
        Display_SetMode(DISPLAY_WIND_ALERT);
    }
    else
    {
        uint32_t newest = 0;
        DisplayMode_t mode = DISPLAY_NORMAL;

        if (state.mainOverloadActive && mainAlarmTime > newest)
        {
            newest = mainAlarmTime;
            mode = DISPLAY_OVL_MAIN;
        }

        if (state.auxOverloadActive && auxAlarmTime > newest)
        {
            newest = auxAlarmTime;
            mode = DISPLAY_OVL_AUX;
        }

        if (state.mainOverloadActive && state.auxOverloadActive)
        {
            uint32_t latest = mainAlarmTime > auxAlarmTime ? mainAlarmTime : auxAlarmTime;
            if (latest > newest)
            {
                newest = latest;
                mode = DISPLAY_OVL_BOTH;
            }
        }

        if (state.ropeActive && ropeAlarmTime > newest)
        {
            newest = ropeAlarmTime;
            mode = DISPLAY_ROPE_ERROR;
        }

        if (state.atpActive && atpAlarmTime > newest)
        {
            newest = atpAlarmTime;
            mode = DISPLAY_ATP_ERROR;
        }

        Display_SetMode(mode);
    }

    Display_SetEnabled(true);
}

void System_Task(void)
{
    uint32_t now = Timer_Millis();

    if (state.canValid && now - lastWeightUpdate >= APP_CAN_UPDATE_INTERVAL_MS)
    {
        lastWeightUpdate = now;
        UpdateWeights();
        UpdateWind();
    }

    Display_SetThreshold(config.displayThreshold, config.displayOffDelay);
    Display_CheckThreshold();
}

void System_ProcessWiFiTimeout(void)
{
    WiFi_Task();
}
