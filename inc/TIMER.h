#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void Timer_Init(void);
uint32_t Timer_Millis(void);
uint32_t Timer_Micros(void);
void Timer_DelayMs(uint32_t milliseconds);
void Timer_DelayUs(uint32_t microseconds);
void Timer_Task(void);

#endif
