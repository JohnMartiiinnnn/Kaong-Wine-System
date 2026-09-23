# Sensor Calibration Guide — Automated Kaong Wine Brewing System

Before starting a production brew, all analog and digital sensors must be calibrated. To keep the main system codebase clean, calibration code for each sensor on the Primary ESP32 has been separated into individual standalone PlatformIO projects in the root directory.

---

## 1. Load Cell (HX711) Calibration & Dedicated Tare Utility

The system provides two methods to calibrate and zero the HX711 scale: directly on the Primary ESP32 TFT screen, or via the standalone calibration project.

### Method A: On-Device TFT Calibration & Tare Screen (Recommended)
1. **Access Dedicated Screen**: Navigate to **Main Menu → SETTINGS → TARE LOAD CELL (Item 7)**.
2. **Tare Scale (Zero Reference)**:
   - Select **TARE ZERO (Row 0)** and press **SELECT**.
   - The UI runs a 3-second non-blocking countdown (`"TARING IN 3s..."` → `"1s"`) with an orange indicator.
   - Takes 10 hardware samples to zero the HX711 baseline.
   - Displays bright green **`"TARED SUCCESSFUL!" [ OK ]`** for 2 seconds upon completion.
3. **Static Container Offset (Optional)**:
   - Select **OFFSET (Row 1)** and press **SELECT** to enter edit mode.
   - Adjust offset from `0.00 kg` to `15.00 kg` using **LEFT / RIGHT** (±0.10 kg) or **UP / DOWN** (±0.50 kg).
   - Press **SELECT** or **LEFT (Return)** to save. All offsets automatically persist in NVS flash (`"wb_config"` namespace).
4. **Adjust Calibration Factor**:
   - Navigate to **Main Menu → SENSOR VALUES → SELECT** (`CALIBRATION_MODE`).
   - Select **CAL. FACTOR (Row 1)** and press **SELECT**.
   - Press **RIGHT** (+10.0) or **LEFT** (-10.0) while observing the live reading against a known test mass. All adjustments save directly to NVS flash (`"wb_config"`).

### Method B: Standalone PlatformIO Calibration Project
1. Navigate to directory: `cd LoadCell_Calibration`
2. Flash and monitor: `pio run -t upload && pio device monitor`
3. Tare: Type `t` and press Enter.
4. Calibrate: Type `c`, place reference mass (e.g. 1000g), and input weight.
5. Save: Update `calibrationFactor` in `src/config.h` (or adjust via TFT screen).

---

## 2. Flow Sensor Calibration

This standalone project measures pulses and calculates K-factors for Flow Sensor 1 and Flow Sensor 2. It provides a visual TFT interface and allows you to toggle the transfer pumps directly using the keypad or Serial Monitor.

### How to Run
1. Navigate to the directory:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System/FlowSensor_Calibration
   ```
2. Flash the firmware and open the Serial Monitor:
   ```bash
   ~/.platformio/penv/bin/pio run -t upload
   ~/.platformio/penv/bin/pio device monitor
   ```

### Calibration Procedure
1. **Prepare Setup**: Place a container with an exactly measured volume of water (e.g., exactly `1.0 L` or `5.0 L`) at the inlet of the pump/sensor path, and an empty container at the outlet.
2. **Set Target Volume**: Use the **UP / DOWN** keypad keys to select the **Target Volume** row on the screen. Use the **LEFT / RIGHT** keys to adjust the volume to match your measured water volume.
3. **Reset Counters**: Navigate to **Reset Counters to 0** and press **SELECT** to clear any old pulses.
4. **Run Pump**:
   - Navigate to **Pump 1 (PH->Ferm)** (or **Pump 2 (Ferm->Past)** depending on which sensor you are calibrating).
   - Press **SELECT** to turn the pump ON.
   - Let the pump run until all the measured water has passed through the sensor, then press **SELECT** again to turn the pump OFF.
5. **Calculate K-Factor**: Navigate down to **Calculate K-Factor** and press **SELECT**. The screen (and Serial Monitor) will instantly calculate and display the new K-Factor.
6. **Update Code**: Open `src/config.h` in the main codebase and update `FLOW1_KF` or `FLOW2_KF` with the computed K-Factor.

---

## 3. DS18B20 Temperature Probes Verification

This standalone project provides a visual TFT interface to read local OneWire temp probes on Pin 26 (Pre-heat and Pasteurization) as well as the remote fermentation probe via UART.

### How to Run
1. Navigate to the directory:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System/DS18B20_Calibration
   ```
2. Flash the firmware:
   ```bash
   ~/.platformio/penv/bin/pio run -t upload
   ```
3. Use the keypad **UP / DOWN** arrows to select which probe you want to view on the screen:
   * **Pre-heat (Index 0)**
   * **Pasteurization (Index 1)**
   * **Fermentation (Remote UART)**
4. **Verification**: Verify the readings in ice water (~0.0°C) or room temperature against a reference thermometer. If the readings are offset, you can add software offsets to `preheatTempOffset`, `pastTempOffset`, and `fermTempOffset` in the main `src/config.h` file.

---

## 4. pH Sensor Calibration (UART Bridge)

Because the Secondary ESP32 is inside the fermentation enclosure, its USB port is inaccessible. The `pHSensor_Calibration` project runs a serial bridge on the Primary ESP32 to route command/response data bi-directionally over UART (`Serial2` pins 16 & 17) to the Secondary.

### Flashing the Secondary MCU
Before using the bridge, the Secondary ESP32 must be running its calibration utility. Since it's enclosed, use OTA or ensure the correct environment is uploaded if connected via USB:
1. Navigate to the transmitter directory: `cd /Users/gian/Coding/Kaong-Wine-System/Secondary_Transmitter`
2. Flash **only** the calibration environment:
   ```bash
   ~/.platformio/penv/bin/pio run -e calibration -t upload
   ```
   *(Note: when done with pH calibration, run `~/.platformio/penv/bin/pio run -e esp32dev -t upload` to restore production firmware).*

### How to Run
1. Ensure the Secondary ESP32 has its calibration code running (see above).
2. Navigate to the bridge directory:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System/pHSensor_Calibration
   ```
3. Flash and open the monitor:
   ```bash
   ~/.platformio/penv/bin/pio run -t upload
   ~/.platformio/penv/bin/pio device monitor
   ```
4. You will immediately start seeing the Secondary's live pH readings streamed to your monitor.
5. **Calibrate pH 7.0**: Place the pH probe in a 7.0 buffer, type `7`, and press Enter. Wait for the countdown. Update the pH offset voltage constant in `Secondary_Transmitter/src/main.cpp`.
6. **Calibrate pH 4.0**: Place the pH probe in a 4.0 buffer, type `4`, and press Enter. Wait for the countdown. Update the pH slope constant in `Secondary_Transmitter/src/main.cpp`.

---

## 5. Returning to Main System Code

After completing all calibrations, navigate back to the root directory and flash the main production firmware:

```bash
cd /Users/gian/Coding/Kaong-Wine-System
~/.platformio/penv/bin/pio run -e esp32dev -t upload
```
