# Sensor Calibration Guide — Automated Kaong Wine Brewing System

Before running formal unit testing and starting a production brew, all analog and digital sensors must be calibrated. This guide explains how to use the standalone calibration utilities for the Primary ESP32 and Secondary ESP32.

---

## 1. Primary ESP32 Calibration

The Primary ESP32 controls the **Load Cell (Scale)**, **DS18B20 Temp Probes (Pre-heat & Pasteurization)**, and **Flow Sensors (1 & 2)**.

### How to Flash
From the root directory `/Users/gian/Coding/Kaong-Wine-System`, compile and upload the calibration target:
```bash
~/.platformio/penv/bin/pio run -e calibration -t upload
```
Open the Serial Monitor at **115200** baud to view raw readouts and enter commands:
```bash
~/.platformio/penv/bin/pio device monitor
```

### 1.1 Load Cell (HX711) Calibration
The Load Cell measures volume/weight. It requires taring (offset) and finding the calibration factor (gain).
1.  **Tare the scale**: Empty the scale platform completely. In the Serial Monitor, type `t` and press Enter. The monitor will print:
    `Tare Finished! Offset Value: XXXXX`
2.  **Calibrate scale factor**:
    *   Place a known weight (e.g., a 1.0 kg calibrated weight or 1 Liter bottle of water) on the scale platform.
    *   In the Serial Monitor, type `c` and press Enter.
    *   Type the weight of the object in grams (e.g., `1000`) when prompted, and press Enter.
    *   The utility will calculate the calibration factor:
        `Calculated Calibration Factor: Y.YYYY`
3.  **Update code**: Open `src/config.h` and update the `calibrationFactor` constant:
    ```cpp
    const float calibrationFactor = Y.YYYY; // Replace with your calculated factor
    ```

### 1.2 DS18B20 Liquid Temperature Probes
The Primary ESP32 reads two probes on Pin 26: Probe 0 (Pasteurization) and Probe 1 (Pre-heat).
*   The calibration utility automatically prints readings from both probes every 2 seconds.
*   **Verification**: Immerse both probes in ice water (should read ~0.0°C) and boiling water (should read ~100.0°C). 
*   *Note: If a probe is disconnected or has a bad pull-up resistor, it will read `85.0°C` or `-127.0°C`.*

### 1.3 Flow Sensors (Flow 1 & Flow 2)
The Flow Sensors measure liquid transferred between chambers.
1.  In the Serial Monitor, type `r` and press Enter to reset both pulse counters to `0`.
2.  Run exactly 1.0 Liter of water through Flow Sensor 1.
3.  Record the pulse count printed in the Serial Monitor (e.g., `580` pulses).
4.  Calculate your K-factor: `K-factor = pulses / Volume (L)` (e.g., `580 / 1.0 = 580`).
5.  Repeat the same process for Flow Sensor 2.
6.  **Update code**: Open `src/config.h` and update the sensor K-factor constants:
    ```cpp
    const float FLOW1_KF = 580.0f; // Replace with Flow 1 pulses/L
    const float FLOW2_KF = 580.0f; // Replace with Flow 2 pulses/L
    ```

---

## 2. Secondary ESP32 Calibration

The Secondary ESP32 transmitter controls the **pH Sensor** and the **Fermentation Liquid Temp Probe**. Because it is mounted inside an enclosure, its USB port is inaccessible. You will calibrate it through the Primary ESP32 using the **UART Bridge Mode**.

### How to Flash
Since the USB port is inaccessible, you must flash the Secondary ESP32 over-the-air (OTA) or use a pre-flashed calibration image.
```bash
cd /Users/gian/Coding/Kaong-Wine-System/Secondary_Transmitter
~/.platformio/penv/bin/pio run -e calibration -t upload --upload-port <IP_ADDRESS_OF_SECONDARY>
```
*(If OTA is not yet configured or the enclosure is open, you may plug it in directly to flash once.)*

### Entering UART Bridge Mode
1. Ensure the **Primary ESP32** is flashed with the calibration utility and plugged into your PC via USB.
2. Open the Primary ESP32's Serial Monitor at **115200** baud:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System
   ~/.platformio/penv/bin/pio device monitor
   ```
3. Type `s` and press Enter. The monitor will display:
   `ENTERING SECONDARY ESP32 BRIDGE MODE`
   *You are now communicating directly with the enclosed Secondary ESP32 via UART.*

### 2.1 pH Sensor (ADS1115 + PH4502C) Calibration
pH calibration uses a standard 2-point buffer method (pH 7.0 neutral buffer and pH 4.0 acid buffer) with temperature compensation.
1.  **Calibrate pH 7.0 (Offset)**:
    *   Submerge the pH probe in a **pH 7.0 buffer solution**.
    *   In the Serial Monitor, type `7` and press Enter.
    *   Wait 5 seconds. The utility will print the average voltage:
        `New Offset Voltage (pH 7.0): Z.ZZZZ V`
    *   **Update code**: Open `Secondary_Transmitter/src/main.cpp` and update the offset constant in the pH reading section:
        ```cpp
        txData.phValue = 7.0 - (voltage - Z.ZZZZ) / slope_V_pH; // Replace 2.555 with Z.ZZZZ
        ```
2.  **Calibrate pH 4.0 (Slope)**:
    *   Submerge the pH probe in a **pH 4.0 buffer solution**.
    *   In the Serial Monitor, type `4` and press Enter.
    *   Wait 5 seconds. The utility will calculate the temperature-adjusted base slope:
        `Calculated Base Slope at 25C: W.WWWWW V/pH`
    *   **Update code**: Open `Secondary_Transmitter/src/main.cpp` and update the slope coefficient:
        ```cpp
        float slope_V_pH = W.WWWWW * (tempC + 273.15) / 298.15; // Replace 0.17126 with W.WWWWW
        ```

### 2.2 Fermentation Liquid Temp Probe
*   While in Bridge Mode, the utility automatically reads and prints the fermentation probe temperature every 2 seconds.
*   Verify accuracy in ice water (~0°C) and warm water.

### Exiting Bridge Mode
Type `p` and press Enter to return to Primary ESP32 Calibration Mode.

---

## 3. Returning to Main System Code

After updating all calibration coefficients, flash the main firmware back to both boards:

*   **Primary Controller** (via USB):
    ```bash
    cd /Users/gian/Coding/Kaong-Wine-System
    ~/.platformio/penv/bin/pio run -e esp32dev -t upload
    ```
*   **Secondary Controller** (via OTA):
    ```bash
    cd /Users/gian/Coding/Kaong-Wine-System/Secondary_Transmitter
    ~/.platformio/penv/bin/pio run -e esp32dev -t upload --upload-port <IP_ADDRESS_OF_SECONDARY>
    ```
