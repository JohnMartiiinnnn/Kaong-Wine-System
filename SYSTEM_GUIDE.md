# Kaong Wine Brewing System — Complete Guide

This document explains how the automated kaong wine brewing system works, from first boot through a complete brew cycle. It covers every screen, every button, and every hardware component.

---

## Table of Contents

1. [Hardware Overview](#1-hardware-overview)
2. [Keypad Controls](#2-keypad-controls)
3. [First Boot](#3-first-boot)
4. [Main Menu](#4-main-menu)
5. [Starting a Brew](#5-starting-a-brew)
6. [The Dashboard](#6-the-dashboard)
7. [Brewing Stages](#7-brewing-stages)
8. [Stage Parameters](#8-stage-parameters)
9. [System Check Tests](#9-system-check-tests)
10. [Flow Sensor Calibration](#10-flow-sensor-calibration)
11. [Load Cell Calibration](#11-load-cell-calibration)
12. [Sensor Monitor](#12-sensor-monitor)
13. [Web Interface](#13-web-interface)
14. [Emergency Stop](#14-emergency-stop)
15. [Status Lights](#15-status-lights)
16. [Data Logging](#16-data-logging)

---

## 1. Hardware Overview

The system consists of two ESP32 microcontrollers connected by a serial cable. The **Primary ESP32** runs the display, controls all heaters and relays, and handles the user interface. The **Secondary ESP32** is a sensor node inside the fermentation tank that measures temperature, pH, specific gravity (via Bluetooth hydrometer), and drives the mixer motor.

### Primary ESP32 — What it controls

| Component | What it does |
|-----------|-------------|
| ILI9488 TFT Display (320×480) | Shows all menus and live data |
| 5-button keypad (via MCP23017) | User input |
| 3× SSR (Solid State Relay) | Controls heaters: Pre-heat, Fermentation, Pasteurization |
| Pre-heating fan relay (GPA7) | Cools the pre-heat tank after sterilization |
| Fermentation fan relay (GPB0) | Cools the fermentation tank when too warm |
| Pump relay 1 (GPB1) | Transfer pump: Pre-heat → Fermentation |
| Pump relay 2 (GPB2) | Transfer pump: Fermentation → Pasteurization |
| 3× Status lights (GPB5/6/7) | Green, Yellow, Red indicator lights |
| Flow sensor 1 (GPIO 32) | Measures volume transferred: Pre-heat → Ferm |
| Flow sensor 2 (GPIO 34) | Measures volume transferred: Ferm → Past |
| DS18B20 × 2 (GPIO 26, shared bus) | Liquid temperature: Pre-heat tank and Pasteurization tank |
| BME280 (I2C) | Local ambient temperature and pressure |
| HX711 + Load cell | Weighs liquid in the pre-heat tank (in liters) |
| RTC DS3231 (I2C) | Keeps real date and time for logging |
| SD card | Logs sensor data every 60 seconds to CSV |

### Secondary ESP32 — What it measures

Located inside or near the fermentation tank.

| Sensor | What it measures |
|--------|-----------------|
| DS18B20 (GPIO 13) | Fermentation liquid temperature |
| BME280 (I2C) | Fermentation ambient temperature and pressure |
| ADS1115 + PH4502C | pH of the fermenting liquid |
| BLE Hydrometer (Bluetooth) | Specific gravity and temperature of the liquid |
| BTS7960 motor driver | Drives the fermentation mixer motor |

The Secondary sends all of this data to the Primary over a serial cable (UART, 115200 baud) once per second.

### Tanks and Flow Path

```
[Pre-Heat Tank]  --(Pump 1 / Flow Sensor 1)-->  [Fermentation Tank]
                                                         |
                                              (Pump 2 / Flow Sensor 2)
                                                         |
                                                 [Pasteurization Tank]
```

The pre-heat tank has the load cell underneath it. Liquid flows through the system in one direction only.

---

## 2. Keypad Controls

Five buttons are used throughout the system. Their meaning changes depending on the current screen, but the general rules are consistent:

| Button | General Role |
|--------|-------------|
| UP | Move selection up / increase a value |
| DOWN | Move selection down / decrease a value |
| LEFT | Go back to the previous screen |
| RIGHT | Go forward / increase a value / enter a sub-screen |
| SELECT | Confirm / toggle / activate |

On most screens, the currently selected item is highlighted with a white border.

---

## 3. First Boot

On power-on, the system shows a splash screen while it initializes all hardware. After about 2 seconds it moves to the Main Menu automatically.

During boot, the system checks:
- BME280 ambient sensor
- DS18B20 liquid temperature sensors
- RTC clock module
- SD card
- HX711 load cell
- Secondary ESP32 (detected via UART heartbeat)

If any of these fail, they are marked as unavailable and the system continues without them. Failed sensors show as errors on the relevant screens.

---

## 4. Main Menu

Five options, navigated with UP/DOWN and confirmed with SELECT.

| Option | What it does |
|--------|-------------|
| NEW BREW | Starts a new brew cycle with a volume check |
| CONTINUE BREW | Goes to the dashboard (use if returning after a restart mid-brew — note: brew state is not saved to flash, so stage must be re-set manually) |
| SYSTEM CHECK | Opens the hardware test menu |
| SENSOR VALUES | Shows a live read of all sensors |
| DEMO RUN | Runs a full simulated 3-stage brew automatically (no real heat or liquid required) |

---

## 5. Starting a Brew

### New Brew Wizard

After selecting NEW BREW, the wizard checks the load cell.

- If more than 10 liters are on the scale: **PROCEED** is available. SELECT on PROCEED starts the brew from Stage 0 (Pre-Heating).
- If there is less than 10 liters: only **BYPASS (TEST)** is available. This skips the volume check and starts the brew anyway — use for dry testing.

When a brew starts:
- The first 5 valid specific gravity readings from the Bluetooth hydrometer are averaged and saved as the **Original Gravity** (OG). This is used later to calculate alcohol content.
- The Stage 0 pre-heating process begins automatically.

---

## 6. The Dashboard

The dashboard has two views that you switch between using SELECT.

### Summary View

Shows three stage tiles (Pre-Heating, Fermentation, Pasteurization) and a live temperature graph below them.

- The **active stage** tile is marked with a white dot and shows a live elapsed timer.
- Completed stages show their total elapsed time.
- UP/DOWN cycles which tile is highlighted (stages 0, 1, 2).
- SELECT on the active stage tile enters the **Module View** for that stage.
- The temperature graph scrolls automatically, one data point per second.
- During liquid transfers between tanks, the graph area is replaced with a countdown timer.

### Module View

An expanded view of a single stage showing all of its sensor values in detail.

**Stage 0 — Pre-Heating:**
- Ambient temperature (BME280)
- Liquid temperature (DS18B20)
- Heating output (%)
- Cooling output (%)
- Fan mode
- Current weight (liters)

**Stage 1 — Fermentation:**
- Ambient temperature (inside ferm tank)
- Liquid temperature
- Specific gravity
- pH level
- BLE hydrometer signal strength and battery
- Mixer status
- Estimated ABV

**Stage 2 — Pasteurization:**
- Pasteurization liquid temperature
- Process status (heating / holding / complete)

Press LEFT to return to summary view. Press RIGHT to open the Stage Parameters screen for the active stage.

### Dashboard Header

The top bar always shows:
- "DASHBOARD"
- Brew start date/time
- SD card status (READY or ERROR)
- Last log write time

---

## 7. Brewing Stages

### Stage 0 — Pre-Heating

**Goal:** Sterilize the raw kaong sap by heating it to 80°C, then cool it down before transferring.

**What happens automatically:**
1. The system runs a PID controller targeting 80°C using the pre-heat tank DS18B20 sensor.
2. Once the liquid reaches 80°C, a 15-second hold timer begins.
3. After the 15-second hold, the heater turns off. Sterilization is complete.
4. The pre-heating fan turns on at 100% to cool the liquid down to 30°C.
5. Once the liquid is at or below 30°C, the fan turns off. The stage is ready to advance.

**To advance to Fermentation:**
Open Stage Parameters (RIGHT in module view) and SELECT "ADVANCE TO FERMENTATION." The system fires the pre-heat→ferm pump relay for 10 seconds, then switches to Stage 1.

**Status light:** RED on during this stage.

---

### Stage 1 — Fermentation

**Goal:** Hold the liquid at fermentation temperature (27-30°C) while yeast converts sugar to alcohol.

**What happens automatically:**
- If the liquid temperature drops below 27°C: the fermentation heater turns on at 100%.
- If the liquid temperature rises above 30°C: the heater turns off and the fermentation fan turns on.
- Between 27-30°C: everything stays off (idle band).

**Monitoring:**
- pH is monitored continuously. If the pH drops below the target (default 3.0), a pH alert activates.
- Specific gravity is read from the Bluetooth hydrometer. When it reaches the target gravity (default 1.010), the fermentation is considered complete.
- The mixer runs automatically in AUTO mode: 5 minutes on, then 355 minutes off, repeating.

**To advance to Pasteurization:**
This is a manual step. Open Stage Parameters and SELECT "ADVANCE TO PASTEURIZATION." The system runs the ferm→past pump relay for 10 seconds, then switches to Stage 2.

**Status light:** YELLOW on during this stage.

---

### Stage 2 — Pasteurization

**Goal:** Heat the wine to 80°C to kill any remaining microorganisms before bottling.

**What happens automatically:**
1. PID controller targets 80°C using the pasteurization tank DS18B20.
2. 15-second hold once temperature is reached.
3. After the hold, the heater turns off. Pasteurization is complete.

**To complete the brew:**
Open Stage Parameters and SELECT "MARK BREW COMPLETE." The system records the end time and moves to the Brew Summary screen.

**Status light:** GREEN on during this stage.

---

### After the Brew

The **Brew Summary** screen shows:
- Total brew duration
- Time spent in each stage
- Final specific gravity and estimated ABV
- Final pH and weight
- Brew start timestamp

ABV is calculated as: `(Original Gravity - Final Gravity) × 131.25`

---

## 8. Stage Parameters

Access this screen by pressing RIGHT while in the Module View on the dashboard.

| Row | What you can change |
|-----|-------------------|
| SIM TEMP | Simulate a temperature reading: OFF / STATIC (fixed value) / DYNAMIC (auto-ramps) / MANUAL (UP/DOWN to adjust live) |
| TARGET TEMP | The temperature the PID controller aims for |
| TARGET PH | (Fermentation only) The pH threshold for the alert |
| TARGET GRAVITY | (Fermentation only) The gravity target for completion |
| ADVANCE / COMPLETE | Triggers the stage transition |

Press LEFT to return to the dashboard module view.

---

## 9. System Check Tests

Access from the Main Menu → SYSTEM CHECK. Ten items, navigated with UP/DOWN, entered with SELECT, exited with LEFT.

### 0. FAN TEST

Tests the pre-heating fan or fermentation fan relay.

1. SELECT the fan group you want to test (pre-heating or fermentation).
2. Press SELECT to turn the fan ON or OFF.
3. Use UP/DOWN to adjust fan speed (in 10% steps).
4. Press LEFT when done. The fan turns off automatically on exit.

---

### 1. LIGHT INDICATORS

Tests the three status lights (Red, Yellow, Green).

- UP/DOWN to select a light.
- SELECT to toggle it ON or OFF.
- Press LEFT to exit.

---

### 2. RELAY TEST

Automatically cycles through all 8 relay channels and the fan relay one by one, 500 milliseconds each. Watch the physical relays click and observe which one activates. No user input required during the test. Press LEFT to exit at any time.

---

### 3. MOTOR TEST

Sends motor commands to the Secondary ESP32 to test the fermentation mixer.

- UP/DOWN: increase or decrease speed in 25% steps.
- RIGHT: toggle direction (CW / CCW).
- SELECT: stop the motor (set speed to 0).
- LEFT: stop the motor and exit.

---

### 4. PID CONTROL

Tests the PID temperature controller for any one of the three tanks without running a full brew.

**Steps:**
1. Select the tank to test (Pre-Heat, Fermentation, or Pasteurization).
2. Press SELECT to enter the test screen.
3. Set the heat target (temperature to heat up to) and cool target (temperature to cool down to).
   - RIGHT cycles between the two targets.
   - UP/DOWN adjust the selected target by 1°C.
4. Press SELECT to start the test. The controller runs the heater and/or fan to reach the targets.
5. The test shows live temperature, the current control effort (heating % or cooling %), elapsed time, and status.
6. "SUCCESS — STABLE!" appears when the temperature stays within 0.5°C of the target for 15 consecutive seconds.
7. Press SELECT again to stop, or LEFT to exit.

---

### 5. HEATER OUTPUT

Tests each SSR heater directly at a set duty cycle, with no temperature feedback.

- UP/DOWN: switch between the three heater stages.
- LEFT/RIGHT: decrease or increase the duty cycle in 5% steps.
- SELECT: start or stop the heater.
- The display shows which GPIO and SSR are being driven.
- **Monitor the tank temperature with a thermometer during this test.** The heater will run until you stop it manually.

---

### 6. SD CARD VERIFY

Checks that the SD card can be written to and read from correctly.

- Press SELECT to run the test: it writes a small file, reads it back, and compares the content.
- Shows PASS or FAIL for both write and read.
- Press LEFT to exit.

---

### 7. UART MONITOR

Shows the live health of the serial link to the Secondary ESP32.

- **LINK:** OK (if a packet was received in the last 5 seconds) or NO SIGNAL.
- **Last packet:** how many seconds ago the last packet arrived.
- **Packets RX / Checksum errors:** total counts since boot.
- All incoming sensor values are shown live (temperature, pH, gravity, BLE status, battery, signal strength).
- This screen updates every second automatically.
- Press LEFT to exit.

---

### 8. SET RTC TIME

Sets the hour and minute on the DS3231 real-time clock.

- UP/DOWN: switch between HOUR and MINUTE fields.
- RIGHT: increment the selected field (hour wraps 0–23, minute wraps 0–59).
- SELECT: save the time to the RTC and exit.
- Press LEFT to exit without saving.

---

### 9. TRANSFER TEST

Tests the transfer pumps and flow sensors together.

The screen shows two paths — Pre-Heat → Ferm and Ferm → Past — each with a pump status tile and a flow measurement tile.

- UP/DOWN: switch between the two paths.
- SELECT: toggle the pump ON or OFF for the selected path.
- The flow tile shows liters measured by the flow sensor in real time (updates every second).
- RIGHT: enter the flow sensor calibration screen for the selected path.
- LEFT: turns off both pumps and returns to System Check.

> The flow reading depends on calibration. Before first use, run the calibration routine (see Section 10) so the liter display is accurate.

---

## 10. Flow Sensor Calibration

The flow sensors count electrical pulses from a spinning wheel inside the sensor body. To convert pulses to liters, the system needs a **K-factor** (pulses per liter). The default is 450 pulses/L (typical for a YF-S201 sensor), but the actual value varies with installation and flow rate. Calibration gets it accurate.

### How to calibrate

Access: Transfer Test → highlight the path you want → press RIGHT.

The calibration screen has three rows, navigated with UP/DOWN.

**Step 1 — Set the known volume (Row 0)**
Decide how much liquid you will flow through the sensor for calibration. A full liter is a good amount. Use LEFT/RIGHT to set this value (in 0.5 L steps, from 0.5 to 20 L).

**Step 2 — Reset the pulse counter (Row 1)**
Press SELECT on RESET PULSE COUNT. The live counter resets to zero and the liter display shows 0.000 L.

**Step 3 — Flow the liquid**
Go back to the Transfer Test (press LEFT), turn the pump on, and let exactly your chosen volume flow through the sensor and into a measured container. Then turn the pump off and return to the calibration screen.

**Step 4 — Capture (Row 2)**
Navigate to CAPTURE K-FACTOR and press SELECT. The system calculates: `K-factor = pulse count ÷ known volume` and saves it.

**Step 5 — Verify**
Run another known volume through and watch the liter display. It should match closely. If not, repeat the calibration.

**Notes:**
- Calibrate at the actual flow rate you use — the K-factor changes slightly at different flow speeds.
- The K-factor is stored in RAM only. You will need to recalibrate after each power cycle until SD card persistence is added.
- Sensor 1 (Pre-Heat → Ferm) is on GPIO 32. Sensor 2 (Ferm → Past) is on GPIO 34. Calibrate them separately.

---

## 11. Load Cell Calibration

The load cell measures the weight of liquid in the pre-heat tank and converts it to liters (1 kg ≈ 1 L for kaong sap).

Access: Main Menu → SENSOR VALUES → SELECT (goes to Load Cell page).

### Tare (Zero)

With the tank empty (or with an empty container on the scale), navigate to the TARE row and press SELECT. This sets the zero point. Any previous weight is ignored from this point.

### Calibration Factor

If the liter reading does not match the known volume:
- Navigate to the CAL FACTOR row.
- RIGHT increases the factor by 10.
- LEFT decreases the factor by 10.
- The live weight display updates immediately. Adjust until the display matches a known reference.

The default calibration factor is **23012.45**, measured with a 9-liter reference weight.

---

## 12. Sensor Monitor

Access: Main Menu → SENSOR VALUES.

Shows all nine sensor values updated every second:

| Row | Sensor | Source |
|-----|--------|--------|
| AMBIENT (PH) | Pre-heat ambient temperature | BME280 on Primary |
| LIQUID (PH) | Pre-heat liquid temperature | DS18B20 index 1 |
| AMBIENT (FERM) | Fermentation ambient temperature | BME280 on Secondary |
| LIQUID (FERM) | Fermentation liquid temperature | DS18B20 on Secondary |
| LIQUID (PST) | Pasteurization liquid temperature | DS18B20 index 0 |
| S. GRAVITY | Specific gravity | BLE hydrometer via Secondary |
| PH LEVEL | pH | PH4502C via ADS1115 on Secondary |
| EST. VOLUME | Liquid weight in pre-heat tank | HX711 load cell |
| RAW DEBUG | Button states and HX711 raw status | Internal |

Press SELECT to go to the Load Cell calibration page. Press LEFT to return to the Main Menu.

---

## 13. Web Interface

The system broadcasts a Wi-Fi access point and also attempts to join a local network on startup.

| Network | Details |
|---------|---------|
| Access Point SSID | `WineBrew_System` |
| AP Password | `12345678` |
| Local hostname | `winebrew.local` |

Open a browser and go to `http://winebrew.local` or the device's IP address to see the web dashboard.

The web page shows live sensor values and updates every second automatically:

| Field | Value |
|-------|-------|
| Volume | Current liquid weight (liters) |
| Local Ambient | BME280 temperature |
| Local Liquid | Pre-heat DS18B20 temperature |
| Ferm Ambient | Fermentation ambient temperature |
| Ferm Liquid | Fermentation liquid temperature |
| pH | Current pH reading |
| Specific Gravity | Current gravity from hydrometer |
| ABV | Estimated alcohol content (%) |

Firmware can also be updated over Wi-Fi (OTA) using the Arduino IDE or PlatformIO targeting `winebrew-main`.

---

## 14. Emergency Stop

The emergency stop button is on MCP23017 GPA6.

### First press — Confirmation

The display shows a warning screen: "EMERGENCY STOP — ARE YOU SURE?" All heaters, fans, and SSRs are cut off immediately. The system waits for confirmation.

- Press LEFT to cancel and resume normally (actuators restore to their previous state).
- Press ESTOP again to fully halt the system.

### Second press — System Halted

All relays open. The mixer stops. The PID controller stops. The display goes red with "SYSTEM HALTED."

The system is frozen — no brew logic, no logging, no sensor updates — until you reset it.

### Resuming after a halt

Hold the ESTOP button for 3 seconds. A progress bar fills on the screen. Release when it reaches 100%. The system restarts and returns to the Main Menu.

> Any in-progress brew is lost when the system halts. Stage progress is not saved to flash.

---

## 15. Status Lights

Three relay-driven indicator lights show the current brew stage at a glance.

| Light | Color | Active during |
|-------|-------|--------------|
| LIGHT_R (GPB7) | Red | Stage 0 — Pre-Heating |
| LIGHT_Y (GPB6) | Yellow | Stage 1 — Fermentation |
| LIGHT_G (GPB5) | Green | Stage 2 — Pasteurization |

All three lights turn on when a brew is complete.

---

## 16. Data Logging

The system logs sensor data to the SD card every 60 seconds while a brew is running.

**File:** `/data_log.csv`

**Columns logged:**

| Column | Description |
|--------|-------------|
| Date | YYYY/M/D from the RTC |
| Time | H:M:S from the RTC |
| LocalTemp | Pre-heat ambient (BME280) |
| LocalLiquid1 | Pre-heat liquid temperature (DS18B20) |
| LocalLiquid2 | Pasteurization liquid temperature (DS18B20) |
| RemoteTemp | Fermentation ambient temperature |
| RemoteLiquid | Fermentation liquid temperature |
| Gravity | Specific gravity (4 decimal places) |
| pH | pH value (2 decimal places) |
| ABV | Estimated ABV based on OG and current gravity |

Logging requires both the RTC and SD card to be working. If either fails, logging is skipped for that cycle. The last log time is shown in the dashboard header bar.

---

## Expected Volume Losses Per Batch

Based on winemaking literature applied to kaong sap (which is a clear liquid, not pulp, so gross lees are minimal):

| Stage | Estimated Loss |
|-------|---------------|
| Pre-heat sterilization (75°C, 20-30 min, open vessel) | 1.5–3% |
| Fermentation (CO2 off-gassing + yeast lees after racking) | 3–5% |
| Pasteurization (65-70°C, 20-30 min) | 1–2% |
| **Total from 20 L input** | **1.1–2.0 L lost** |
| **Expected output** | **~18–19 L** |

A yield below 80% of starting volume is considered abnormal and may indicate excessive evaporation, spillage, or a leak.
