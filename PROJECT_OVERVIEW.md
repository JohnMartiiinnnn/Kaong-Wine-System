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
    *   **PWM (TBD):** TB6612FN PWMA — mixing impeller speed (LEDC ch1, 1 kHz, 8-bit). GPIO pin not yet assigned — Primary ESP32 has no free output pins.
    *   **AC Dimmer:** Pins 12, 13, 14 for heating control; Pin 32 for Zero-Cross detection.
*   **Motor Driver (TB6612FN):**
    *   **PWMA → TBD** (GPIO pin pending — no free output pin on Primary ESP32)
    *   **AIN1 → 3.3 V** hardwired — fixed CW direction
    *   **AIN2 → GND** hardwired
    *   **STBY → 3.3 V** hardwired — always active
    *   **VM → 12 V**, **VCC → 3.3 V**
    *   Motor: JGY370 12 V DC worm gear, 40–55 RPM, stall current ≤ 1.5 A
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
    *   `MIXER_MENU`: Mixing impeller control — OFF / MANUAL (speed adjust) / AUTO (5 min ON, 355 min OFF).
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
| **Motor PWM** | TBD | TB6612FN PWMA (LEDC ch1) — no free GPIO currently |

---

## 6. Mixing Impeller — Connection, Testing & Debugging

### 6.1 Hardware Connection

Wire the TB6612FN motor driver as follows. All hardwired pins are connected directly to the rail — no ESP32 GPIO needed for direction or standby.

| TB6612FN Pin | Connect to | Notes |
| :--- | :--- | :--- |
| VM | 12 V supply | Motor power rail |
| VCC | 3.3 V | Logic supply |
| GND | GND | Common ground |
| STBY | 3.3 V | Tie HIGH — always active |
| AIN1 | 3.3 V | Tie HIGH — fixed CW direction |
| AIN2 | GND | Tie LOW — fixed CW direction |
| PWMA | GPIO0 (ESP32) | Speed control (0–100 % duty) |
| AO1 | Motor + | Red wire of JGY370 |
| AO2 | Motor – | Black wire of JGY370 |

> **GPIO0 note:** GPIO0 is the ESP32 boot-strapping pin. It is safe to use as PWM output after the firmware boots. During firmware flashing, the motor driver must be disconnected or STBY pulled LOW to prevent the motor from running while the board re-programs.

### 6.2 UI Navigation to Mixer Control

```
MAIN MENU
  └─ CONTINUE BREW  (or NEW BREW → start brew)
       └─ DASHBOARD
            └─ Navigate to FERMENTATION tab (RIGHT/DOWN)
                 └─ SELECT  →  enter Fermentation module view
                      └─ SELECT  →  MIXER CONTROL menu

In MIXER CONTROL:
  SELECT         — cycle mode: OFF → MANUAL → AUTO → OFF
  UP / DOWN      — adjust speed in 10 % steps (MANUAL mode only)
  LEFT / RETURN  — exit back to dashboard
```

The Fermentation module view always shows a live **MIXER** row at the bottom:
- `OFF: 0%` — motor stopped
- `MANUAL: 80%` — running at the set duty cycle
- `AUTO: RUNNING` — auto-schedule ON period active
- `AUTO: STANDBY` — auto-schedule waiting for next cycle

### 6.3 Functional Test Procedure

Perform these checks before installing the impeller into the fermentation vessel.

**Step 1 — Bench power test (no firmware needed)**

1. Connect TB6612FN with STBY and AIN1 to 3.3 V, AIN2 to GND.
2. Apply 12 V to VM, 3.3 V to VCC.
3. Manually pull PWMA HIGH (connect to 3.3 V).
4. Motor should spin immediately. Confirm direction (CW from above shaft).
5. Pull PWMA LOW — motor should stop (coast, no brake).

**Step 2 — Manual mode test (firmware running)**

1. Flash firmware and connect the ESP32 normally.
2. Boot the system and start or bypass a brew.
3. Navigate to the MIXER CONTROL menu.
4. Press SELECT once → mode changes to **MANUAL**, motor starts at 100 %.
5. Press DOWN → speed decreases in 10 % steps; confirm motor slows.
6. Press DOWN until 0 % → motor should stop.
7. Press UP to restore speed.
8. Press LEFT to exit — motor keeps running at last set speed.

**Step 3 — Auto mode test (short-cycle verification)**

To verify the scheduler without waiting 6 hours, temporarily change `MIXER_ON_MS` and `MIXER_OFF_MS` in `config.h` to short values (e.g., 10 s ON / 20 s OFF), flash, and confirm:
- Motor starts on AUTO activation.
- Motor stops after `MIXER_ON_MS` elapses.
- Motor restarts after `MIXER_OFF_MS` elapses.
- Restore original values (5 min / 355 min) before production use.

**Step 4 — Emergency stop test**

1. While motor is running (MANUAL or AUTO), press the ESTOP button.
2. Motor must stop immediately.
3. Verify the system halted screen appears and motor does not restart.
4. Reboot to reset.

### 6.4 Debugging

**Motor does not spin**

| Symptom | Likely cause | Fix |
| :--- | :--- | :--- |
| Motor silent in MANUAL at 100 % | GPIO0 PWM not reaching driver | Probe GPIO0 with a meter — should show ~2.5 V average at 50 % duty. Check `ledcAttachPin` call in `setup()`. |
| Motor silent, driver hot | STBY or AIN1 not pulled HIGH | Verify 3.3 V on STBY and AIN1 with a meter. |
| Motor silent, driver cool | 12 V rail missing | Check VM pin on TB6612FN. |
| Motor runs at boot briefly then stops | GPIO0 pulled LOW during boot | Normal boot behavior — motor will start once `setup()` runs `setMixerSpeed(0)`. Not an issue. |

**Motor runs in wrong direction**

Swap AO1 and AO2 leads at the motor terminals. AIN1/AIN2 are hardwired so direction can only be changed at the motor connector.

**Speed does not change with UP/DOWN**

Confirm mode is **MANUAL** — UP/DOWN only works in MANUAL mode. In AUTO mode, speed is firmware-controlled.

**AUTO mode never runs motor**

Check that `mixerCycleTimer` initialises to 0 when AUTO is first activated — it does by design, so the first ON period starts immediately. If it still doesn't start, verify `MIXER_OFF_MS` constant in `config.h` (should be `355UL * 60 * 1000`).

**Serial monitor output**

The firmware prints `raw=` and `ema=` weight values over `Serial` at 115200 baud. There is no dedicated mixer log line, but the PWM duty can be confirmed with a logic analyser or oscilloscope on GPIO0.

### 6.5 Known Constraints

- **GPIO0 is the boot pin.** Do not hold GPIO0 LOW externally during power-on or the ESP32 will enter bootloader mode instead of running firmware.
- **No brake mode.** When speed is set to 0 % (PWMA = LOW with AIN1 HIGH, AIN2 LOW), the TB6612FN enters coast mode, not active brake. The worm gear's self-locking mechanism prevents the impeller from back-driving, so this is acceptable.
- **Single-channel only.** Only the A channel of the TB6612FN is used (1.2 A continuous). The B channel is unused. If higher torque is needed in the future, bridge both channels in parallel for 2.4 A continuous.
