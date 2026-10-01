#include "DISPLAY.h"
#include "ESP32S3.h"
#include "GPIO.h"
#include "TIMER.h"
#include "SYSTEM.h"
#include <stdio.h>
#include <math.h>

static const gpio_num_t segmentPins[7] = {
    APP_SEGMENT_A_PIN,
    APP_SEGMENT_B_PIN,
    APP_SEGMENT_C_PIN,
    APP_SEGMENT_D_PIN,
    APP_SEGMENT_E_PIN,
    APP_SEGMENT_F_PIN,
    APP_SEGMENT_G_PIN
};

static const gpio_num_t digitPins[4] = {
    APP_DIGIT1_PIN,
    APP_DIGIT2_PIN,
    APP_DIGIT3_PIN,
    APP_DIGIT4_PIN
};

static const uint8_t digitMap[10] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111
};

static const uint8_t letterE = 0b01111001;
static const uint8_t letterR = 0b01010000;
static const uint8_t letterBlank = 0b00000000;
static const uint8_t letterO = 0b00111111;
static const uint8_t letterV = 0b00111110;
static const uint8_t letterL = 0b00111000;
static const uint8_t letterA = 0b01110111;
static const uint8_t letterT = 0b01111000;
static const uint8_t letterP = 0b01110011;
static const uint8_t letterD = 0b01011110;
static const uint8_t letterI = 0b00000110;
static const uint8_t letterN = 0b01010100;
static const uint8_t letterM = 0b01010101;
static const uint8_t letterW = 0b01111110;
static const uint8_t letterC = 0b00111001;
static const uint8_t letterK = 0b01110110;

static uint8_t currentDigit = 0;
static DisplayMode_t displayMode = DISPLAY_NORMAL;
static float displayWeight = 0.0f;
static float displayWind = 0.0f;
static uint8_t decimalMode = 1;
static bool displayEnabled = true;
static float threshold = 90.0f;
static uint8_t offDelayMinutes = 5;
static uint32_t belowSince = 0;

static void SetSegments(uint8_t pattern)
{
    for (uint8_t i = 0; i < 7; i++)
        GPIO_Write(segmentPins[i], (pattern >> i) & 1U);

    GPIO_SetLow(APP_DISPLAY_DP_PIN);
}

static void DisableDigits(void)
{
    for (uint8_t i = 0; i < 4; i++)
        GPIO_SetLow(digitPins[i]);
}

static void ShowError(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3)
{
    const uint8_t pattern[4] = {p0, p1, p2, p3};
    SetSegments(pattern[currentDigit]);
    GPIO_SetLow(APP_DISPLAY_DP_PIN);
}

static void ShowWeight(void)
{
    int value;
    char buffer[5];

    if (decimalMode == 1)
        value = (int)(displayWeight * 10.0f + 0.5f);
    else
        value = (int)(displayWeight * 100.0f + 0.5f);

    if (value < 0)
        value = 0;
    if (value > 9999)
        value = 9999;

    snprintf(buffer, sizeof(buffer), "%04d", value);

    bool blank = false;

    if (decimalMode == 1)
    {
        if ((currentDigit == 0 || currentDigit == 1) && buffer[currentDigit] == '0')
            blank = true;
    }
    else
    {
        if (currentDigit == 0 && buffer[currentDigit] == '0')
            blank = true;
    }

    if (decimalMode == 1 && currentDigit == 3)
        blank = false;

    if (decimalMode == 2 &&
        (currentDigit == 1 || currentDigit == 2 || currentDigit == 3))
        blank = false;

    SetSegments(blank ? 0 : digitMap[buffer[currentDigit] - '0']);

    if ((decimalMode == 1 && currentDigit == 2) ||
        (decimalMode == 2 && currentDigit == 1))
        GPIO_SetHigh(APP_DISPLAY_DP_PIN);
    else
        GPIO_SetLow(APP_DISPLAY_DP_PIN);
}

static void ShowWind(void)
{
    float wind = displayWind < 0.0f ? 0.0f : displayWind;
    int value = (int)(wind * 10.0f + 0.5f);
    char buffer[5];

    if (value > 9999)
        value = 9999;

    snprintf(buffer, sizeof(buffer), "%04d", value);

    bool blank = (currentDigit < 2 && buffer[currentDigit] == '0');
    if (currentDigit == 3)
        blank = false;

    SetSegments(blank ? 0 : digitMap[buffer[currentDigit] - '0']);

    if (currentDigit == 2)
        GPIO_SetHigh(APP_DISPLAY_DP_PIN);
    else
        GPIO_SetLow(APP_DISPLAY_DP_PIN);
}

static void ShowMode(void)
{
    switch (displayMode)
    {
        case DISPLAY_ER1:
            ShowError(letterE, letterR, 0, 0);
            break;

        case DISPLAY_ATP_ERROR:
            ShowError(letterA, letterT, letterP, letterBlank);
            break;

        case DISPLAY_WIND_ALERT:
            ShowError(letterW /* replaced below */, letterI /* replaced below */, letterN, letterD);
            break;

        case DISPLAY_OVL_MAIN:
            ShowError(letterO, letterV, letterL, 0);
            break;

        case DISPLAY_OVL_AUX:
            ShowError(letterO, letterV, letterL, letterA);
            break;

        case DISPLAY_OVL_BOTH:
            ShowError(letterO, letterV, letterL, letterD);
            break;

        case DISPLAY_ROPE_ERROR:
            ShowError(letterR, letterO, letterP, letterE);
            break;

        case DISPLAY_ANEMO_ERROR:
            ShowError(letterA, letterN, letterE, letterM);
            break;

        default:
            ShowWeight();
            break;
    }
}

void Display_Init(void)
{
    GPIO_OutputInit(APP_DISPLAY_DP_PIN);

    for (uint8_t i = 0; i < 7; i++)
        GPIO_OutputInit(segmentPins[i]);

    for (uint8_t i = 0; i < 4; i++)
    {
        GPIO_OutputInit(digitPins[i]);
        GPIO_SetLow(digitPins[i]);
    }

    SetSegments(0);
}

void Display_Task(void)
{
    DisableDigits();
    SetSegments(0);
    Timer_DelayUs(5);

    if (!displayEnabled)
        return;

    if (displayMode == DISPLAY_NORMAL)
    {
        SystemConfig_t *config = System_GetConfig();
        if (config->anemoDisplayEnable)
            ShowWind();
        else
            ShowWeight();
    }
    else
    {
        ShowMode();
    }

    GPIO_SetHigh(digitPins[currentDigit]);
    currentDigit = (uint8_t)((currentDigit + 1U) % 4U);
}

void Display_SetMode(DisplayMode_t mode)
{
    displayMode = mode;
}

DisplayMode_t Display_GetMode(void)
{
    return displayMode;
}

void Display_SetWeight(float weight)
{
    displayWeight = weight;
}

void Display_SetWind(float wind)
{
    displayWind = wind;
}

void Display_SetDecimalMode(uint8_t mode)
{
    decimalMode = (mode == 2) ? 2 : 1;
}

void Display_SetEnabled(bool enabled)
{
    displayEnabled = enabled;
}

void Display_SetThreshold(float value, uint8_t delayMinutes)
{
    threshold = value;
    offDelayMinutes = delayMinutes;
}

void Display_CheckThreshold(void)
{
    if (threshold <= 0.0f)
    {
        displayEnabled = true;
        belowSince = 0;
        return;
    }

    if (displayWeight >= threshold)
    {
        displayEnabled = true;
        belowSince = 0;
        return;
    }

    if (belowSince == 0)
        belowSince = Timer_Millis();

    if (Timer_Millis() - belowSince >= (uint32_t)offDelayMinutes * 60000U)
        displayEnabled = false;
}
