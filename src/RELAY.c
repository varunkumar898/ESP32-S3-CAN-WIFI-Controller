#include "RELAY.h"
#include "GPIO.h"
#include "ESP32S3.h"

static const gpio_num_t relayPins[3] = {
    APP_RELAY1_PIN,
    APP_RELAY2_PIN,
    APP_RELAY3_PIN
};

static bool relayState[3] = {false, false, false};
static uint8_t relayConfig[3] = {0, 0, 0};

void Relay_Init(void)
{
    for (int i = 0; i < 3; i++)
    {
        GPIO_OutputInit(relayPins[i]);
        GPIO_SetHigh(relayPins[i]);
        relayState[i] = false;
    }
}

void Relay_Set(Relay_t relay, bool alarmActive)
{
    if (relay > RELAY_3)
        return;

    relayState[relay] = alarmActive;

    /* Active LOW: LOW = relay ON, HIGH = relay OFF. */
    GPIO_Write(relayPins[relay], alarmActive ? 0 : 1);
}

bool Relay_Get(Relay_t relay)
{
    if (relay > RELAY_3)
        return false;

    return relayState[relay];
}

void Relay_SetConfig(uint8_t relay, uint8_t mask)
{
    if (relay < 3)
        relayConfig[relay] = mask;
}

uint8_t Relay_GetConfig(uint8_t relay)
{
    return relay < 3 ? relayConfig[relay] : 0;
}

void Relay_UpdateSafety(bool mainOverload, bool auxOverload, bool rope,
                        bool atp, bool anemo)
{
    bool condition[5] = {
        mainOverload,
        auxOverload,
        rope,
        atp,
        anemo
    };

    for (uint8_t relay = 0; relay < 3; relay++)
    {
        bool active = false;

        for (uint8_t parameter = 0; parameter < 5; parameter++)
        {
            if (condition[parameter] &&
                (relayConfig[relay] & (1U << parameter)))
            {
                active = true;
            }
        }

        Relay_Set((Relay_t)relay, active);
    }
}

void Relay_Task(void)
{
}
