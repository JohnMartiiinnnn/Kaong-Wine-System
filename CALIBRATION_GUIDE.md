# Sensor Calibration Guide — Automated Kaong Wine Brewing System

Before starting a production brew, all analog and digital sensors must be calibrated. To keep the main system codebase clean, calibration code for each sensor on the Primary ESP32 has been separated into individual standalone PlatformIO projects in the root directory.

---

## 1. Load Cell (HX711) Calibration

This standalone project tares and calculates the calibration factor for the weight/volume scale.

### How to Run
1. Navigate to the directory:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System/LoadCell_Calibration
   ```
2. Flash the firmware and open the Serial Monitor at **115200** baud:
   ```bash
   ~/.platformio/penv/bin/pio run -t upload
   ~/.platformio/penv/bin/pio device monitor
   ```
3. **Tare the scale**: Empty the scale platform completely, type `t` in the monitor, and press Enter.
4. **Calibrate**: Place a known weight (e.g., a 1000g calibration weight) on the scale, type `c`, press Enter, and input the weight in grams (`1000`).
5. **Update code**: Open `src/config.h` in the main codebase and update `calibrationFactor` with the output value.

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

This standalone project reads local OneWire temp probes on Pin 26 (Pre-heat and Pasteurization).

### How to Run
1. Navigate to the directory:
   ```bash
   cd /Users/gian/Coding/Kaong-Wine-System/DS18B20_Calibration
   ```
2. Flash and open the monitor:
   ```bash
   ~/.platformio/penv/bin/pio run -t upload
   ~/.platformio/penv/bin/pio device monitor
   ```
3. The utility will print temperatures for Index 0 (Pasteurization) and Index 1 (Pre-heat) every 2 seconds.
4. **Verification**: Since the DS18B20 is a digital sensor and is **factory-calibrated**, it does not require you to calculate or write any calibration factor in code. Simply verify the readings in ice water (~0.0°C) or room temperature against a reference thermometer to ensure the probes are working correctly and not damaged.

---

## 4. pH Sensor Calibration (UART Bridge)

Because the Secondary ESP32 is inside the fermentation enclosure, its USB port is inaccessible. The `pHSensor_Calibration` project runs a serial bridge on the Primary ESP32 to route command/response data bi-directionally over UART (`Serial2` pins 16 & 17) to the Secondary.

### How to Run
1. Ensure the Secondary ESP32 has its calibration code running (flashed via OTA previously).
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
