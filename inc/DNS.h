#ifndef DNS_H
#define DNS_H

#include <stdbool.h>

void DNS_Init(void);
void DNS_Start(void);
void DNS_Stop(void);
void DNS_Task(void);
bool DNS_IsRunning(void);

#endif
