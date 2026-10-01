# ESP32-S3 CAN Wi-Fi Controller

Modular ESP-IDF C project for an ESP32-S3 controller.

## Structure

```text
ESP32-S3-CAN-WIFI-Controller/
├── docs/
├── inc/
├── src/
├── web/
├── main/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── .gitignore
```

## Main functions

- ESP32-S3 TWAI/CAN at 250 kbit/s
- Dual load processing
- 7-segment multiplexed display
- Three active-low relay outputs
- Rope/slack monitoring
- Anti-tilt monitoring
- Wind monitoring
- Wi-Fi access point
- Captive DNS redirection
- HTTP dashboard and settings pages
- Non-volatile configuration storage

## Build

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## CAN IDs

| ID | Data |
|---:|---|
| `0x100` | Main load, signed 32-bit value |
| `0x101` | X/Y angles, two 32-bit floats |
| `0x102` | Auxiliary load, signed 32-bit value |

## Pins

CAN TX: GPIO16

CAN RX: GPIO17

Display segments A-G: GPIO18, GPIO1, GPIO21, GPIO7, GPIO8, GPIO2, GPIO3

Display decimal point: GPIO15

Display digits: GPIO10, GPIO11, GPIO12, GPIO14

Relay 1/2/3: GPIO6, GPIO4, GPIO5

Relay outputs are active LOW.

## Wi-Fi

The default access-point configuration is defined in `inc/ESP32S3.h`.

The access-point address is `192.168.4.1`.

## Architecture

The project uses ESP-IDF services for Wi-Fi, HTTP, NVS and TWAI. The application itself is written in C and separated into drivers instead of Arduino `setup()` and `loop()`.
