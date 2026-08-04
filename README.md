# ESP32 Smart Weather Station

A personal embedded/IoT project based on ESP32 + TFT display and a separate ESP8266/NodeMCU temperature node.

## Project status

**v1.0.0-rc1 — Release Candidate**

This repository is prepared for hardware validation. Library and board versions are intentionally marked as `TBD` until they are verified from the development environment.

## Architecture

```text
                 ESP32 + TFT
                      |
          +-----------+-----------+
          |                       |
          v                       v
    Weather API              ESP8266 / NodeMCU
          |                       |
          |                 DS18B20 temperature
          |                       |
          +-----------+-----------+
                      |
                      v
                 TFT display
```

## Features

- ESP32-based TFT user interface
- Weather data retrieval
- NTP-based clock
- Temperature data from a separate ESP8266/NodeMCU node
- DS18B20 temperature sensor
- Wi-Fi networking
- TCP communication between ESP32 and ESP8266
- JSON weather data processing
- TFT_eSPI display support
- Non-blocking/background network architecture in the original project

## Hardware

- ESP32 development board
- TFT display
- ESP8266 / NodeMCU
- DS18B20 temperature sensor
- Wi-Fi network

## Software

- Arduino Framework / Arduino IDE
- C++
- ESP32 Arduino Core
- ESP8266 Arduino Core
- TFT_eSPI
- ArduinoJson
- OneWire
- DallasTemperature

## Configuration

1. Copy `config.example.h` to `config.h` when the project is finalized.
2. Add your local Wi-Fi credentials and API key.
3. Configure the ESP8266/NodeMCU IP address for your local network.
4. Configure TFT_eSPI according to your display controller and wiring.

**Do not commit `config.h` or real credentials to a public repository.**

## Important

The exact library and board versions must be verified before tagging a stable release. See:

- `docs/software-versions.md`
- `docs/architecture.md`
- `docs/wiring.md`

## Release plan

- `v1.0.0-rc1` — repository cleanup and documentation baseline
- `v1.0.0` — after successful compilation and hardware validation
