# ESP32 Smart Weather Station

A personal embedded/IoT project based on ESP32 + TFT display and a separate ESP8266/NodeMCU temperature node.

## Features

- Real-time clock using NTP
- Weather data from external API
- Temperature measurement using DS18B20
- ESP32 ↔ ESP8266 communication
- TFT graphical user interface
- Wi-Fi connectivity
- TCP communication
- JSON data processing
- Non-blocking network operations
- Automatic display updates
  
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


## Hardware

- ESP32 C3 ARD-ESP32-C3-TFT24
- ESP8266 / NodeMCU
- DS18B20 temperature sensor
- Wi-Fi network

## Software Environment

- Arduino IDE 2.3.10
- ESP32 Arduino Core 2.0.14
- TFT_eSPI 2.5.43
- ArduinoJson 7.4.3
- OneWire 2.3.8
- DallasTemperature 4.0.6

## Installation

1. Install Arduino IDE 2.3.10.
2. Install ESP32 Arduino Core 2.0.14.
3. Install required libraries.
4. Configure TFT_eSPI.
5. Create `config.h`.
6. Configure Wi-Fi.
7. Configure weather API.
8. Configure ESP8266 IP address.
9. Upload firmware to ESP8266.
10. Upload firmware to ESP32.



## Release plan

- `v1.0.0-rc1` — repository cleanup and documentation baseline
- `v1.0.0` — after successful compilation and hardware validation
