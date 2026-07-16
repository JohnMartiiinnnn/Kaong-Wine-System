# Kaong Wine System — Component Test Cases

This document outlines all possible scenarios, expected behaviors, and failure states for every physical component within the Primary and Secondary ESP32 circuits.

## 1. Primary ESP32 Board (Dashboard & Controller)

| Component | Test Scenario | Action | Expected Behavior | Failure Mode |
| :--- | :--- | :--- | :--- | :--- |
| **TFT LCD & Touch** | Display initialization | Power on Primary ESP32 | Splash screen appears immediately | White screen (check `CS`/`DC` wiring) |
| | Touch calibration | Tap any button on the screen | Button highlights and executes action | No response / inverted touch (check `DIN`/`DO`/`CLK`) |
| **RTC (DS3231)** | Battery backup | Unplug ESP32 for 5 mins, turn back on | System clock retains correct time | Time resets to default (replace coin cell battery) |
| **SD Card Module** | Data logging | Complete a brew cycle wizard | `data_log.csv` is created/updated on the card | UI crashes / logs fail (check `MISO`/`MOSI`/`CS`) |
| **MCP23017 IO Expander**| I2C communication | Navigate to System Check menu | Relays and LED options are accessible | ESP32 halts/reboots instantly (check `SDA`/`SCL`) |
| **Physical Keypad** | Hardware input | Press Left, Right, Up, Down, Select | Menu cursor moves corresponding to input | Buttons bounce/double-press (check internal pullups) |
| **E-Stop Button** | Emergency override | Press E-Stop while fans and motor are running | ALL relays cut out, Motor stops, UI locks | System ignores button (check `GPA6` connection to MCP) |
| **HX711 Load Cell** | Tare & Calibrate | Place empty vessel, hit Tare | Display reads `0.0g` exactly | Constant heavy drifting (check shielding/ground loop) |
| | Scale Response | Add 100g weight | Display rapidly changes to `100.0g` | Reads `FAILED` or `NAN` (check `DT`/`SCK` pins) |
| **SSR Heaters** | Slow PWM Control | Turn on heater outputs | SSR indicator LED lights up, tank starts heating | No heating (check GPIO 13/12/14 wiring, SSR power) |
| **Flow Sensors** | Interrupt Pulses | Pump liquid through sensor 1 or 2 | Dashboard/Calib Wizard shows live pulse counts increasing | Pulse count stays at 0 (check GPIO 32/34 wiring, pull-ups) |
| **Calibration Status LED** | Visual signaling | Run Calibration Wizard | Onboard LED (GPIO 2) blinks (1Hz/5Hz) and lights up solid | LED remains off (check GPIO 2 connection / orientation) |

---

## 2. Secondary ESP32 Board (Sensor Node)

| Component | Test Scenario | Action | Expected Behavior | Failure Mode |
| :--- | :--- | :--- | :--- | :--- |
| **BME280 / BMP280** | Ambient Data | Blow gently on the sensor | Temp & Humidity rise on the Dashboard | Reads `FAILED` on UI (check I2C 3.3V power) |
| **DS18B20 Temp Probe** | Submersion | Hold the stainless probe in your hand | Liquid temp slowly climbs to body temp ~36°C | Reads `-127.0` (missing 4.7kΩ pullup resistor) |
| **ADS1115 (pH Sensor)**| Standard reading | Place pH probe in neutral tap water | Dashboard shows pH value around 7.0 | Dashboard says `FAILED` (check if powered by 5V instead of 3.3V) |
| | Buffer calibration | Place in pH 4.0 buffer solution | Readings stabilize (adjust hardware offset if needed) | Readings fluctuate wildly (probe is dry/damaged) |
| **NimBLE (RAPT Pill)** | Bluetooth Decoding | Turn on RAPT Pill near the tank | Telemetry (Temp/SG/Bat) updates on Dashboard | Stays `FAILED` (RAPT Pill asleep or out of range) |
| **UART Comm Link** | Disconnect | Unplug the TX/RX wires between ESP32s | Secondary sensor tiles freeze, eventually timeout | Data loss / Garbage characters (missing common GND) |

---

## 3. High-Voltage Actuators & Relays

> [!WARNING]
> Use extreme caution when testing mains power and high-current 12V/24V sources.

| Component | Test Scenario | Action | Expected Behavior | Failure Mode |
| :--- | :--- | :--- | :--- | :--- |
| **BTS7960 Motor Driver**| Speed & Direction | Navigate to Motor Test Menu, set 50% CW | Motor spins quietly at half speed clockwise; current draw matches load | Whining without movement (Check 12V supply / motor connections) |
| | Current-to-RPM | Measure current draw voltage on ADS1115 A1 under load | Voltage ranges from 0.035V (no-load, ~66 RPM) to 0.447V (stall, 0 RPM) | Always reads 0V (check IS pins wiring / pull-down resistor) |
| | E-Stop Cutoff | Hit E-Stop while motor is running | PWM cuts to 0, Motor freewheels to a stop | Motor ignores E-stop (Check `L_EN`/`R_EN` physical wiring) |
| **Pre-Heat Fan Relay** | Trigger | Navigate to Fan Test Menu, turn Fan 1 ON | Relay clicks, fan spins up | No click (Check MCP `GPA7` wiring to Relay IN) |
| **Fermentation Fans** | Trigger | Navigate to Fan Test Menu, turn Fan 2 ON | Relays 2 & 3 click simultaneously, dual fans spin | Fans stutter (Insufficient 12V power supply current) |
| **LED Status Lights** | Trigger | Navigate to Light Test Menu, toggle R/Y/G | Tower lights illuminate | Lights very dim (missing 12V/24V supply to the light tower) |

---

## 4. Mixing Impeller (JGB37-545 1260) Current Draw vs. Speed Calibration Chart

The RPM of the 12V JGB37-545 1260 motor (with 90 reduction ratio) can be calculated by measuring the output voltage on the BTS7960's current sense (`IS`) pins. These pins are tied together, connected to **ADS1115 Channel A1** (Secondary ESP32) with a **1kΩ pull-down resistor** to GND.

The motor speed drops linearly as the load and current draw increase:

| Motor Torque Load | Motor Current (A) | Sense Output Voltage (V) | Estimated Speed (RPM) | Operational State |
| :--- | :--- | :--- | :--- | :--- |
| **No Load** | 0.30 A | 0.035 V | ~66 RPM | Free-spinning must |
| **Light Load** | 1.00 A | 0.118 V | ~50 RPM | Rated continuous mixing (rated load) |
| **Medium Load** | 1.80 A | 0.212 V | ~38 RPM | Normal fermentation viscosity |
| **Heavy Load** | 2.60 A | 0.306 V | ~23 RPM | High-density must resistance |
| **Straining** | 3.20 A | 0.376 V | ~11 RPM | Approaching motor capacity limit |
| **Stall** | 3.80 A | 0.447 V | 0 RPM | Stalled (Power cutoff required) |

> [!IMPORTANT]
> **Stall Limit Protection:** If the voltage on ADS1115 A1 exceeds **0.353V (3.0A)** for more than 3 consecutive seconds, the primary controller must automatically shut down the motor to prevent the gearbox gears from stripping or the motor winding from burning out.
