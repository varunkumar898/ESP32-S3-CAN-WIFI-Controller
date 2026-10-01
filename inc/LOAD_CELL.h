#ifndef LOAD_CELL_H
#define LOAD_CELL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    int32_t count;
    float value;
    bool valid;
    uint8_t reserved[3];
} LoadEntry_t;

void LoadCell_Init(void);
void LoadCell_Task(void);
void LoadCell_ProcessMainRaw(int32_t raw);
void LoadCell_ProcessAuxRaw(int32_t raw);
void LoadCell_ProcessCalibrationRaw(int32_t mainRaw, int32_t auxRaw);
void LoadCell_SetSwap(bool swap);
bool LoadCell_GetSwap(void);
float LoadCell_GetMainWeight(void);
float LoadCell_GetAuxWeight(void);
float LoadCell_GetMainCPT(void);
float LoadCell_GetAuxCPT(void);
int32_t LoadCell_GetMainNoLoad(void);
int32_t LoadCell_GetAuxNoLoad(void);
void LoadCell_SetMainNoLoad(int32_t value);
void LoadCell_SetAuxNoLoad(int32_t value);
void LoadCell_SetMainConfig(bool enable, bool alarm, float capacity, float percent);
void LoadCell_SetAuxConfig(bool enable, bool alarm, float capacity, float percent);
void LoadCell_GetMainConfig(bool *enable, bool *alarm, float *capacity, float *percent);
void LoadCell_GetAuxConfig(bool *enable, bool *alarm, float *capacity, float *percent);
const LoadEntry_t *LoadCell_GetMainLoads(void);
const LoadEntry_t *LoadCell_GetAuxLoads(void);
void LoadCell_Reset(bool aux);

#endif
