# Automated Yeast Dispenser & System Integration Guide

This guide documents the hardware specifications, DRV8871 motor driver configuration, Yeast Dispenser API, calibration procedure, and recent PID/relay fixes for the **Kaong Wine System**.

---

## 1. Yeast Dispenser Hardware Specifications

| Parameter | Specification | Notes |
|:---|:---|:---|
| **Microcontroller** | ESP32-C3 SuperMini (Secondary MCU) | Drives DRV8871 IN1/IN2 pins |
| **Motor Driver** | Texas Instruments DRV8871 H-Bridge | Fast-Decay PWM control with active electric brake |
| **Motor** | N20 6V DC Geared Motor | Coil resistance ~32 Ω, rated voltage 6.0V |
| **Power Supply** | 11.93 V DC | Connected to DRV8871 `VM` and `GND` |
| **Common Ground** | Unified Ground Rail | ESP32-C3 GND, Primary ESP32 GND, DRV8871 GND, and 11.93V Supply (-) physically tied |
| **PWM Duty Cap** | `MAX_YEAST_MOTOR_DUTY = 128` | **50% max duty cap** (`128/255`), limiting peak average output voltage to **~6.0V** |
| **PWM Frequency** | 1 kHz | Fast-Decay mode for maximum low-speed startup torque |

---

## 2. Pin Mapping & Configuration

### ESP32-C3 Pin Assignments
* `DRV8871 IN1` → **GPIO 4** (PWM channel 4)
* `DRV8871 IN2` → **GPIO 5** (PWM channel 5)

### System Configuration (`src/config.h`)
```cpp
const int DRV8871_IN1_PIN_CFG = 4;
const int DRV8871_IN2_PIN_CFG = 5;
const uint8_t MAX_YEAST_MOTOR_DUTY = 128;       // 50% duty cap (6.0V max for 6V motor on 11.93V rail)
const float DEFAULT_MS_PER_GRAM_YEAST = 1764.0f; // Empirical calibration fit: 0.567 g/s (1764 ms/g, R2=0.98)
const uint32_t YEAST_DISPENSE_TIMEOUT_MS = 30000UL; // Safety cutoff timeout
const uint16_t YEAST_BRAKE_DURATION_MS = 200;   // Active braking duration
```

---

## 3. Software API (`YeastDispenser.h` / `YeastDispenser.cpp`)

The Yeast Dispenser module is available on both the Primary ESP32 (`src/`) and Secondary ESP32-C3 (`Secondary_Transmitter/src/`):

### Core Functions
* `void initYeastDispenser()`
  Initializes GPIO 4 & 5, configures 1 kHz LEDC PWM timers, and sets the driver to idle state.

* `void dispenseYeastDuration(uint32_t durationMs)`
  Drives the N20 motor forward at 50% PWM duty cap (`128/255`) in Fast-Decay mode for `durationMs`. At the end of the duration, it executes an **active brake pulse** (`IN1=HIGH, IN2=HIGH` for 200ms) to stop motor rotation instantly and prevent over-dispensing or drip.

* `void dispenseYeastGrams(float grams)`
  Calculates required duration $\text{durationMs} = \text{grams} \times \text{msPerGramYeast}$ and executes `dispenseYeastDuration(durationMs)`.

* `void stopYeastDispenser()`
  Immediately applies active braking (`IN1=HIGH, IN2=HIGH` for 200ms) and returns pins to idle (`IN1=LOW, IN2=LOW`).

* `float calculateMsPerGram(float testDurationMs, float weighedGrams)`
  Calculates and updates the saved calibration factor:
  $$\text{msPerGramYeast} = \frac{\text{testDurationMs}}{\text{weighedGrams}}$$

---

## 4. Empirical Calibration & Fermentation Pitch Mixing

### Empirical Calibration Model
From 21 bench trials across pulse durations of 3s to 25s:
* **Flow Rate**: $0.567\text{ g/s}$ ($\approx 1764.0\text{ ms/g}$)
* **Linear Fit ($R^2 = 0.98$)**: $\text{Output (g)} = 0.567 \times \text{Time (s)}$

### Proportional Pitch Mixing
When transitioning into the Fermentation stage (`activeBrewStage == 1`), the system runs the mixing impeller for **10 seconds per 1 gram** of dispensed yeast:
$$\text{Mixing Duration} = \text{Yeast Grams} \times 10.0\text{ s}$$
* **3.0g dispensed**: Mixer runs for **30 seconds**.
* **5.0g dispensed**: Mixer runs for **50 seconds**.
After completing initial pitch mixing, it resumes the standard periodic cycle (5 minutes ON / 355 minutes OFF).
5. Perform a verification dose:
   ```cpp
   dispenseYeastGrams(5.0f); // Dispenses exactly 5.0 grams of yeast
   ```

---

## 5. PID Thermal Tracking & Fermentation Interface System Rules

### Quartz Heater Duty Cycle Safety Rule
* **Quartz Radiant Heater (`SSR_FERM`, pin 12):** Pulse duration is capped at a maximum of **1.5 seconds (1500ms)** per window to prevent thermal shock, insulation damage, or fire retardant tape melting.

### PID Thermal Tracking Interface Layout
* **Graph Plotting Rule:** For Fermentation PID control, **Ambient Temperature (`room2Temp`) is the ONLY temperature plotted on the trend line graph**.
* **Metrics Card Display:** The card above the graph displays **both Ambient Temperature (`AMB: xx.xC / xx.xC`) and Liquid Probe Temperature (`LIQUID: xx.xC`)**.
* **3-Row Non-Overlapping Grid:**
  * **Row 1:** `AMB: xx.xC / xx.xC` (Left) \| `ERR: xx.xxC` (Right)
  * **Row 2:** `LIQUID: xx.xC` (Left) \| `STABILITY: STATUS` (Right, color-coded)
  * **Row 3:** `POWER: xx%` (Left) \| `OVERSHOOT: +xx.xC` (Right)

### Fan Relay Test Driving Rule
* **Preheat Fan Inverted PWM Requirement:** All fan/relay manual test functions must use `setRelayTestChannel(idx, state)` to drive MCP relay pins and PWM speed signals synchronously. Preheat fan uses inverted PWM (`isFanOn = true; setFanSpeed(100);` maps to 0 LEDC duty = 100% speed), while Fermentation fan uses non-inverted PWM (`isFermFanOn = true; setFanSpeed(100);` maps to 255 LEDC duty = 100% speed).

---

*Last Updated: July 31, 2026*
