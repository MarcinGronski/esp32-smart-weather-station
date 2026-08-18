# Git Weather Station

ESP32-based weather station with a separate NodeMCU temperature sensor.

The system consists of two devices:

* **ESP32** — main weather station with TFT display
* **NodeMCU ESP8266** — remote temperature node with DS18B20 sensor

The ESP32 communicates with the NodeMCU over TCP and requests the temperature only when it is needed.

---

## Features

* Weather station based on ESP32
* TFT display using TFT_eSPI
* Remote DS18B20 temperature sensor
* NodeMCU ESP8266 temperature node
* TCP communication between ESP32 and NodeMCU
* Temperature request using the `TEMP` command
* OTA firmware updates
* Wi-Fi connectivity
* Discord notifications
* Static IP configuration for the NodeMCU
* Non-blocking communication approach

---

## System architecture

```text
                    Wi-Fi
              ┌────────────────┐
              │                │
              ▼                │
        ┌──────────────┐       │
        │    ESP32     │       │
        │              │       │
        │ Weather      │       │
        │ Station      │       │
        │              │       │
        │ TFT Display  │       │
        └──────┬───────┘       │
               │               │
               │ TCP :12345    │
               │               │
               ▼               │
        ┌──────────────┐       │
        │   NodeMCU    │       │
        │   ESP8266    │       │
        │              │       │
        │ DS18B20      │       │
        └──────────────┘       │
                               │
                               │
                         Internet / Wi-Fi
                               │
                               ▼
                          Discord Webhook
```

---

## Temperature communication

The NodeMCU works as a TCP temperature server.

The ESP32 sends:

```text
TEMP
```

to the NodeMCU on TCP port:

```text
12345
```

The NodeMCU then:

1. receives the `TEMP` command,
2. requests a measurement from the DS18B20,
3. reads the temperature,
4. sends the temperature value back to the ESP32.

Example:

```text
ESP32  →  TEMP
NodeMCU → 23.47
```

The NodeMCU does **not** continuously read the temperature in the main loop. The sensor is queried when the ESP32 requests the value.

---

## Hardware

### ESP32

Main weather station controller.

The ESP32 handles:

* TFT display
* weather data
* user interface
* Wi-Fi communication
* communication with the NodeMCU
* OTA updates
* Discord notifications

### NodeMCU ESP8266

Remote temperature node.

The NodeMCU handles:

* Wi-Fi connection
* TCP server on port `12345`
* DS18B20 temperature sensor
* OTA firmware updates

### DS18B20

The DS18B20 is connected to:

```text
NodeMCU D2 → DATA
3.3V       → VCC
GND        → GND
```

A typical 4.7 kΩ pull-up resistor should be connected between DATA and 3.3V.

---

## Software versions

The project was tested using the following versions.

| Software / Library         |    Version |
| -------------------------- | ---------: |
| Arduino IDE                | **2.3.10** |
| ESP32 by Espressif Systems | **2.0.14** |
| TFT_eSPI                   | **2.5.43** |
| DallasTemperature          |  **4.0.6** |
| OneWire                    |  **2.3.8** |
| ArduinoJson                |  **7.4.3** |

### Boards

Main station:

```text
ESP32 Dev Module
```

Temperature node:

```text
NodeMCU ESP8266
```

Using the versions listed above is recommended because this is the tested configuration.

---

## Repository structure

```text
git_weather_station/
│
├── ESP32_WeatherStation/
│   └── ESP32_WeatherStation.ino
│
└── NodeMCU_Temperature/
    └── NodeMCU_Temperature.ino
```

---

## Installation

### 1. Install Arduino IDE

Install:

```text
Arduino IDE 2.3.10
```

### 2. Install ESP32 support

Install:

```text
ESP32 by Espressif Systems
Version: 2.0.14
```

Select:

```text
ESP32 Dev Module
```

for the main weather station.

### 3. Install ESP8266 support

Install the ESP8266 board package and select the appropriate NodeMCU ESP8266 board.

### 4. Install libraries

Install the tested library versions:

```text
TFT_eSPI         2.5.43
DallasTemperature 4.0.6
OneWire          2.3.8
ArduinoJson      7.4.3
```

---

## Configuration

Before compiling the firmware, configure your local Wi-Fi settings.

Do not publish real Wi-Fi credentials in a public repository.

Example:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

Configure your own OTA password:

```cpp
const char* otaPassword = "YOUR_OTA_PASSWORD";
```

If Discord notifications are enabled, configure your own Discord webhook.

Do not publish the real webhook URL on GitHub.

---

## NodeMCU network configuration

The NodeMCU uses a static IP configuration.

Default project configuration:

```text
IP:       192.168.50.105
Gateway:  192.168.50.1
Subnet:   255.255.255.0
DNS:      192.168.50.1
```

TCP server:

```text
Port: 12345
```

If your network uses a different address range, change the network configuration in the NodeMCU firmware.

---

## First firmware upload

The first firmware upload should be performed using USB.

### ESP32

1. Connect the ESP32 using USB.
2. Open `ESP32_WeatherStation.ino`.
3. Select the correct board.
4. Select the correct COM port.
5. Compile the project.
6. Upload the firmware.

### NodeMCU

1. Connect the NodeMCU using USB.
2. Open `NodeMCU_Temperature.ino`.
3. Select the NodeMCU ESP8266 board.
4. Select the correct COM port.
5. Compile the project.
6. Upload the firmware.

After the first successful upload and Wi-Fi connection, OTA can be used for subsequent firmware updates.

---

## OTA updates

Both devices support OTA firmware updates.

After the device connects to Wi-Fi, it becomes available as a network port in Arduino IDE.

The OTA password configured in the firmware must be entered when Arduino IDE requests authentication.

OTA allows firmware updates without connecting the device directly to USB.

---

## NodeMCU temperature server

The NodeMCU listens on:

```text
TCP 12345
```

The expected request is:

```text
TEMP
```

Example communication:

```text
Client:
TEMP

Server:
23.47
```

The temperature is measured only after receiving the request.

---

## Discord

The ESP32 can send notifications to Discord using a Discord webhook.

The webhook URL must be configured locally.

For security reasons, never commit a real Discord webhook URL to a public GitHub repository.

If a webhook is accidentally exposed, revoke it and create a new one.

---

## Troubleshooting

### NodeMCU does not return temperature

Check:

* DS18B20 wiring
* DATA connection to D2
* 3.3V power
* GND connection
* 4.7 kΩ pull-up resistor
* NodeMCU IP address
* TCP port `12345`

### OTA authentication failed

Check that the OTA password entered in Arduino IDE matches:

```cpp
ArduinoOTA.setPassword(...);
```

If the OTA password was changed, the firmware must first be uploaded through USB.

### ESP32 cannot obtain temperature

Check that:

* NodeMCU is powered on
* NodeMCU is connected to Wi-Fi
* IP address is correct
* TCP port `12345` is accessible
* ESP32 sends the command:

```text
TEMP
```

### Discord connection problems

Check:

* Internet access
* DNS configuration
* Discord webhook URL
* webhook validity
* TLS connection

Never publish the webhook token in the repository.

---

## Project status

**Version: 1.0.0**

The current version has been tested with the hardware and software configuration described above.

The goal of this repository is to maintain a stable, reproducible version of the weather station firmware and its remote temperature node.

---

## License

This project is provided under the MIT License.
