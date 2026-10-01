#include "LOAD_CELL.h"
#include "SYSTEM.h"
#include "TIMER.h"
#include <math.h>
#include <string.h>

#define HX_ERR (-999999L)

static int32_t rawMain = 0;
static int32_t rawAux = 0;
static float filteredMain = 0.0f;
static float filteredAux = 0.0f;
static bool firstMain = true;
static bool firstAux = true;
static bool mainHXErr = false;
static bool auxHXErr = false;
static bool swapSensors = false;

static LoadEntry_t mainLoads[4] = {0};
static LoadEntry_t auxLoads[4] = {0};
static int32_t mainNoLoad = 0;
static int32_t auxNoLoad = 0;
static float mainCPT = 0.0f;
static float auxCPT = 0.0f;
static float mainWeight = 0.0f;
static float auxWeight = 0.0f;
static bool mainLoadEnable = true;
static bool mainOverloadEnable = true;
static float mainCapacity = 0.0f;
static float mainOverloadPercent = 0.0f;
static bool auxLoadEnable = true;
static bool auxOverloadEnable = true;
static float auxCapacity = 0.0f;
static float auxOverloadPercent = 0.0f;
static int mainDirection = 1;
static int auxDirection = 1;

static float CalculateCPT(const LoadEntry_t *loads)
{
    float total = 0.0f;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        if (loads[i].valid && loads[i].value > 0.0f)
        {
            total += fabsf((float)loads[i].count) / loads[i].value;
            valid++;
        }
    }

    return valid ? total / valid : 0.0f;
}

void LoadCell_Init(void)
{
    mainCPT = CalculateCPT(mainLoads);
    auxCPT = CalculateCPT(auxLoads);
}

void LoadCell_ProcessMainRaw(int32_t raw)
{
    rawMain = raw;
    mainHXErr = (raw == HX_ERR);

    if (mainHXErr)
        return;

    if (firstMain)
    {
        filteredMain = (float)raw;
        firstMain = false;
    }
    else
    {
        SystemConfig_t *config = System_GetConfig();
        float alpha = config->alphaValue;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        filteredMain = (1.0f - alpha) * filteredMain + alpha * raw;
    }
}

void LoadCell_ProcessAuxRaw(int32_t raw)
{
    rawAux = raw;
    auxHXErr = (raw == HX_ERR);

    if (auxHXErr)
        return;

    if (firstAux)
    {
        filteredAux = (float)raw;
        firstAux = false;
    }
    else
    {
        SystemConfig_t *config = System_GetConfig();
        float alpha = config->alphaValue;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        filteredAux = (1.0f - alpha) * filteredAux + alpha * raw;
    }
}

void LoadCell_ProcessCalibrationRaw(int32_t mainRawValue, int32_t auxRawValue)
{
    LoadCell_ProcessMainRaw(mainRawValue);
    LoadCell_ProcessAuxRaw(auxRawValue);
}

void LoadCell_SetSwap(bool swap)
{
    swapSensors = swap;
    firstMain = true;
    firstAux = true;
}

bool LoadCell_GetSwap(void)
{
    return swapSensors;
}

void LoadCell_Task(void)
{
    if (mainLoadEnable && !mainHXErr && mainCPT > 0.0f)
    {
        float diff = filteredMain - mainNoLoad;
        if ((mainDirection == 1 && diff > 0.0f) ||
            (mainDirection == -1 && diff < 0.0f))
            mainWeight = fabsf(diff) / mainCPT;
        else
            mainWeight = 0.0f;
    }
    else
    {
        mainWeight = 0.0f;
    }

    if (auxLoadEnable && !auxHXErr && auxCPT > 0.0f)
    {
        float diff = filteredAux - auxNoLoad;
        if ((auxDirection == 1 && diff > 0.0f) ||
            (auxDirection == -1 && diff < 0.0f))
            auxWeight = fabsf(diff) / auxCPT;
        else
            auxWeight = 0.0f;
    }
    else
    {
        auxWeight = 0.0f;
    }
}

float LoadCell_GetMainWeight(void) { return mainWeight; }
float LoadCell_GetAuxWeight(void) { return auxWeight; }
float LoadCell_GetMainCPT(void) { return mainCPT; }
float LoadCell_GetAuxCPT(void) { return auxCPT; }
int32_t LoadCell_GetMainNoLoad(void) { return mainNoLoad; }
int32_t LoadCell_GetAuxNoLoad(void) { return auxNoLoad; }

void LoadCell_SetMainNoLoad(int32_t value)
{
    mainNoLoad = value;
}

void LoadCell_SetAuxNoLoad(int32_t value)
{
    auxNoLoad = value;
}

void LoadCell_SetMainConfig(bool enable, bool alarm, float capacity, float percent)
{
    mainLoadEnable = enable;
    mainOverloadEnable = alarm;
    mainCapacity = capacity;
    mainOverloadPercent = percent;
}

void LoadCell_SetAuxConfig(bool enable, bool alarm, float capacity, float percent)
{
    auxLoadEnable = enable;
    auxOverloadEnable = alarm;
    auxCapacity = capacity;
    auxOverloadPercent = percent;
}

void LoadCell_GetMainConfig(bool *enable, bool *alarm, float *capacity, float *percent)
{
    if (enable) *enable = mainLoadEnable;
    if (alarm) *alarm = mainOverloadEnable;
    if (capacity) *capacity = mainCapacity;
    if (percent) *percent = mainOverloadPercent;
}

void LoadCell_GetAuxConfig(bool *enable, bool *alarm, float *capacity, float *percent)
{
    if (enable) *enable = auxLoadEnable;
    if (alarm) *alarm = auxOverloadEnable;
    if (capacity) *capacity = auxCapacity;
    if (percent) *percent = auxOverloadPercent;
}

const LoadEntry_t *LoadCell_GetMainLoads(void) { return mainLoads; }
const LoadEntry_t *LoadCell_GetAuxLoads(void) { return auxLoads; }

void LoadCell_Reset(bool aux)
{
    if (aux)
    {
        auxNoLoad = 0;
        auxCPT = 0.0f;
        auxWeight = 0.0f;
        memset(auxLoads, 0, sizeof(auxLoads));
    }
    else
    {
        mainNoLoad = 0;
        mainCPT = 0.0f;
        mainWeight = 0.0f;
        memset(mainLoads, 0, sizeof(mainLoads));
    }
}
