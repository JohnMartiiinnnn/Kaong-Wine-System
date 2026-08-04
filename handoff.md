# Session Handoff & Progress Report

**Date:** July 31, 2026 / August 4, 2026  
**Repository:** `Kaong-Wine-System` (`/Users/gian/Coding/Kaong-Wine-System/`)  
**Branch:** `main`  
**Latest Commit:** `b35089a`  

---

## 1. Goal
To refine, debug, and integrate hardware and software subsystems for the **Automated Wine Brewing System thesis project**, including:
* Relay Test & Fan PWM driving logic.
* PID Thermal Tracking interface optimization, metrics layout, and graph plotting.
* DRV8871 H-Bridge + ESP32-C3 Yeast Dispenser module integration.
* Menu UI navigation fixes (Load Cell return path) and motor driver telemetry cleanup.

---

## 2. Current State
* **Firmware Build Status:** **SUCCESS** — Compiles cleanly via PlatformIO (`pio run -e esp32dev` for Primary, `pio run -d Secondary_Transmitter` for Secondary).
* **Git Status:** Clean tree, all edits committed and pushed to GitHub `main`.
* **Hardware Integration:**
  * **Preheat Fan Relay Test:** Fixed using `setRelayTestChannel(idx, state)` to drive MCP relays and PWM speed in sync.
  * **PID Thermal Tracking:** Ambient temperature (`room2Temp`) is plotted on the trend line graph, while both Ambient and Liquid (`getFermTemp()`) temperatures are rendered in a 3-row non-overlapping metrics card.
  * **Yeast Dispenser Module:** Implemented `YeastDispenser.h/.cpp` with a 50% PWM duty cap (`128/255` = ~6.0V max output limit) for the 6V N20 motor on an 11.93V supply, 1 kHz Fast-Decay mode, 200ms active braking (`IN1=HIGH, IN2=HIGH`), and dosage calibration math ($\text{msPerGram} = \text{testMs} / \text{weighedGrams}$).
  * **Load Cell Navigation:** Fixed `LOAD_CELL_PAGE` return path via `prevLoadCellState` so returning from Settings returns to `SETTINGS_MENU`.
  * **Motor Test & Telemetry:** Removed current-sense stall protection loop (which was resetting `motorTestSpeed` to 0) and removed current/RPM text overlays.

---

## 3. Active & Key Files

| File Path | Description |
|:---|:---|
| [src/config.h](file:///Users/gian/Coding/Kaong-Wine-System/src/config.h) | Central pin constants, struct declarations, `prevLoadCellState`, and Yeast Dispenser parameters. |
| [src/main.cpp](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp) | State machine, button handlers, PID tracking loop, motor commands, and state transition logic. |
| [src/display.h](file:///Users/gian/Coding/Kaong-Wine-System/src/display.h) / [src/display.cpp](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp) | TFT LCD rendering (`drawPidTrackingMenu`, `drawMotorTestMenu`, `drawMixerMenu`, `drawLoadCellPage`). |
| [src/YeastDispenser.h](file:///Users/gian/Coding/Kaong-Wine-System/src/YeastDispenser.h) / [src/YeastDispenser.cpp](file:///Users/gian/Coding/Kaong-Wine-System/src/YeastDispenser.cpp) | Dedicated Yeast Dispenser module for Primary Controller. |
| [Secondary_Transmitter/src/YeastDispenser.h](file:///Users/gian/Coding/Kaong-Wine-System/Secondary_Transmitter/src/YeastDispenser.h) / [.cpp](file:///Users/gian/Coding/Kaong-Wine-System/Secondary_Transmitter/src/YeastDispenser.cpp) | Yeast Dispenser module for Secondary ESP32-C3 Controller. |
| [AGENTS.md](file:///Users/gian/Coding/Kaong-Wine-System/AGENTS.md) / [CLAUDE.md](file:///Users/gian/Coding/Kaong-Wine-System/CLAUDE.md) | Unified project architecture and system rules (Section 11 added for Yeast Dispenser). |
| [YEAST_DISPENSER_GUIDE.md](file:///Users/gian/Coding/Kaong-Wine-System/YEAST_DISPENSER_GUIDE.md) | Complete hardware specs, pinout, API documentation, and calibration steps. |

---

## 4. Changes Made

1. **Preheat Fan Relay Driving (`src/main.cpp` & `src/display.cpp`):**
   * Created `setRelayTestChannel(idx, state)` helper function.
   * Fixed preheat fan inverted PWM mapping (`isFanOn = true; setFanSpeed(100);`) so preheat fan spins at 100% speed when toggled in Relay Test mode.
   * Removed header warning banner (`! PUMPS + FANS WILL ENERGIZE !`) and aligned 9 relay tiles starting at $Y=60$.

2. **PID Thermal Tracking & Fermentation Interface (`src/display.cpp` & `src/main.cpp`):**
   * Removed `RISE TIME` and `SETTLE TIME` fields.
   * Expanded graph canvas height to 215px ($Y=205$ to $420$).
   * Configured Fermentation PID tracking to plot **Ambient Temperature (`room2Temp`) as the only curve on the graph**.
   * Structured the metrics card into a 3-row layout:
     * **Row 1:** `AMB: xx.xC / xx.xC` \| `ERR: xx.xxC`
     * **Row 2:** `LIQUID: xx.xC` \| `STABILITY: STATUS`
     * **Row 3:** `POWER: xx%` \| `OVERSHOOT: +xx.xC`
   * Eliminated label text collisions between `HEATER POWER` and `TARGET`.

3. **Automated Yeast Dispenser Module (`src/YeastDispenser.h/.cpp` & `Secondary_Transmitter/`):**
   * DRV8871 IN1 → ESP32-C3 **GPIO 4**, IN2 → **GPIO 5**.
   * Voltage protection cap: `MAX_YEAST_MOTOR_DUTY = 128` (50% max duty cap of 255) for ~6.0V output on 11.93V supply.
   * PWM architecture: 1 kHz Fast-Decay mode (`IN1=PWM, IN2=0`).
   * Active electric brake pulse: `IN1=HIGH, IN2=HIGH` for 200ms at cycle completion to prevent overrun/drips.
   * Calibration math: `calculateMsPerGram(testDurationMs, weighedGrams)`.

4. **Menu Navigation & Telemetry Cleanup (`src/main.cpp` & `src/display.cpp`):**
   * Added `prevLoadCellState` variable. Returning from `LOAD_CELL_PAGE` now goes back to `SETTINGS_MENU` when opened from Settings.
   * Removed current-sense stall protection loop from `src/main.cpp` that was resetting `motorTestSpeed` back to 0 after 3 seconds.
   * Removed `currentA` and `rpmVal` telemetry text overlays from `drawMotorTestMenu()` and `drawMixerMenu()`.

---

## 5. Failed Attempts & Solutions

* **Serial Upload Target Port Failure:**  
  * *Issue:* `pio run -t upload` auto-detected `/dev/cu.Bluetooth-Incoming-Port` and failed with connection timeout.  
  * *Solution:* Explicitly pass upload port parameter:
    ```bash
    ~/.platformio/penv/bin/pio run -t upload --upload-port /dev/cu.usbserial-0001
    ```
* **Preheat Fan Not Spinning in Relay Test:**  
  * *Issue:* Toggling `FAN_RELAY_PIN` (GPA7) turned on the relay coil, but because Preheat Fan uses inverted PWM mapping, 0 duty cycle output held the fan at 0 RPM.  
  * *Solution:* Built `setRelayTestChannel()` to enable `isFanOn = true; setFanSpeed(100);` simultaneously with the relay pin.
* **Motor Test Speed Instantly Resetting to 0:**  
  * *Issue:* A stall-protection check reading `incomingData.motorSenseVolts > 0.353f` triggered after 3 seconds and called `motorTestSpeed = 0`.  
  * *Solution:* Removed the uncalibrated current sense stall protection loop.
* **Metrics Card Text Collision:**  
  * *Issue:* Displaying `TARGET: FERMENTATION (LIQUID)` on the right collided with `HEATER POWER: 100%` on the left.  
  * *Solution:* Separated metrics into a clean 3-row layout with shorter labels (`AMB`, `LIQUID`, `POWER`, `OVERSHOOT`).

---

## 6. Next Steps
1. **Physical Scale Calibration for Yeast Dispenser:**  
   Run `dispenseYeastDuration(5000)` on the secondary controller, weigh the output on a scale, and call `calculateMsPerGram(5000, weighedGrams)` to calibrate `msPerGramYeast`.
2. **UART Motor Command Testing:**  
   Verify serial execution of yeast dispenser and mixer motor commands sent from Primary ESP32 to Secondary ESP32-C3 during automated brew execution.
3. **Chamber PID Step Response Test:**  
   Run a PID thermal tracking test for the Fermentation chamber to log ambient step response to `/pid_MMDD_HHMM.csv` and verify thermal stability.

---

*Handoff document saved to `handoff.md`.*
