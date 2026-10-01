#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>

void WebServer_Init(void);
void WebServer_Task(void);
void WebServer_Stop(void);
bool WebServer_IsRunning(void);

#endif
