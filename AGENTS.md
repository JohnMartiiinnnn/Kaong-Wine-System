# Automated Wine Brewing System - Unified Project Context & Rules

This document serves as the single source of truth for the Automated Wine Brewing System thesis project. It merges the architecture details, hardware configurations, display design guidelines, programming patterns, and debugging instructions. All Agentic AIs working on this repository must adhere to this document.

---

## 1. System Architecture

The system utilizes two **ESP32 DevKits** communicating via **UART (Serial)**.

*   **Primary Controller (Master):** Handles the UI (TFT LCD), local sensors (Pre-heating/Pasteurization), high-power actuators (Relays/Dimmers), and data logging (SD Card).
*   **Secondary Controller (Remote Sensor Node):** Handles fermentation environment sensors (Ambient/Liquid), pH monitoring, and receives BLE data from a RAPT Pill digital hydrometer.

---

## 2. Compilation, Flashing & Tools

Use the PlatformIO CLI commands for building and uploading the primary firmware:

```bash
# Compile code
~/.platformio/penv/bin/pio run

# Flash code (auto-detect port)
~/.platformio/penv/bin/pio run -t upload

# Flash code on a specific port
~/.platformio/penv/bin/pio run -t upload --upload-port /dev/cu.usbserial-0001
```

*Note: Host macOS clang may show false-positive errors on TFT constants or Arduino header references in your IDE. Ignore them; the ESP32 toolchain via PlatformIO is the absolute source of correctness.*

---

## 3. Primary Controller — Source Layout

The primary firmware lives in `src/` and is split into modules:

```
src/
  config.h       — All #includes, pin constants, enums, struct_message, extern declarations
  display.h/.cpp — All TFT draw functions (drawDashboard, drawWizard, etc.)
  server.h/.cpp  — Wi-Fi AP web server (INDEX_HTML, handleRoot, handleData)
  logging.h/.cpp — SD card CSV logging (logDataToSD) and UART checksum
  main.cpp       — Global variable definitions, setFanSpeed, setup(), loop()
```

**Rule:** Every `.cpp` file includes only its own `.h`. All shared symbols live in `config.h` and are declared `extern` there — they are *defined once* in `main.cpp`.

### Hardware Connections
*   **Display:** 3.5" TFT LCD (ILI9488) via SPI.
*   **GPIO Expansion (MCP23017):**
    *   **GPB0-GPB7:** 8-Channel Relay Module.
    *   **GPA0-GPA4:** 5-Button Keypad (Right, Left, Up, Down, Select).
    *   **GPA6:** Emergency Stop Button.
    *   **GPA7:** Fan Relay.
*   **Sensors:**
    *   **BME280:** Ambient Temperature (Pre-heating area).
    *   **DS18B20 (x2):** Liquid temperatures (Vat/Pasteurization) on a shared OneWire bus (pin 26).
    *   **HX711:** Load cell for volume/weight measurement (DT=36, SCK=27).
    *   **DS3231 RTC:** Real-time clock for logging timestamps.
    *   **Flow Sensors:** Flow Sensor 1 (Pin 32, Pre-Heat -> Ferm) and Flow Sensor 2 (Pin 34, Ferm -> Past) with hardware interrupts.
*   **Actuators:**
    *   **PWM (Pin 25):** 4-wire Fan speed control. Preheat fan is inverted; fermentation fan is non-inverted.
    *   **PWM (TBD):** TB6612FN PWMA — mixing impeller speed (LEDC ch1, 1 kHz, 8-bit).
    *   **SSR Heaters:** Pins 13 (Pre-Heat), 12 (Fermentation), and 14 (Pasteurization) driving solid-state relays (SSRs) via slow time-proportional PWM. *Note: Fermentation Quartz Radiant Heater (`SSR_FERM`, pin 12) pulse duration is capped at a maximum of 1.5 seconds (1500ms) per window to prevent radiant overheating, thermal shock, and insulation melting.*
    *   **Status LED:** GPIO 2 (onboard LED) used for Calibration Wizard signaling.
*   **Motor Driver (TB6612FN):**
    *   **PWMA → GPIO0** (ESP32) — speed control (0-100% duty)
    *   **AIN1 → 3.3 V** hardwired — fixed CW direction
    *   **AIN2 → GND** hardwired
    *   **STBY → 3.3 V** hardwired — always active
    *   **VM → 12 V**, **VCC → 3.3 V**
    *   Motor: JGB37-545 1260 12 V DC gear motor, 90 reduction ratio, 50–66 RPM, stall current 3.8 A
*   **Storage:** SD Card (SPI, CS=5) for CSV data logging.

### Software Logic State Machine
*   `SYSTEM_INIT` → `START_MENU` (after splash screen).
*   `NEW_BREW_WIZARD`: Minimum volume safety check (locked if volume requirement is not met) and option to disable preheat immersion heater.
*   `SETTINGS_MENU`: Replaces Continue Brew on Main Menu; configure minimum volume, preheat heater toggle, fermentation baseline fan speed, RTC date/time shortcut, and scale tare shortcut.
*   `RTC_SET_MENU`: 7-field D-pad navigation (YEAR, MONTH, DAY, HOUR, MIN, SAVE & EXIT, CANCEL); RIGHT increases (+), LEFT decreases (-).
*   `DASHBOARD_ACTIVE`: Three sub-views — Pre-Heating, Fermentation, Pasteurization.
*   `COOLING_MENU`: Manual/Auto fan control.
*   `MIXER_MENU`: Mixing impeller control — OFF / MANUAL (speed adjust) / AUTO (5 min ON, 355 min OFF).
*   `SENSOR_MONITOR`: Raw value display for debugging (accessed via SENSOR VALUES on Main Menu).
*   `LOAD_CELL_PAGE`: Simple scale tare utility, now located as item #11 inside `SYSTEM CHECK` menu.
*   `RAPT_TEST_MENU`: RAPT Pill telemetry logs test screen, now located as item #12 inside `SYSTEM CHECK` menu. Logs specific gravity and time logged, ignoring duplicate packets received within 15 seconds.
*   **Data Logging:** Records all sensor data to `/data_log.csv` every 60 seconds.
*   **Web Dashboard:** Soft-AP `WineBrew_System` (pass: `12345678`), mDNS `winebrew.local`. Live JSON at `/data`, UI at `/`.

---

## 4. Display Design System

TFT_eSPI, ILI9488, 320×480 px. Every system screen must follow these rules exactly.

### Layout Zones

| Zone | Y Range | Notes |
|:---|:---|:---|
| **Header** | 0–50 | Green or Navy bar |
| **Sub-info bar (optional)** | 52–79 | Dark context strip, some test screens only |
| **Content** | 52–432 | Tiles, panels, data |
| **Footer** | 434–480 | White background, 1–2 hint lines |

### Color Palette

| Name | Hex | Use |
|:---|:---|:---|
| **Header green** | `0x03E0` | All screen headers (`fillRect 0,0,320,50`) |
| **Selected/active tile** | `0x3566` | Highlighted menu item, active button |
| **Unselected tile** | `0xD6BA` | Default tile background |
| **Running/OK** | `0x0400` | Heater/pump/fan on, test passing |
| **Stopped/idle** | `0x4208` | Off state (neutral, not an error), sub-info bars |
| **Error/fail** | `0xF800` | Sensor fail, test fail, danger, START selected-but-stopped |
| **Warning** | `0xFFE0` | Notice/caution bar |
| **Dark panel** | `0x2124` | Live data panels, dark backgrounds |
| **Active panel header** | `0x0320` | Active section strip in multi-panel screens |

### Core UI Code Patterns

#### Header (every screen)
```cpp
tft.fillRect(0, 0, 320, 50, 0x03E0); // or 0x0493
tft.setTextColor(TFT_WHITE);
tft.drawCentreString("SCREEN TITLE", CENTER_X, 15, 4);
```

#### Sub-info bar (optional, test screens)
```cpp
tft.fillRect(0, 52, 320, 28, 0x4208);
tft.setTextColor(TFT_WHITE, 0x4208);
tft.drawCentreString("CONTEXT INFO", CENTER_X, 60, 2);
```

#### Tile (selection highlight)
- Selected: `0x3566` fill, white text
- Unselected: `0xD6BA` fill, black text
- **Always call `drawRect` AFTER `fillRect`** — border must go on top of fill.
```cpp
uint16_t bg = selected ? 0x3566 : 0xD6BA;
uint16_t fg = selected ? TFT_WHITE : TFT_BLACK;
tft.fillRect(x, y, w, h, bg);
tft.setTextColor(fg, bg);
tft.drawCentreString(label, cx, ty, font);
tft.drawRect(x, y, w, h, TFT_DARKGREY); // border last
```

#### Editing tile (double border)
When a row is selected AND in editing mode, draw a double white border to indicate active edit:
```cpp
if (isSelected && isEditing) {
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.drawRect(x+1, y+1, w-2, h-2, TFT_WHITE);
} else {
  tft.drawRect(x, y, w, h, TFT_DARKGREY);
}
```

#### Status tile (three-way logic for START/STOP rows)
Action rows (START/STOP) use three-way color based on running state and cursor position:
```cpp
uint16_t statusBg = isRunning ? 0x0400          // green: running
                  : isSelected ? 0xF800          // red: selected but stopped
                  : 0x4208;                      // dark: idle, not focused
```

#### Circle ON/OFF indicator
```cpp
tft.fillCircle(cx, cy, r, isOn ? 0x0400 : TFT_DARKGREY);
tft.drawCircle(cx, cy, r, TFT_BLACK);
```

#### Footer (every screen)
Single-line footer:
```cpp
tft.fillRect(0, 434, 320, 46, TFT_WHITE);
tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE   RETURN: BACK", CENTER_X, 458, 1);
```
Two-line footer:
```cpp
tft.fillRect(0, 434, 320, 46, TFT_WHITE);
tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
tft.drawCentreString("Line 1 hints", CENTER_X, 447, 1);
tft.drawCentreString("Line 2 hints   RETURN: BACK", CENTER_X, 462, 1);
```
*Footer text format: `"KEY: ACTION   KEY: ACTION   RETURN: BACK"` — always include `RETURN: BACK`.*

---

## 5. UI Code Abstractions & Checklist

### New screen checklist
1. Add `AppState` enum value in `config.h`
2. Add `bool xxxNeedsFullRedraw` + selection/state vars to `config.h` (extern) and `main.cpp` (define)
3. Add draw function declaration to `display.h`
4. Implement draw function in `display.cpp` following the layout/color rules above
5. Add state transitions in `main.cpp` (SELECT to enter, LEFT to exit, set `xxxNeedsFullRedraw = true` on entry)

### No `fillScreen` — draw header color first
Never use `tft.fillScreen(TFT_WHITE)` in any draw function. It writes all 153,600 pixels before any content appears, causing a white flash on every screen transition. Instead:
```cpp
// In the full-redraw block:
tft.fillRect(0, 0, 320, 50, 0x03E0);      // header first — user sees color immediately
tft.fillRect(0, 50, 320, 430, TFT_WHITE); // body — must be 430 height, not 384
```
**Critical:** body fill height must be `430` (covering y=50 to y=480). Using `384` leaves the footer zone (y=434–480) uncovered, causing splash-screen remnants or stale footer text to bleed through on screen transitions.

### Preventing selection flash (`static prevSel` pattern)
For any screen where only the selection highlight changes on UP/DOWN (no live data):
```cpp
void drawMyPickScreen() {
  static int prevSel = -1;
  const char *options[] = {"OPTION A", "OPTION B", "OPTION C"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t bg  = sel ? 0x3566 : 0xD6BA;
    uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 110), 280, 80, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString(options[i], CENTER_X, 105 + (i * 110), 4);
    tft.drawRect(20, 80 + (i * 110), 280, 80, TFT_DARKGREY);
  };

  if (myScreenNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    for (int i = 0; i < 3; i++) drawTile(i, mySelection == i);
    myScreenNeedsFullRedraw = false;
    prevSel = mySelection;
  } else if (prevSel != mySelection) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(mySelection, true);
    prevSel = mySelection;
  }
}
```
This pattern eliminates selection flicker by only drawing the changed tiles.

### Per-tile cursor model (`xxxSelection` + `xxxEditing`)
For screens with navigable rows that each hold an editable value (heater test, flow cal):
*   `xxxSelection` (int): which row cursor is on (0, 1, 2, ...)
*   `xxxEditing` (bool): whether that row is currently being edited
Button handling:
*   UP/DOWN when not editing: move cursor
*   UP/DOWN when editing: adjust the selected row's value
*   SELECT when not editing on a value row: enter editing (`xxxEditing = true`)
*   SELECT when editing: exit editing (`xxxEditing = false`)
*   SELECT on an action row (e.g. START/STOP): toggle directly, no editing needed
*   LEFT when editing: exit editing only (`xxxEditing = false; drawMenu();`)
*   LEFT when not editing: clean up and exit screen

### Live-data refresh (`valuesOnly` pattern)
For screens that update sensor/timer data every second:
```cpp
void drawMyDataScreen(bool valuesOnly) {
  if (!valuesOnly) {
    // full redraw: header, static labels, tiles, footer
  }
  // always: update live value text in place using setTextPadding
  tft.setTextPadding(120);
  tft.drawRightString(liveValue, 300, y + 13, 2);
  tft.setTextPadding(0);
}
```
Call `drawMyDataScreen(true)` from the 1-second ticker in `main.cpp`.

---

## 6. Hardware Quirks & Sensor Configurations

### Fan Speed & Duty Cycle Logic
The fermentation fan and the pre-heating fan are both controlled via `setFanSpeed` on Pin 25:
*   **Pre-heating Fan:** Uses **inverted** PWM duty cycle mapping.
    *   `0%` speed = maximum duty cycle (`255` on LEDC)
    *   `100%` speed = minimum duty cycle (`0` on LEDC)
    *   `map(percent, 0, 100, 255, 0)`
*   **Fermentation Fan:** Uses **non-inverted** PWM duty cycle mapping.
    *   `0%` speed = minimum duty cycle (`0` on LEDC)
    *   `100%` speed = maximum duty cycle (`255` on LEDC)
    *   `map(percent, 0, 100, 0, 255)`

#### Fan Relays (MCP23017)
The fermentation fan relays are normally-open (NO). To energize them, write `RELAY_ON` (LOW); to de-energize them, write `RELAY_OFF` (HIGH).

**Correct ON Sequence for Fermentation Fan:**
```cpp
isFermFanOn = true;                                    // 1. Set flag first
mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_ON);        // 2. Turn on relays
mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_ON);
setFanSpeed(percent);                                  // 3. Set PWM (reads isFermFanOn)
```

**Correct OFF Sequence for Fermentation Fan:**
```cpp
setFanSpeed(0);                                        // 1. Clear PWM first
mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);       // 2. Turn off relays
mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
isFermFanOn = false;                                   // 3. Clear flag
```

*Note on Relay Test Fan Driving:* When testing either fan in Relay Test mode, turning on the MCP relay alone is insufficient because Preheat Fan uses inverted PWM (`isFanOn = true; setFanSpeed(100);`) to output 0 duty cycle, while Fermentation Fan uses non-inverted PWM (`isFermFanOn = true; setFanSpeed(100);`) to output 255 duty cycle. All relay tests must use `setRelayTestChannel(idx, state)` to synchronize relays and PWM signals.

### Fermentation Chamber Sensor Routing
Two separate temperature readouts are sent by the secondary controller via `incomingData`:
*   `room2Temp`: BME280 (ambient air inside the fermentation enclosure). Use this for environment/cooling control.
*   `room2LiquidTemp`: DS18B20 (liquid probe inside the fermentation vessel). Use this for liquid monitoring.
*Note: DS18B20 returns approximately -127°C when disconnected. Always verify `if (temp > -100.0f)` before using the data in controller routines.*

---

## 7. System Check Menu (10 items, indices 0–9)

Selecting an option enters the corresponding `AppState`:

| Index | Label | AppState Entered | Description |
| :--- | :--- | :--- | :--- |
| **0** | FAN TEST | `FAN_TEST_PICK` | Test preheat or fermentation fan speeds |
| **1** | LIGHT INDICATORS | `LIGHT_TEST_MENU` | Toggle Green, Yellow, and Red status lights |
| **2** | RELAY TEST | `RELAY_TEST_MENU` | Manual/automated sequencing of all 9 relays |
| **3** | MOTOR TEST | `MOTOR_TEST_MENU` | Test JGB37 mixing motor speed & direction |
| **4** | PID CONTROL | `PID_TEST_PICK` | Test temperature profiles, PID controller, and PID Thermal Tracking (80°C step test with live graph & SD logging) |
| **5** | SD CARD VERIFY | `SD_VERIFY_MENU` | Check SPI connection & read/write capabilities on SD card |
| **6** | UART MONITOR | `UART_MONITOR_MENU` | Monitor incoming data struct from Secondary Controller |
| **7** | TRANSFER TEST | `TRANSFER_TEST_MENU` | Test pump and transfer logic between chambers |
| **8** | RAPT PILL | `RAPT_TEST_MENU` | Log up to 10 incoming BLE telemetry updates live (ignores duplicates within 15 seconds) |
| **9** | PH & FERM TEMP | `PH_FERM_MENU` | Live display for pH probe and liquid/ambient temperatures |

---

## 8. Mixing Impeller — Connection, Testing & Debugging

### Hardware Wiring
Connect the TB6612FN motor driver as follows:

| TB6612FN Pin | Connect to | Notes |
| :--- | :--- | :--- |
| **VM** | 12 V supply | Motor power rail |
| **VCC** | 3.3 V | Logic supply |
| **GND** | GND | Common ground |
| **STBY** | 3.3 V | Tie HIGH — always active |
| **AIN1** | 3.3 V | Tie HIGH — fixed CW direction |
| **AIN2** | GND | Tie LOW — fixed CW direction |
| **PWMA** | GPIO0 (ESP32) | Speed control (0–100 % duty) |
| **AO1** | Motor + | Red wire of JGB37-545 |
| **AO2** | Motor – | Black wire of JGB37-545 |

### UI Navigation
```
MAIN MENU
  ├─ NEW BREW (or VIEW ACTIVE BREW if brewing)
  ├─ SETTINGS
  ├─ SYSTEM CHECK
  └─ SENSOR VALUES
            └─ Navigate to FERMENTATION tab (RIGHT/DOWN)
                 └─ SELECT  →  enter Fermentation module view
                      └─ SELECT  →  MIXER CONTROL menu
```
Options in `MIXER CONTROL`:
*   **SELECT:** Cycle modes: `OFF` → `MANUAL` → `AUTO` → `OFF`.
*   **UP / DOWN:** Adjust speed in 10% steps (MANUAL mode only).
*   **LEFT / RETURN:** Exit back to the Fermentation dashboard view.

### Debugging & Test Steps
1.  **Bench Power:** Apply VM=12V, VCC=3.3V, tie STBY/AIN1 HIGH, AIN2 LOW. Connect PWMA to 3.3V. Motor should spin CW.
2.  **Manual Mode:** Navigate to mixer control, set to MANUAL, verify speed adjustments in 10% increments.
3.  **Auto Mode:** Confirm that it starts mixing on initial activation, running for 5 minutes (`MIXER_ON_MS`) and sleeping for 355 minutes (`MIXER_OFF_MS`).

### Speed Feedback, RPM Estimation & Stall Protection
The JGB37 mixing motor speed is estimated by measuring the output voltage on the BTS7960's current sense (`IS`) pins, which are connected to ADS1115 Channel A1 (Secondary ESP32) with a 1kΩ pull-down resistor to GND.
*   **RPM Equation:** `RPM = 66.0f - 160.19f * (SenseVoltage - 0.035f)` (range-bounded `[0.0, 66.0]`).
*   **Stall Limit Protection:** If `SenseVoltage` exceeds `0.353 V` (representing `3.0 A` current draw) for more than `3 consecutive seconds`, the Primary Controller automatically shuts down the motor (turns off auto/manual mixing, sets speed to `0%`, and sends stop commands to Secondary) to prevent gearbox damage or motor winding burnouts.

---

## 9. Flashing and Calibration for Enclosed Secondary ESP32

Because the Secondary ESP32 is inside the fermentation enclosure, its USB port is inaccessible during normal operation, and direct serial monitoring is not possible. Over-the-Air (OTA) flashing is unsupported due to high maintenance. Flashing is always performed by removing the Secondary ESP32 from the enclosure and plugging it in directly via Micro-USB.

### Flashing the Secondary ESP32
To flash the Secondary ESP32, remove it from the enclosure and run:
```bash
~/.platformio/penv/bin/pio run -d Secondary_Transmitter -t upload
```

### Sensor Calibration Workflow
Sensor calibration is handled over the board's inter-controller UART connection (`Serial2` at 115200 baud) using a bridge mode on the Primary:
1.  Remove the Secondary ESP32 from the enclosure and flash it with the calibration utility via Micro-USB:
    ```bash
    ~/.platformio/penv/bin/pio run -d Secondary_Transmitter -e calibration -t upload
    ```
2.  Flash the Primary ESP32 with the standalone bridge utility:
    *   Command: `cd pHSensor_Calibration && ~/.platformio/penv/bin/pio run -t upload`
3.  Open the Serial Monitor on the Primary ESP32:
    *   Command: `~/.platformio/penv/bin/pio device monitor`
4.  The monitor will stream pH and fermentation temperature readouts forwarded from the Secondary.
5.  Calibrate the Secondary pH probe using:
    *   `7`: pH 7.0 neutral offset calibration.
    *   `4`: pH 4.0 acid slope calibration.
6.  Once calibration is done, re-flash the production code back to both microcontrollers (removing Secondary from the enclosure again to flash it via Micro-USB).

---

## 10. Pin Mapping Summary (Main ESP32)

| Component | Pin | Note |
| :--- | :--- | :--- |
| **TFT CS** | 15 | SPI |
| **TFT DC** | 2 | SPI |
| **TFT RST** | 4 | SPI |
| **Touchscreen CS** | 33 | SPI |
| **SD CS** | 5 | SPI |
| **SPI SCLK** | 18 | Shared: SD, TFT, Touchscreen |
| **SPI MOSI** | 23 | Shared: SD, TFT, Touchscreen |
| **SPI MISO** | 19 | Shared (TFT MISO disconnected) |
| **I2C SDA** | 21 | RTC DS3231 + MCP23017 |
| **I2C SCL** | 22 | RTC DS3231 + MCP23017 |
| **Heater/Fan PWM** | 25 | LEDC ch0, 25 kHz, 8-bit |
| **OneWire** | 26 | DS18B20 shared bus (both liquid temp sensors) |
| **HX711 DT** | 36 | Load cell data (input-only) |
| **HX711 SCK** | 27 | Load cell clock |
| **UART RX** | 16 | Serial2 from Secondary ESP32 |
| **UART TX** | 17 | Serial2 to Secondary ESP32 |
| **Flow Sensor 1** | 32 | Flow Sensor 1 input (interrupt on RISING) |
| **Flow Sensor 2** | 34 | Flow Sensor 2 input (input-only, interrupt on RISING) |
| **Heater SSR 1** | 13 | Pre-heat tank heater SSR |
| **Heater SSR 2** | 12 | Fermentation tank heater SSR |
| **Heater SSR 3** | 14 | Pasteurization tank heater SSR |
| **Status LED** | 2 | Onboard LED (GPIO 2) used for Calibration Wizard |
| **Motor PWM** | 0 | TB6612FN PWMA (LEDC ch1, 1 kHz) — boot pin |

### MCP23017 Pin Assignments

| MCP Pin | Name in config.h | Function |
| :--- | :--- | :--- |
| **GPA0** | `BTN_RIGHT_PIN` | Keypad RIGHT |
| **GPA1** | `BTN_LEFT_PIN` | Keypad LEFT |
| **GPA2** | `BTN_UP_PIN` | Keypad UP |
| **GPA3** | `BTN_DOWN_PIN` | Keypad DOWN |
| **GPA4** | `BTN_SELECT_PIN` | Keypad SELECT |
| **GPA5** | — | Unused |
| **GPA6** | — | Unused |
| **GPA7** | `FAN_RELAY_PIN` | Pre-heating fan relay |
| **GPB0** | `FERM_FAN_RELAY_PIN`  | Fermentation fan relay 1 |
| **GPB1** | `FERM_FAN2_RELAY_PIN` | Fermentation fan relay 2 |
| **GPB2–GPB4** | `RELAY_PINS[2–4]` | General relay channels |
| **GPB5** | `LIGHT_G` | Status light — Green |
| **GPB6** | `LIGHT_Y` | Status light — Yellow |
| **GPB7** | `LIGHT_R` | Status light — Red |
