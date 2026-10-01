#include "TIMER.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void Timer_Init(void)
{
}

uint32_t Timer_Millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

uint32_t Timer_Micros(void)
{
    return (uint32_t)esp_timer_get_time();
}

void Timer_DelayMs(uint32_t milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

void Timer_DelayUs(uint32_t microseconds)
{
    esp_rom_delay_us(microseconds);
}

void Timer_Task(void)
{
}
