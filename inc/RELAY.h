#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    RELAY_1 = 0,
    RELAY_2,
    RELAY_3
} Relay_t;

void Relay_Init(void);
void Relay_Set(Relay_t relay, bool alarmActive);
bool Relay_Get(Relay_t relay);
void Relay_SetConfig(uint8_t relay, uint8_t mask);
uint8_t Relay_GetConfig(uint8_t relay);
void Relay_UpdateSafety(bool mainOverload, bool auxOverload, bool rope,
                        bool atp, bool anemo);
void Relay_Task(void);

#endif
