# Wiring

The exact pin mapping should be verified against the physical hardware before publication.

## ESP32 / TFT

The project contains a `User_Setup.h` file for the TFT_eSPI configuration. Use the settings matching the actual TFT controller, resolution, wiring, and rotation.

## ESP8266 / DS18B20

The DS18B20 data pin must be connected to the GPIO configured by the source code and normally requires an appropriate pull-up resistor.

## Network

Configure the ESP8266 static IP and TCP port to match the ESP32 client configuration.

Do not publish private network credentials or API keys.
