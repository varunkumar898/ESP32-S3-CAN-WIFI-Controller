#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    DISPLAY_NORMAL = 0,
    DISPLAY_ATP_ERROR,
    DISPLAY_ER1,
    DISPLAY_WIND_ALERT,
    DISPLAY_OVL_MAIN,
    DISPLAY_OVL_AUX,
    DISPLAY_OVL_BOTH,
    DISPLAY_ROPE_ERROR,
    DISPLAY_ANEMO_ERROR
} DisplayMode_t;

void Display_Init(void);
void Display_Task(void);
void Display_SetMode(DisplayMode_t mode);
DisplayMode_t Display_GetMode(void);
void Display_SetWeight(float weight);
void Display_SetWind(float wind);
void Display_SetDecimalMode(uint8_t mode);
void Display_SetEnabled(bool enabled);
void Display_SetThreshold(float threshold, uint8_t offDelayMinutes);
void Display_CheckThreshold(void);

#endif
