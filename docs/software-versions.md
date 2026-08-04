# Software Versions

This project was developed using the following software environment.

## Development Environment

| Component | Version |
|---|---|
| Arduino IDE | 2.3.10 |
| Board | ESP32 C3 ARD-ESP32-C3-TFT24 |
| ESP32 Arduino Core | 2.0.14 |

## Libraries

| Library | Version |
|---|---|
| TFT_eSPI | 2.5.43 |
| ArduinoJson | 7.4.3 |
| OneWire | 2.3.8 |
| DallasTemperature | 4.0.6 |

## Display

The project uses the TFT_eSPI library.

The display configuration is stored in:

`esp32_weather_station/User_Setup.h`

Make sure the TFT controller, resolution, SPI pins and RGB color order match the actual hardware.

## Notes

The versions listed above describe the development environment used for this project.

For reproducible results, use the same library and board package versions when building the project.
