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
| **AC Dimmers (TRIAC)** | Zero-Cross Interrupt | Power on AC dimmer board | ESP32 triggers heating pulses successfully | Lights flicker wildly (Zero-cross pin `32` not connected) |

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
| **BTS7960 Motor Driver**| Forward / Reverse | Navigate to Motor Test Menu, set 50% CW | Motor spins quietly at half speed clockwise | Whining sound without movement (Increase power / check 12V supply) |
| | E-Stop Cutoff | Hit E-Stop while motor is running | PWM cuts to 0, Motor freewheels to a stop | Motor ignores E-stop (Check `L_EN`/`R_EN` physical wiring) |
| **Pre-Heat Fan Relay** | Trigger | Navigate to Fan Test Menu, turn Fan 1 ON | Relay clicks, fan spins up | No click (Check MCP `GPA7` wiring to Relay IN) |
| **Fermentation Fans** | Trigger | Navigate to Fan Test Menu, turn Fan 2 ON | Relays 2 & 3 click simultaneously, dual fans spin | Fans stutter (Insufficient 12V power supply current) |
| **LED Status Lights** | Trigger | Navigate to Light Test Menu, toggle R/Y/G | Tower lights illuminate | Lights very dim (missing 12V/24V supply to the light tower) |
