#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "driver/gpio.h"

void GPIO_Init(void);
void GPIO_OutputInit(gpio_num_t pin);
void GPIO_Write(gpio_num_t pin, uint8_t state);
uint8_t GPIO_Read(gpio_num_t pin);
void GPIO_SetHigh(gpio_num_t pin);
void GPIO_SetLow(gpio_num_t pin);

#endif
