# IZAR RC 686 I W R4 MQTT Water Meter (Unit‑C6L) Bridge

Firmware for the M5 Stack Unit‑C6L that receives wM‑Bus frames over SX1262 FSK, decodes IZAR RC 686 I W R4 meter data (T1 mode, 868.95 MHz), and publishes JSON to MQTT with Home Assistant discovery. Includes a web configuration portal, captive AP fallback, OTA updates, and a web log viewer.

## Table of Contents

- [Quick Start](#quick-start)
- [Example Images](#example-images)
- [Features](#features)
- [Hardware](#hardware)
- [Project Structure](#project-structure)
- [Configuration](#configuration)
- [Web Portal](#web-portal)
- [MQTT](#mqtt)
- [Home Assistant](#home-assistant)
- [OTA Updates](#ota-updates)
- [Logging](#logging)
- [Development & Testing](#development--testing)
- [Troubleshooting](#troubleshooting)
- [Resources](#resources)

## Quick Start

1. Build and upload:
   ```bash
   pio run -e m5stack-unit-c6l
   pio run -e m5stack-unit-c6l -t upload
   ```
2. If WiFi is not configured, connect to the AP:
  - SSID: `izar-mqtt-bridge-config-XXXX`
  - Password: `izar-mqtt`
3. The captive portal should auto‑open; if it doesn’t, open `http://192.168.4.1/` and set WiFi + MQTT settings.
4. The device starts in discovery mode and adds found meters to a list. Use the button to navigate the list. Long‑press the currently displayed meter to bind it as the default. You can reset the binding from the web configuration portal. This was tested with an IZAR RC 686 I W R4 meter configured to send frequent data updates in T1 mode at 868.95 MHz.

## Example Images

### Welcome boot screen
<img src="assets/1%20-%20Welcome%20screen.png" alt="Welcome boot screen" width="240">

### Meter Discovery Mode
<img src="assets/2%20-%20Discovery%20mode%20display.png" alt="Meter Discovery Mode" width="240">

### Meter Bound Mode
<img src="assets/3%20-%20Bound%20mode%20display.png" alt="Meter Bound Mode" width="240">

### Home Assistant Sensors
<img src="assets/4%20-%20Home%20Assistant.png" alt="Home Assistant Sensors" width="240">

## Features

- **wM‑Bus T1 FSK reception** using SX1262 (RadioLib)
- **IZAR decoding** with PRIOS handling and alarm flags
- **MQTT publishing** with Home Assistant discovery
- **Web configuration portal** (WiFi, MQTT, serial number) with captive AP
- **OTA firmware update** from the web UI
- **Web log viewer** with streaming logs
- **SSD1306 64×48 display** with meter binding workflow
- **mDNS hostname** and local broker discovery (.local)

## Hardware

- **Board:** M5 Stack Unit‑C6L (ESP32‑C6)
- **Radio:** SX1262 used in FSK mode (wM‑Bus T1 mode, SPI)
- **Display:** SSD1306 64×48 (SPI)
- **GPIO expander:** PI4IOE5V6408 (I2C)

### Pin Mapping (configured in config.h)

**Shared SPI Bus**

| Signal | GPIO | Notes |
|---|---:|---|
| SCK | 20 | Shared (SSD1306 + SX1262) |
| MOSI | 21 | Shared (SSD1306 + SX1262) |
| MISO | 22 | Shared (SSD1306 + SX1262) |

**SX1262**

| Signal | GPIO |
|---|---:|
| CS | 23 |
| IRQ | 7 |
| BUSY | 19 |

**GPIO Expander (I2C)**

| Signal | GPIO | Notes |
|---|---:|---|
| SDA | 10 | I2C data |
| SCL | 8 | I2C clock |
| INT | 3 | Interrupt output |
| Address | 0x43 | Fixed on Unit‑C6L |

Control pins via GPIO expander:

| Expander Pin | Function |
|---|---|
| E0.P5 | Output: SX_LNA_EN |
| E0.P6 | Output: SX_ANT_SW |
| E0.P7 | Output: SX_NRST |
| E0.P0 | Input: Button (Active low) |

**SSD1306 Display (SPI)**

| Signal | GPIO |
|---|---:|
| CS | 6 |
| DC | 18 |
| RST | 15 |

**Other Peripherals**

| Function | GPIO |
|---|---:|
| RGB LED (WS2812C) | 2 |
| Buzzer | 11 |

## Project Structure

```
izar-mqtt/
├── platformio.ini
├── include/
│   ├── config.h
│   ├── config_manager.h
│   ├── display_manager.h
│   ├── fsk_modem_manager.h
│   ├── gpio_expander_manager.h
│   ├── hardware_manager.h
│   ├── izar_handler.h
│   ├── mqtt_manager.h
│   ├── prios_handler.h
│   ├── web_config_server.h
│   ├── web_logger.h
│   ├── wifi_manager.h
│   └── wm_bus_handler.h
└── src/
    ├── main.cpp
    ├── config_manager.cpp
    ├── display_manager.cpp
    ├── fsk_modem_manager.cpp
    ├── gpio_expander_manager.cpp
    ├── hardware_manager.cpp
    ├── izar_handler.cpp
    ├── mqtt_manager.cpp
    ├── prios_handler.cpp
    ├── web_config_server.cpp
    ├── web_logger.cpp
    ├── wifi_manager.cpp
    └── wm_bus_handler.cpp
```

## Configuration

Configuration is stored in flash (Preferences) and managed via the web portal.

Defaults live in `include/config.h` and should not be edited for day‑to‑day config.

## Web Portal

- LAN: `http://<device-ip>/`
- AP fallback: `http://192.168.4.1/`

Note: the portal is unauthenticated on the local network. Restrict access to trusted networks only.

Settings:

- WiFi SSID/password
- Hostname
- MQTT broker/port/credentials
- Base topic
- Optional meter serial number

## MQTT

Base topic defaults to `home/water_meter`.

### Publishing (Device → MQTT)

- `<base>/status` — LWT / availability (`online` / `offline`)
- `<base>/reading` — JSON payload

### Subscribing (MQTT → Device)

- `<base>/cmd` — commands: `beep`, `reset`, `status`

### JSON Payload (reading)

`unit` is reported as `m3` for cubic meters, otherwise `unknown`.

```json
{
  "meter_id": "12345678",
  "current_reading": 12.345,
  "h0_reading": 10.000,
  "h0_date": "2025-12-31",
  "unit": "m3",
  "battery_years": 4.2,
  "radio_interval": 8,
  "meter_rssi": -72,
  "wifi_rssi": -58,
  "flow_rate": 15.25,
  "free_heap_kb": 95.4,
  "alarms": {
    "general": false,
    "leakage_current": false,
    "leakage_previous": false,
    "meter_blocked": false,
    "back_flow": false,
    "underflow": false,
    "overflow": false,
    "submarine": false,
    "sensor_fraud_current": false,
    "sensor_fraud_previous": false,
    "mechanical_fraud_current": false,
    "mechanical_fraud_previous": false
  }
}
```

## Home Assistant

Home Assistant discovery is published automatically on first successful MQTT connection.

### Optional Manual Sensors

```yaml
sensor:
  - platform: mqtt
    name: "Water Meter Reading"
    state_topic: "home/water_meter/reading"
    unit_of_measurement: "m³"
    device_class: "water"
    value_template: "{{ value_json.current_reading | round(3) }}"

  - platform: mqtt
    name: "Water Flow Rate"
    state_topic: "home/water_meter/reading"
    unit_of_measurement: "l/h"
    icon: mdi:water
    value_template: "{{ value_json.flow_rate | round(2) }}"
```

## OTA Updates

Open `http://<device-ip>/update` and upload a new firmware binary.

## Logging

Log levels are defined in `include/config.h`:

- `LOG_LEVEL_ERROR`
- `LOG_LEVEL_WARNING`
- `LOG_LEVEL_INFO`
- `LOG_LEVEL_DEBUG`

Macros:

- `LOG_ERROR`, `LOG_WARN`, `LOG_INFO`, `LOG_DEBUG`, `LOG_ALWAYS`

Logs are written to Serial and the web log viewer at `http://<device-ip>/logs`.

## Development & Testing

```bash
pio run -e m5stack-unit-c6l
pio run -e m5stack-unit-c6l -t upload
pio device monitor -e m5stack-unit-c6l
```

Manual checks:

- WiFi connects or AP starts when unconfigured
- MQTT connects and publishes discovery
- `<base>/reading` publishes on meter updates
- Display shows binding workflow and meter status
- Button long‑press binds to selected meter
- Logs appear on `http://<device-ip>/logs`

## Troubleshooting

- **No WiFi config:** Connect to AP and open `http://192.168.4.1/`
- **No MQTT:** Verify broker hostname/IP and port; check `http://<device-ip>/logs`
- **No meter data:** Ensure the meter is in range and matching serial (if set)

## Resources

- [M5 Stack Official Documentation](https://docs.m5stack.com/)
- [M-bus Protocol Specification](https://www.m-bus.com/)
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/docs/mqtt/discovery/)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [wmbusmeters](https://github.com/wmbusmeters/wmbusmeters)
- [IZAR I R4 PRIOS meter collector
](https://github.com/ZeWaren/izar-prios-smart-meter-collector)
