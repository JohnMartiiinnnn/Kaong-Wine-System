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
~/.platformio/penv/bin/pio run -t upload --upload-port 192.168.1.137
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
```
