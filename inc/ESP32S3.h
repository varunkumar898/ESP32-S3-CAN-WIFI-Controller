#ifndef ESP32S3_H
#define ESP32S3_H

#include <stdint.h>
#include "driver/gpio.h"

#define APP_CAN_TX_PIN          GPIO_NUM_16
#define APP_CAN_RX_PIN          GPIO_NUM_17

#define APP_RELAY1_PIN          GPIO_NUM_6
#define APP_RELAY2_PIN          GPIO_NUM_4
#define APP_RELAY3_PIN          GPIO_NUM_5

#define APP_DISPLAY_DP_PIN      GPIO_NUM_15
#define APP_SEGMENT_A_PIN       GPIO_NUM_18
#define APP_SEGMENT_B_PIN       GPIO_NUM_1
#define APP_SEGMENT_C_PIN       GPIO_NUM_21
#define APP_SEGMENT_D_PIN       GPIO_NUM_7
#define APP_SEGMENT_E_PIN       GPIO_NUM_8
#define APP_SEGMENT_F_PIN       GPIO_NUM_2
#define APP_SEGMENT_G_PIN       GPIO_NUM_3

#define APP_DIGIT1_PIN          GPIO_NUM_10
#define APP_DIGIT2_PIN          GPIO_NUM_11
#define APP_DIGIT3_PIN          GPIO_NUM_12
#define APP_DIGIT4_PIN          GPIO_NUM_14

#define APP_CAN_ID_MAIN_LOAD    0x100U
#define APP_CAN_ID_ANGLES       0x101U
#define APP_CAN_ID_AUX_LOAD     0x102U
#define APP_CAN_BITRATE         250000U

#define APP_AP_IP               "192.168.4.1"
#define APP_AP_NETMASK          "255.255.255.0"
#define APP_AP_CHANNEL          11
#define APP_HTTP_PORT           80
#define APP_DNS_PORT            53

#define APP_DEFAULT_WIFI_SSID   "ESP32S3_Controller"
#define APP_DEFAULT_WIFI_PASS   "1234567890"

#define APP_ALPHA_PASSCODE      "9361"

#define APP_CAN_PHYSICAL_TIMEOUT_MS 3000U
#define APP_CAN_UPDATE_INTERVAL_MS  500U
#define APP_WIND_UPDATE_INTERVAL_MS 250U

#endif
