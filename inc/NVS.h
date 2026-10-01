#ifndef NVS_H
#define NVS_H

#include <stdint.h>
#include <stdbool.h>
#include "SYSTEM.h"

void NVS_Init(void);
bool NVS_LoadConfiguration(SystemConfig_t *config);
bool NVS_SaveConfiguration(const SystemConfig_t *config);
bool NVS_LoadCalibration(SystemState_t *state);
bool NVS_SaveCalibration(const SystemState_t *state);
void NVS_Reset(void);

#endif
