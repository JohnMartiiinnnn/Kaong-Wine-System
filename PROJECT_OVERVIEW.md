# Automated Wine Brewing System - Project Overview

This document summarizes the architecture, hardware, and software logic for the Automated Wine Brewing System thesis project. It serves as the primary context for development and troubleshooting.

## 1. System Architecture

The system utilizes two **ESP32 DevKits** communicating via **UART (Serial)**.

*   **Primary Controller (Master):** Handles the UI (TFT LCD), local sensors (Pre-heating/Pasteurization), high-power actuators (Relays/Dimmers), and data logging (SD Card).
*   **Secondary Controller (Remote Sensor Node):** Handles fermentation environment sensors (Ambient/Liquid), pH monitoring, and receives BLE data from a RAPT Pill digital hydrometer.

---

## 2. Primary Controller (Main Unit)

### Hardware Connections
*   **Display:** 3.5" TFT LCD (ILI9488) via SPI.
*   **GPIO Expansion (MCP23017):** 
    *   **GPA0-GPA7:** 8-Channel Relay Module.
    *   **GPB0-GPB4:** 5-Button Keypad (Right, Left, Up, Down, Select).
    *   **GPB6:** Emergency Stop Button.
    *   **GPB7:** Fan Relay.
*   **Sensors:**
    *   **BME280:** Ambient Temperature (Pre-heating area).
    *   **DS18B20 (x2):** Liquid temperatures (Vat/Pasteurization).
    *   **HX711:** Load cell for volume/weight measurement (Liquid Level).
    *   **DS3231 RTC:** Real-time clock for logging timestamps.
*   **Actuators:**
    *   **PWM (Pin 25):** 4-wire Fan speed control.
    *   **AC Dimmer:** Pins 12, 13, 14 for heating control; Pin 32 for Zero-Cross detection.
*   **Storage:** SD Card (SPI) for CSV data logging.

### Software Logic
*   **State Machine:**
    *   `SYSTEM_INIT`: Hardware check and splash screen.
    *   `START_MENU`: Navigation between New Brew, Continue, System Check, and Monitor.
    *   `NEW_BREW_WIZARD`: Volume check (min 10L) before starting.
    *   `DASHBOARD_ACTIVE`: Three modes (Pre-Heating, Fermentation, Pasteurization) displaying relevant data.
    *   `COOLING_MENU`: Manual/Auto fan control.
    *   `SENSOR_MONITOR`: Raw value display for debugging.
*   **Data Logging:** Records all local and remote sensor data to `/data_log.csv` every minute.

---

## 3. Secondary Controller (Remote Node)

### Hardware Connections
*   **Sensors:**
    *   **BME280/BMP280:** Fermentation ambient temp and pressure.
    *   **DS18B20:** Fermentation liquid temp.
    *   **ADS1115 + PH4502C:** High-precision pH monitoring.
*   **Wireless:**
    *   **BLE:** Scans for RAPT Pill beacons to extract Specific Gravity (SG) and internal temperature.

### Software Logic
*   **RAPT Pill Decoding:** Parses manufacturer-specific BLE data to extract battery level, temperature, and density (converted to SG).
*   **pH Calculation:** Uses Nernst-based temperature compensation using the DS18B20 liquid temp reading.
*   **Data Transmission:** Packages all readings into a `struct_message` with a `0xDEADBEEF` signature and XOR checksum, sent via UART to the Main unit every 1 second.

---

## 4. Communication Protocol (UART)

The controllers exchange data using a packed C-struct over `Serial2` at 115200 baud.

### Data Structure (`struct_message`)
| Field | Type | Description |
| :--- | :--- | :--- |
| `signature` | `uint32_t` | `0xDEADBEEF` for sync |
| `pillTemp` | `float` | RAPT Pill Temperature |
| `pillGravity` | `float` | RAPT Pill Specific Gravity |
| `room2Temp` | `float` | Remote Ambient Temp |
| `room2Pres` | `float` | Remote Ambient Pressure |
| `phValue` | `float` | Current pH Level |
| `room2LiquidTemp` | `float` | Remote Liquid Temp |
| `sensor2Status` | `uint8_t` | BME/BMP Health |
| `adsStatus` | `uint8_t` | ADS1115 Health |
| `ds18Status` | `uint8_t` | DS18B20 Health |
| `bleStatus` | `uint8_t` | BLE Scanner Status |
| `pillBattery` | `uint8_t` | RAPT Pill Battery % |
| `pillRSSI` | `int16_t` | BLE Signal Strength |
| `checksum` | `uint8_t` | Integrity check |

---

## 5. Pin Mapping Summary (Main ESP32)

| Component | Pin | Note |
| :--- | :--- | :--- |
| **TFT CS** | 15 | SPI |
| **TFT DC** | 2 | SPI |
| **TFT RST** | 4 | SPI |
| **SD CS** | 5 | SPI |
| **SPI SCLK** | 18 | Shared |
| **SPI MOSI** | 23 | Shared |
| **SPI MISO** | 19 | Shared |
| **I2C SDA** | 21 | RTC, MCP23017 |
| **I2C SCL** | 22 | RTC, MCP23017 |
| **PWM Fan** | 25 | 25kHz PWM |
| **OneWire** | 26 | Liquid Sensors |
| **HX711 DT** | 36 | Load Cell |
| **HX711 SCK** | 27 | Load Cell |
| **UART RX** | 16 | From Secondary |
| **UART TX** | 17 | To Secondary |
| **AC ZC** | 32 | Zero Cross |
| **AC DIM1** | 14, 12 | Dimmer Channels |
| **AC DIM2** | 13 | Shared |
