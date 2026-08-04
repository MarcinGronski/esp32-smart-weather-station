# Architecture

The project consists of two microcontroller nodes.

## ESP32

Responsibilities:

- TFT user interface
- Weather API communication
- NTP time synchronization
- Display of weather and temperature information
- Communication with the ESP8266 temperature node

## ESP8266 / NodeMCU

Responsibilities:

- DS18B20 temperature measurement
- Wi-Fi connectivity
- TCP server for temperature data

## Communication

```text
ESP32
  |
  | Wi-Fi / TCP
  v
ESP8266 / NodeMCU
  |
  v
DS18B20
```

The ESP32 also communicates with an external weather service to obtain weather information.

## Release note

The architecture documentation describes the project supplied in the source archive. Any further refactoring should be validated on the target hardware before release.
