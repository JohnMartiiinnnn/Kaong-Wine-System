# Automated Kaong Wine Brewing System — Master Registry & Operations

## 1. System Ecosystem Overview

* **Primary Controller (Master)**
  * **Role**: Orchestrates UI, thermal profiles, SSR heater PWM, relay manifolds, flow tracking, load cell measurement, SD logging, and Wi-Fi AP / STA web server.
  * **Location**: `src/`
  * **Platform**: ESP32 DevKit v1 (4MB Flash, `min_spiffs.csv` dual-OTA partition)
  * **Network**: Soft-AP `WineBrew_System` (192.168.4.1), STA `Living-Room-WiFi` (`192.168.1.137`, mDNS `winebrew-main.local`)
  * **Status**: Active (OTA Enabled on port 3232)
  * **Documentation**: [SYSTEM_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/SYSTEM_GUIDE.md)

* **Secondary Controller (Remote Sensor & Actuator Node)**
  * **Role**: Fermentation chamber node managing DS18B20 liquid probe, BME280 ambient environment, ADS1115 pH probe, BLE hydrometer telemetry, BTS7960 mixing impeller, and DRV8871 automated yeast dispenser.
  * **Location**: `Secondary_Transmitter/`
  * **Platform**: ESP32 DevKit v1 / ESP32-C3
  * **Network**: Serial2 UART (115200 baud, 8N1) linked to Primary + fallback AP / STA (`winebrew-secondary.local`)
  * **Status**: Active
  * **Documentation**: [YEAST_DISPENSER_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/YEAST_DISPENSER_GUIDE.md)

* **RAPT Pill Digital Hydrometer**
  * **Role**: Floating BLE beacon broadcasting specific gravity, temperature, and RSSI every 15 to 60 seconds.
  * **Interface**: Bluetooth Low Energy (NimBLE beacon decoder on Secondary ESP32)
  * **Telemetry**: Specific gravity range 0.980 to 1.150, real-time ABV calculation.

---

## 2. Directory Hierarchy

```
Kaong-Wine/
├── platformio.ini              # Primary ESP32 PlatformIO build environments & OTA config
├── min_spiffs.csv              # Flash partition table (dual 1.9MB app OTA partitions)
├── SYSTEMS.md                  # Master System Architecture and Operations Manual
├── README.md                   # Repository Entrypoint (links to SYSTEMS.md)
├── AGENTS.md                   # AI pair programming guidelines, hardware constraints, and pinouts
├── SYSTEM_GUIDE.md             # Complete user and operator manual (screens, keypad, workflows)
├── PINOUT.md                   # Comprehensive electrical wiring and pin connection reference
├── CALIBRATION_GUIDE.md        # Step-by-step sensor calibration workflows
├── YEAST_DISPENSER_GUIDE.md    # Automated yeast delivery system specs and motor control
├── src/                        # Primary Controller Firmware
│   ├── config.h                # Pin constants, state enums, shared variables, struct definitions
│   ├── main.cpp                # Setup, loop, thermal PID controllers, stage logic, state machine
│   ├── display.h / display.cpp # ILI9488 TFT LCD graphics, menus, dashboards, wizards
│   ├── server.h / server.cpp   # Web server, live JSON telemetry endpoints (/data), web dashboard
│   ├── logging.h / logging.cpp # SD card CSV telemetry logger, UART checksum utilities
│   └── YeastDispenser.h / .cpp # Yeast dispensing command interface for Primary
└── Secondary_Transmitter/      # Secondary Controller Firmware
    ├── platformio.ini          # Secondary build configs (esp32dev and calibration envs)
    └── src/
        ├── main.cpp            # Secondary telemetry transmitter, UART parser, BTS7960 motor driver
        ├── dashboard_server.h  # Secondary local captive portal & web telemetry server
        ├── dashboard_server.cpp# Secondary standalone status dashboard UI
        ├── YeastDispenser.h    # DRV8871 H-Bridge pin definitions and duty cap configurations
        ├── YeastDispenser.cpp  # Yeast motor timing, brake routines, and reverse cycle logic
        └── calibration_utility.cpp # Standalone pH 4.0/7.0 Nernst calibration sketch
```

---

## 3. Hardware Architecture & Pinout Directory

### Primary Controller (Main ESP32)
* **TFT SPI Display (ILI9488, 320x480)**: SCLK=18, MOSI=23, MISO=19, CS=15, DC=2, RST=4.
* **SD Card SPI**: Shared SCLK=18, MOSI=23, MISO=19, CS=5.
* **I2C Bus (MCP23017 + DS3231 RTC)**: SDA=21, SCL=22.
* **MCP23017 Port A**:
  * GPA0: Keypad RIGHT
  * GPA1: Keypad LEFT
  * GPA2: Keypad UP
  * GPA3: Keypad DOWN
  * GPA4: Keypad SELECT
  * GPA6: Emergency Stop
  * GPA7: Pre-heating Fan Relay
* **MCP23017 Port B**:
  * GPB0: Fermentation Fan Relay 1
  * GPB1: Fermentation Fan Relay 2
  * GPB2: Pump 1 Transfer Relay (Pre-heat to Fermentation)
  * GPB3: Pump 2 Transfer Relay (Fermentation to Pasteurization)
  * GPB4: Auxiliary Relay
  * GPB5: Status Light Green
  * GPB6: Status Light Yellow
  * GPB7: Status Light Red
* **Direct High-Power SSR Actuators**:
  * GPIO 13: Pre-heat Immersion Heater SSR (Slow PWM, 2-second time-proportioning window)
  * GPIO 12: Fermentation Quartz Heater SSR (Slow PWM, pulse duration capped at 1.5s max)
  * GPIO 14: Pasteurization Immersion Heater SSR (Slow PWM, 2-second time-proportioning window)
* **Direct Sensors & Frequency Inputs**:
  * GPIO 25: 4-Wire Fan PWM (LEDC ch0, 25 kHz, 8-bit)
  * GPIO 26: DS18B20 Shared OneWire Bus (Pre-heat liquid probe and Pasteurization liquid probe)
  * GPIO 32: Flow Sensor 1 (Hardware interrupt on RISING edge)
  * GPIO 34: Flow Sensor 2 (Hardware interrupt on RISING edge)
  * GPIO 36 (DT) & GPIO 27 (SCK): HX711 Load Cell Interface
  * GPIO 16 (RX2) & GPIO 17 (TX2): Serial2 UART Link to Secondary

### Secondary Controller (Remote Node)
* **I2C Bus (ADS1115 ADC + BME280 Ambient)**: SDA=21, SCL=22.
* **OneWire Bus**: GPIO 13 (Fermentation liquid probe DS18B20).
* **UART Link**: GPIO 16 (RX2 to Main TX17), GPIO 17 (TX2 to Main RX16).
* **BTS7960 Mixing Impeller**: LPWM=GPIO 25 (LEDC ch1), RPWM=GPIO 26 (LEDC ch2), 1 kHz PWM.
* **DRV8871 Yeast Dispenser**: IN1=GPIO 5 (LEDC ch4), IN2=GPIO 4 (LEDC ch5), 1 kHz PWM capped at 50% duty (6V max).

---

## 4. Brewing State Machine & Stages

The system supports automated batch brewing with NVS memory persistence:

* **Stage 0: Pre-Heating & Sterilization**
  * Target: Liquid heated to setpoint (e.g. 40.0 C for testing, 72.0 C for standard production).
  * Cool-down: Active fan cooling reduces temperature to transfer threshold (e.g. 38.0 C for testing, 30.0 C for pitch).
  * Auto-Transfer: Pump 1 energizes, transferring liquid to the fermentation tank governed strictly by Flow Sensor 1 pulses. Once liquid runs out and pulses cease for 60 seconds (1 minute), the pre-heat chamber is confirmed completely evacuated and the system initializes Fermentation.
* **Stage 1: Fermentation**
  * Inoculation: Automated yeast dispenser dispenses exact weighed grams or pulse duration.
  * Thermal Control: Quartz radiant heater PID regulates chamber temperature to maintain target (e.g. 38.0 C testing / 28.0 C production).
  * Agitation: Mixing impeller executes scheduled cycles (5 minutes ON, 355 minutes OFF) or manual continuous speed override.
  * Completion: Monitors specific gravity progression, pH drop, and elapsed fermentation duration.
  * Auto-Transfer: Pump 2 transfers fermented wine to the pasteurization tank governed strictly by Flow Sensor 2 pulses. Once liquid runs out and pulses cease for 60 seconds (1 minute), the fermentation chamber is confirmed completely evacuated and the system initializes Pasteurization.
* **Stage 2: Pasteurization**
  * Target: Liquid heated to 65.0 C to 72.0 C via immersion heater and held for 15 minutes to stabilize wine.
  * Cooling: Natural or assisted cooling back to ambient before bottling.
* **Dual-Transfer Test Mode (`TRANSFER TEST`)**
  * Enabled in New Brew Wizard.
  * Executes back-to-back fluid transfer tests (Chamber 1 -> Chamber 2 -> Chamber 3) with zero heating, zero fans, and zero yeast dispensing to safely validate pump flow and plumbing.

---

## 5. Hardware Safety Interlocks & Thermal Protection

* **Flow-Sensor Drain-to-Empty and Dual-Stage Settle Verification (< 0.15L Delta)**
  * All inter-chamber liquid transfers rely 100% on flow sensor pulse activity to verify liquid movement and complete emptying.
  * A 15-second pump priming grace period prevents false stall detection while lines fill.
  * Microsecond ISR filtering rejects pulse intervals under 14ms (flow rate > 9.5 L/min) and tests pin state HIGH, stopping motor EMI and empty-pipe turbine spinning on GPIO 34.
  * When bulk liquid is evacuated, pump cavitation produces intermittent bubbles and droplets. During bulk transfer, a 20-second rolling window (`TRANSFER_DRAIN_TIMEOUT_MS = 20000UL`) detects stalls. Once 90% of expected batch volume is moved, a 10-second window (`TRANSFER_DRAIN_SETTLE_MS = 10000UL`) confirms complete chamber evacuation if volume change is under 0.15 L (`TRANSFER_DRAIN_MAX_DELTA_L = 0.15f`).
  * Anti-runaway ceiling terminates transfer cleanly when volume reaches `max(target * 1.25 + 0.5L, target + 1.0L)` and clamps destination chamber volume, preventing dry-run loops.
  * Load cell initialization volume cap strictly limits any values of transferred liquid volume (`transferVolumeTransferred`, `transfer1Volume`, `transfer2Volume`, and destination `chamberVolume`) to the initial weight (`transferStartWeight`) measured by the load cell after initialization.
  * Minimum volume transfer validation ensures at least 85% of `minVolumeReq` (or >= 0.5L in test mode) moved before advancing; zero/trickle flow with < 0.5L transferred immediately trips `transferDryRunAlarm` and halts stage advancement.
* **5.0L Chamber Volume Interlock (Low-Level SSR Kill-Switch)**
  * Enforced unconditionally at the hardware output layer before `digitalWrite(SSR_*, HIGH)`.
  * **Chamber 1 (Pre-Heat)**: Requires live scale reading `>= 5.0 kg`.
  * **Chamber 2 (Fermentation)**: Requires accumulated Transfer 1 volume `>= 5.0 L`.
  * **Chamber 3 (Pasteurization)**: Requires accumulated Transfer 2 volume `>= 5.0 L`.
  * If volume is `< 5.0 L`, the corresponding SSR is hard-locked to 0V (OFF) and heating percent is zeroed.
* **Dynamic 2.0 °C / Second Thermal Rate-of-Rise (RoR) Cutoff**
  * Active whenever any SSR is firing, across any baseline temperature.
  * Samples heater temperature every 1 second. If temperature climbs by `>= 2.0 °C / sec` (indicating bare heating coil in empty air rather than liquid), `dryElementAlarm` trips immediately.
  * Forces all 3 SSRs to `LOW`, shuts down heating, and persists alarm state in NVS.
* **Quartz Heater Pulse Limiter**
  * Fermentation quartz heater continuous ON duration is capped at **1.5 seconds max** per pulse cycle to protect insulation.
* **1-Wire DS18B20 Dynamic Hot-Plug Auto-Recovery**
  * Re-scans OneWire bus every 5 seconds if sensors report disconnected or invalid readings (`-127.0 °C` or `85.0 °C`), restoring live telemetry automatically upon probe replacement.

---

## 6. Wireless OTA, Telemetry Daemon & Cloud Sync

PlatformIO CLI commands for building and uploading firmware:

* **Compile Primary Controller**:
  `~/.platformio/penv/bin/pio run`

* **Wireless OTA Upload to Primary**:
  `~/.platformio/penv/bin/pio run -t upload --upload-port 192.168.1.137`
  or using mDNS:
  `~/.platformio/penv/bin/pio run -t upload --upload-port winebrew-main.local`

* **USB Upload to Primary**:
  `~/.platformio/penv/bin/pio run -t upload`

* **Compile Secondary Controller**:
  `~/.platformio/penv/bin/pio run -d Secondary_Transmitter -e esp32dev`

* **USB Upload to Secondary**:
  `~/.platformio/penv/bin/pio run -d Secondary_Transmitter -e esp32dev -t upload`

* **Live Telemetry Inspection (CLI)**:
  `curl -s http://192.168.1.137/data`

* **Beelink 24/7 Wi-Fi Telemetry Logger Service**:
  * Daemon location: `~/thesis/Kaong-Wine/logger_daemon.py` / `/home/dave/systems/winebrew-logger/winebrew_logger.py`
  * Dynamic batch rotation: Logs active brews to `brew_wifi_YYYYMMDD_HHMMSS.csv` and standby data to `idle_telemetry.csv`.
  * Status command: `systemctl --user status winebrew-logger.service`

* **Google Sheets Real-Time Multi-Batch Sync**:
  * Sync script: `~/thesis/Kaong-Wine/winebrew_sheets_sync.py`
  * Automatically provisions tabs (`Batch 1`, `Batch 2`, `Batch 3`), inserts styled bold headers, and freezes Row 1.

* **Beelink Automated One-Click OTA Deployment**:
  * Command on Beelink: `flash-winebrew` (or `~/.local/bin/flash-winebrew`)
  * Actions performed: Pulls latest git commit, compiles firmware, flashes ESP32 via OTA on `192.168.1.137`, and verifies `/data` response.

---

## 7. Sensor Calibration References

* **pH Sensor (PH4502C + ADS1115)**: Dual-slope Nernst temperature compensation. See [CALIBRATION_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/CALIBRATION_GUIDE.md).
* **Load Cell (HX711)**: Tare and calibration coefficient stored in NVS. Available in `SYSTEM CHECK` item 10 or `SETTINGS`.
* **Flow Sensors (YF-S201 / YF-B series)**: Pulses per liter calibration stored in EEPROM/NVS.
* **Yeast Dispenser (DRV8871)**: Milliseconds per gram delivery curve configured via `msPerGramYeast`. See [YEAST_DISPENSER_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/YEAST_DISPENSER_GUIDE.md).
