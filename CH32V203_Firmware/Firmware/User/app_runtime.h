#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdint.h>

void AppRuntime_Init(void);
uint32_t millis(void);
void App_DelayMs(uint32_t delayMs);

#endif
