#ifndef TWAI_H
#define TWAI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool extended;
} TWAI_Message_t;

void TWAI_Init(void);
bool TWAI_Send(const TWAI_Message_t *message);
bool TWAI_Receive(TWAI_Message_t *message);
bool TWAI_IsReady(void);
void TWAI_Task(void);

#endif
