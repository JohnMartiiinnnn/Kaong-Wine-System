# Automated Wine Brewing System - Project Overview

This document summarizes the architecture, hardware, and software logic for the Automated Wine Brewing System thesis project. It serves as the primary context for development and troubleshooting.

---

## 1. System Architecture

The system utilizes two **ESP32 DevKits** communicating via **UART (Serial)**.

*   **Primary Controller (Master):** Handles the UI (TFT LCD), local sensors (Pre-heating/Pasteurization), high-power actuators (Relays/Dimmers), and data logging (SD Card).
*   **Secondary Controller (Remote Sensor Node):** Handles fermentation environment sensors (Ambient/Liquid), pH monitoring, and receives BLE data from a RAPT Pill digital hydrometer.

---

## 2. Primary Controller — Source Layout

The primary firmware lives in `src/` and is split into modules:

```
src/
  config.h       — All #includes, pin constants, enums, struct_message, extern declarations
  display.h/.cpp — All TFT draw functions (drawDashboard, drawWizard, etc.)
  server.h/.cpp  — Wi-Fi AP web server (INDEX_HTML, handleRoot, handleData)
  logging.h/.cpp — SD card CSV logging (logDataToSD) and UART checksum
  main.cpp       — Global variable definitions, setFanSpeed, setup(), loop()
```

**Rule:** every `.cpp` file includes only its own `.h`. All shared symbols live in `config.h` and are declared `extern` there — they are *defined once* in `main.cpp`.

### Hardware Connections
*   **Display:** 3.5" TFT LCD (ILI9488) via SPI.
*   **GPIO Expansion (MCP23017):**
    *   **GPA0-GPA7:** 8-Channel Relay Module.
    *   **GPB0-GPB4:** 5-Button Keypad (Right, Left, Up, Down, Select).
    *   **GPB6:** Emergency Stop Button.
    *   **GPB7:** Fan Relay.
*   **Sensors:**
    *   **BME280:** Ambient Temperature (Pre-heating area).
    *   **DS18B20 (x2):** Liquid temperatures (Vat/Pasteurization) on a shared OneWire bus (pin 26).
    *   **HX711:** Load cell for volume/weight measurement (DT=36, SCK=27).
    *   **DS3231 RTC:** Real-time clock for logging timestamps.
*   **Actuators:**
    *   **PWM (Pin 25):** 4-wire Fan speed control.
    *   **AC Dimmer:** Pins 12, 13, 14 for heating control; Pin 32 for Zero-Cross detection.
*   **Storage:** SD Card (SPI, CS=5) for CSV data logging.

### Software Logic
*   **HX711 Filtering (Hybrid EMA):** 
    *   **Smoothing:** Uses an Exponential Moving Average (EMA) with $\alpha=0.2$ to eliminate drift and noise.
    *   **Responsiveness:** Snaps instantly to raw values if a change $>0.5$ L is detected (object added/removed).
    *   **Safety:** Rejects readings outside $-5.0$ to $70.0$ L and floors results at $0.0$ L.
*   **State Machine:**
    *   `SYSTEM_INIT` → `START_MENU` (after splash)
    *   `NEW_BREW_WIZARD`: Volume check (min 10 L) before starting brew.
    *   `DASHBOARD_ACTIVE`: Three sub-views — Pre-Heating, Fermentation, Pasteurization.
    *   `COOLING_MENU`: Manual/Auto fan control.
    *   `SENSOR_MONITOR`: Raw value display for debugging.
    *   `CALIBRATION_MODE`: Live scale tare and calibration factor adjust.
*   **Data Logging:** Records all sensor data to `/data_log.csv` every 60 seconds.
*   **Web Dashboard:** Soft-AP `WineBrew_System` (pass: `12345678`), mDNS `winebrew.local`. Live JSON at `/data`, UI at `/`.

---

## 3. Secondary Controller — Source Layout

```
Secondary_Transmitter/src/
  main.cpp   — Self-contained: BLE scan, pH, DS18B20, BME280, UART transmit
```

### Hardware Connections
*   **Sensors:**
    *   **BME280/BMP280:** Fermentation ambient temp and pressure (I2C).
    *   **DS18B20:** Fermentation liquid temp (OneWire, non-blocking).
    *   **ADS1115 + PH4502C:** High-precision pH monitoring.
*   **Wireless:**
    *   **BLE (NimBLE):** Scans for RAPT Pill beacons — SG and internal temperature.

### Software Logic
*   **BLE race condition guard:** `portMUX_TYPE` spinlock protects `txData` writes (BLE callback, Core 0) from UART reads (loop, Core 1).
*   **Non-blocking DS18B20:** `setWaitForConversion(false)` + read-then-request pattern (reads prior result, then requests the next conversion).
*   **RAPT Pill Decoding:** Manufacturer-specific BLE advertisement → battery, temperature, density (SG).
*   **pH Calculation:** Nernst-based temperature compensation using DS18B20 liquid temp.
*   **Data Transmission:** `struct_message` with `0xDEADBEEF` signature + XOR checksum over UART at 115200 baud, sent every 1 second.

---

## 4. Communication Protocol (UART)

Controllers exchange data using a packed C-struct over `Serial2` at 115200 baud.

| Field | Type | Description |
| :--- | :--- | :--- |
| `signature` | `uint32_t` | `0xDEADBEEF` for frame sync |
| `pillTemp` | `float` | RAPT Pill Temperature |
| `pillGravity` | `float` | RAPT Pill Specific Gravity |
| `room2Temp` | `float` | Remote Ambient Temp |
| `room2Pres` | `float` | Remote Ambient Pressure |
| `phValue` | `float` | Current pH Level |
| `room2LiquidTemp` | `float` | Remote Liquid Temp |
| `sensor2Status` | `uint8_t` | BME/BMP Health (0=fail, 1=ok) |
| `adsStatus` | `uint8_t` | ADS1115 Health |
| `ds18Status` | `uint8_t` | DS18B20 Health |
| `bleStatus` | `uint8_t` | BLE Scanner Status |
| `pillBattery` | `uint8_t` | RAPT Pill Battery % |
| `pillRSSI` | `int16_t` | BLE Signal Strength (dBm) |
| `checksum` | `uint8_t` | XOR over all preceding bytes |

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
| **PWM Fan** | 25 | 25 kHz PWM |
| **OneWire** | 26 | DS18B20 (shared bus) |
| **HX711 DT** | 36 | Load Cell data |
| **HX711 SCK** | 27 | Load Cell clock |
| **UART RX** | 16 | From Secondary |
| **UART TX** | 17 | To Secondary |
| **AC ZC** | 32 | Zero Cross |
| **AC DIM1** | 14, 12 | Dimmer Channels |
| **AC DIM2** | 13 | Shared |
