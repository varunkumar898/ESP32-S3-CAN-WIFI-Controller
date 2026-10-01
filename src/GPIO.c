#include "GPIO.h"
#include "ESP32S3.h"

void GPIO_OutputInit(gpio_num_t pin)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config);
}

void GPIO_Init(void)
{
    GPIO_OutputInit(APP_RELAY1_PIN);
    GPIO_OutputInit(APP_RELAY2_PIN);
    GPIO_OutputInit(APP_RELAY3_PIN);
}

void GPIO_Write(gpio_num_t pin, uint8_t state)
{
    gpio_set_level(pin, state ? 1 : 0);
}

uint8_t GPIO_Read(gpio_num_t pin)
{
    return (uint8_t)gpio_get_level(pin);
}

void GPIO_SetHigh(gpio_num_t pin)
{
    gpio_set_level(pin, 1);
}

void GPIO_SetLow(gpio_num_t pin)
{
    gpio_set_level(pin, 0);
}
