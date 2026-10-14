# Automated Kaong Wine Brewing System

[![PlatformIO CI](https://img.shields.io/badge/PlatformIO-ESP32-orange.svg)](https://platformio.org/)
[![Dual-ESP32 Architecture](https://img.shields.io/badge/Architecture-Dual--ESP32%20UART-blue.svg)]()
[![OTA Enabled](https://img.shields.io/badge/OTA-Enabled-green.svg)]()

Production and thesis repository for the automated, sensor-monitored Kaong (Sugar Palm) Wine Brewing System.

---

## Quick Navigation

* [SYSTEMS.md](file:///home/dave/Projects/Kaong-Wine/SYSTEMS.md) : Master architecture, hardware topology, and deployment commands.
* [SYSTEM_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/SYSTEM_GUIDE.md) : Complete user manual covering all screens, keypad controls, and brewing workflows.
* [PINOUT.md](file:///home/dave/Projects/Kaong-Wine/PINOUT.md) : Comprehensive electrical pin mappings for both ESP32 controllers.
* [CALIBRATION_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/CALIBRATION_GUIDE.md) : Sensor calibration procedures for pH, load cell, and flow meters.
* [YEAST_DISPENSER_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/YEAST_DISPENSER_GUIDE.md) : Automated yeast dispensing motor and calibration guide.
* [AGENTS.md](file:///home/dave/Projects/Kaong-Wine/AGENTS.md) : System rules, display guidelines, and agent instructions.

---

## Deployment Quick Reference

### Primary ESP32 (Wireless OTA)
```bash
# Automated one-click OTA deployment from Beelink (pulls git, compiles, flashes, verifies telemetry)
flash-winebrew

# Direct PlatformIO OTA via LAN IP or mDNS
~/.platformio/penv/bin/pio run -t upload --upload-port 192.168.1.137
~/.platformio/penv/bin/pio run -t upload --upload-port winebrew-main.local
```

### Primary ESP32 (USB Auto-Detect)
```bash
~/.platformio/penv/bin/pio run -t upload
```

### Secondary ESP32 (USB)
```bash
~/.platformio/penv/bin/pio run -d Secondary_Transmitter -e esp32dev -t upload
```

### Live Telemetry (CLI)
```bash
curl -s http://192.168.1.137/data
curl -s http://winebrew-main.local/data
```

---

## System Architecture

The brewing machine operates on a distributed dual-ESP32 architecture interconnected via hardware UART (Serial2, 115200 baud, 8N1):

* **Primary Controller (Master - `src/`)**
  * Hardware: ESP32 DevKit v1 with 4MB Flash and dual-OTA partitions (`min_spiffs.csv`).
  * Peripherals: 3.5" ILI9488 SPI TFT LCD (320x480), MCP23017 I2C GPIO expander (5-button keypad and 8-channel relay manifold), DS3231 precision RTC, SD card SPI logger, and three high-power SSR drivers (GPIO 12, 13, 14).
  * Network: Multi-AP STA failover (`Bedroom Deco`, `Living-Room-WiFi`, `Infinix Hot 60 Pro` hotspot, `CvSU-ICT-WiFi`) plus fallback SoftAP `WineBrew_System` (192.168.4.1).
  * Responsibilities: Brewing state machine orchestration, thermal PID loops, automated fluid transfer logic, safety interlocks, and local HTTP REST telemetry.

* **Secondary Controller (Sensor & Actuator Node - `Secondary_Transmitter/`)**
  * Hardware: ESP32 DevKit v1 / ESP32-C3 inside the fermentation vessel.
  * Peripherals: ADS1115 16-bit ADC reading PH4502C analog probe, DS18B20 OneWire liquid probe, BME280 ambient sensor, NimBLE RAPT Pill digital hydrometer scanner, BTS7960 43A motor driver (mixing impeller), and DRV8871 H-bridge (automated yeast dispenser).
  * Responsibilities: High-resolution analog sensor burst sampling, outlier filtering, BLE packet decoding, and continuous telemetry transmission to Primary.

---

## Signal Processing & Safety Interlocks

* **Dual-Layer pH Filtering & Terminal Acidity Debounce**
  * Layer 1 (Secondary Transmitter): Executes 10-sample ADC burst reads at 5ms spacing on ADS1115 channel 0. Applies a trimmed mean by discarding the two lowest and two highest samples, then averages the central six. An Exponential Moving Average (EMA, alpha 0.30) smooths drift.
  * Layer 2 (Primary Controller): Feeds incoming UART telemetry into an 11-sample rolling circular window median filter and assigns the filtered output directly to incomingData.phValue, delivering stable, noise-immune readings across all UI screens (PH & FERM TEMP, Live Dashboard) and web REST endpoints.
  * Persistence Debounce: Terminal acidity cutoff (pH <= 3.00) requires a sustained 10.0-second persistence timer before tripping Stage 1 advancement to pasteurization.
* **5.0L Chamber Volume Interlock**
  * Enforced at the hardware driver level before any SSR can fire. Chamber 1 requires scale reading >= 5.0 kg; Chamber 2 and Chamber 3 require accumulated flow transfer volume >= 5.0 L.
* **Dynamic 2.0 °C / Second Rate-of-Rise (RoR) Cutoff**
  * Continuous heater temperature monitoring trips `dryElementAlarm` if temperature climbs by >= 2.0 °C in one second, shutting down heating to prevent dry-fire coil damage.
* **Drain-to-Empty Flow Verification (< 0.15L Delta)**
  * Fluid transfers rely on continuous flow sensor pulses. A 15-second rolling settle window with volume delta < 0.05L confirms vessel evacuation before pump disengagement.
* **Pasteurization Trigger Interlock (Test 13 / Table 20)**
  * Automated 10-cycle system check verifying heater activation and cutoff at 60.0 °C setpoint with rate-of-rise validation.

---

## Field Network Configuration

When deploying at CvSU or remote field testing:
* **Mobile Hotspot**: Connect to phone hotspot SSID `Infinix Hot 60 Pro` (Password: `aaaaaaaa`).
* **Campus Wi-Fi**: Pre-configured for `CvSU-ICT-WiFi` (Password: `#cvsuictfreewifi`).
* **Standalone SoftAP**: Connect directly to `WineBrew_System` (Password: `12345678`) at `http://192.168.4.1/data`.

