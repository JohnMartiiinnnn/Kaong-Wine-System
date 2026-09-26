# Kaong Wine Brewing System: System Scan and Upgrade Report

**Updated**: September 26, 2026  
**Repository**: `/Users/gian/Coding/Kaong-Wine-System`  
**Git Status**: Synchronized with `origin/main` (commits `1154165` and `a315381` merged)  
**Compilation Status**:  
* Primary Firmware (`src/`): PlatformIO `esp32dev`: **SUCCESS** (RAM: 17.3%, Flash: 54.0%)  
* Secondary Firmware (`Secondary_Transmitter/`): PlatformIO `esp32dev` & `calibration`: **SUCCESS** (RAM: 6.7%, Flash: 22.5%)

---

## 1. Hardware and Subsystem Audit

### SD Card Configuration and Formatting Rules
* **Required Filesystem**: **FAT32** (on macOS Disk Utility, select **MS-DOS (FAT)**).
* **Required Partition Table**: **Master Boot Record (MBR)**. Do not format using GUID Partition Map (GPT).
* **Allocation Size**: Default or 32 KB.
* **Driver Architecture**: ESP32 standard `SD.h` utilizes the FatFS SPI subsystem and strictly rejects exFAT, NTFS, APFS, and GPT partition structures. Cards 64 GB or larger pre-formatted as exFAT fail at [`src/main.cpp:636`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L636) (`SD.begin()`), causing `sdStatus = false` and logging failure.

---

## 2. Liquid Transfer Drain-To-Empty Protocol (Implemented)

Automated liquid transfers between chambers (Pre-Heat -> Fermentation and Fermentation -> Pasteurization) rely on flow sensor pulse verification and load cell volume boundaries.

### Completed Operational Rules
1. **Flow Sensor Priority**: Fluid displacement continues until the upstream chamber physically drains dry.
2. **Priming Grace Window**: Pumps run for 15 seconds (`TRANSFER_PRIMING_GRACE_MS = 15000UL`) before stall detection activates.
3. **Dual-Stage Drain Settling Verification**:
   * During bulk pumping, 30 seconds of volume delta < 0.05L indicates a stall (`TRANSFER_DRAIN_TIMEOUT_MS = 30000UL`).
   * Once 90% of target liquid is transferred, a 25-second settle window (`TRANSFER_DRAIN_SETTLE_MS = 25000UL`) with < 0.05L delta confirms complete chamber evacuation. Threshold lowered from 0.15L to 0.05L so flow rates as low as 0.12 L/min keep pumps running.
4. **Hardware Glitch and Noise Filtering**: Interrupt routines enforce a 14 ms microsecond debounce ceiling (maximum 71.4 Hz / ~9.5 L/min pump rate) to eliminate pump motor EMI and dry-air turbine cavitation on input-only GPIO 34.
5. **Anti-Runaway Volume Ceiling**: Pumping terminates automatically if transferred volume reaches `max(target * 1.50 + 1.0L, target + 2.0L)`, preventing premature cutoff from sensor K-factor discrepancies.
6. **Master Volume Clamping to Load Cell Ground Truth**: All live and final transferred liquid volume values (`transferVolumeTransferred`, `transfer1Volume`, `transfer2Volume`, and destination `chamberVolume`) are strictly clamped to the initial load cell weight (`transferStartWeight`) captured after initialization.
7. **Empirical K-Factor Logging**: Upon transfer completion, the controller computes and prints `pulses / transferStartWeight` over Serial to assist sensor calibration. Live transfer diagnostics are also streamed every 2 seconds.
8. **Minimum Volume Safety Validation**:
   * A transfer is only declared successful if `transferVolumeTransferred >= minVolumeReq * 0.85f` (or `>= 0.5L` in test mode).
   * If zero pulses elapse with `< 0.5L` moved, `transferDryRunAlarm` trips immediately, halting pumps, locking heaters to 0%, and preventing stage advancement.
9. **Live TFT Settling Feedback**: When pulses cease, the dashboard status tile displays `DRAINING (XXs)` counting down the settle window.

---

## 3. Priority 1: Firmware Safety and Active Brew Resilience (Pending Implementation)

### 3.1 Bypass Scale Auto-Tare on Active Brew Reboot
* **Location**: [`src/main.cpp:667-677`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L667-L677) and [`src/main.cpp:740-754`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L740-L754)
* **Current Behavior**: In `setup()`, `scale.tare(10)` runs unconditionally before `loadBrewStateFromNVS()` at line 740. If an E-stop button is pressed or power fluctuates during an active brew, the scale wipes true liquid weight to 0.0 kg. Furthermore, `setup()` unconditionally resets `currentAppState = START_MENU` instead of returning to `DASHBOARD_ACTIVE`.
* **Required Implementation**:
  1. Call `loadBrewStateFromNVS()` before `scale.begin()`.
  2. If `activeBrewStage >= 0`, bypass `scale.tare(10)` to preserve calibrated scale baseline.
  3. Set `currentAppState = DASHBOARD_ACTIVE` and trigger full redraw.

### 3.2 Guard Return Confirmation Modal Repaint Race Condition
* **Location**: [`src/main.cpp:3679-3681`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L3679-L3681)
* **Current Behavior**: When `LEFT` is pressed on `DASHBOARD_ACTIVE`, `returnConfirmState = 1` draws the modal prompt (`drawReturnConfirmation()`). The 1-second loop ticker continues invoking `updateDashboardValues()`, drawing parameter tiles over the confirmation dialog.
* **Required Implementation**:
  Wrap the call to check confirmation state:
  ```cpp
  if (currentAppState == DASHBOARD_ACTIVE && returnConfirmState == 0) {
    updateDashboardValues();
  }
  ```

### 3.3 On-Screen Low-Volume (< 5.0 L) Lockout Banner
* **Location**: [`src/display.cpp:380-418`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L380-L418)
* **Current Behavior**: SSR heating is locked out when volume is under 5.0 L to prevent dry heating elements, but the LCD displays no visual explanation why duty cycle is 0%.
* **Required Implementation**:
  Render an amber alert banner when volume safety halts heating: `VOL < 5.0L (HEATER LOCKED)`.

### 3.4 Fermentation Fan Baseline Parameter Enforcement
* **Location**: [`src/main.cpp:3178`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L3178)
* **Current Behavior**: When `liquidTemp <= stageTargetTemp[1]`, the code hardcodes `setFanSpeed(20)`. If the operator sets `pidFanPercent = 0` in Settings, the controller still forces fans ON at 20% while the quartz heater attempts to warm liquid.
* **Required Implementation**:
  Apply configured baseline: `setFanSpeed(pidFanPercent)`. If `pidFanPercent == 0`, keep fan relays de-energized.

---

## 4. Priority 2: UI Spatial Collisions, Boundaries, and Navigation (Pending Implementation)

### 4.1 Sensor Monitor (KEY VALUES) Footer Overlap
* **Location**: [`src/display.cpp:1305`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L1305) and [`src/display.cpp:1355`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L1355)
* **Current Behavior**: Row 8 (`RAW DEBUG`) starts at `Y = 60 + 47*8 = 436` and extends down to Y=476, covering the `RETURN: BACK` footer text.
* **Required Implementation**:
  Adjust layout to `yStart = 54` and `yGap = 41` so all 9 rows terminate cleanly before Y=425, keeping the footer bar (`Y = 434-480`) unobstructed.

### 4.2 Transfer Test Panel Frame Border Clipping
* **Location**: [`src/display.cpp:2526`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L2526)
* **Current Behavior**: Lower tank frame border `drawRect(10, 251, 300, 185, ...)` reaches Y=436, cutting 2 pixels into the white footer bar (Y=434).
* **Required Implementation**:
  Reduce frame height to `182` (`drawRect(10, 251, 300, 182, ...)`), ending at Y=433.

### 4.3 Active Dashboard Sub-Header vs. Wi-Fi Badge Collision
* **Location**: [`src/display.cpp:432-434`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L432-L434)
* **Current Behavior**: When idle, `"STARTED: NOT STARTED"` at X=10 extends past X=238, overlapping the Wi-Fi status badge.
* **Required Implementation**:
  Shorten the string format to `"START: NOT SET"` or `"START: %s"`.

### 4.4 Dashboard Footer Height and Keypad Hint
* **Location**: [`src/display.cpp:526-529`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L526-L529)
* **Current Behavior**: Dashboard draws footer with `fillRect(0, 444, 320, 36, TFT_WHITE)`, leaving Y=434-444 unpainted. The label also displays `"ENTER: MENU"`, but the physical keypad button is `SELECT`.
* **Required Implementation**:
  Standardize footer to `fillRect(0, 434, 320, 46, TFT_WHITE)` and change hint text to `"SELECT: MENU"`.

### 4.5 Settings Menu Footer Hints Frozen
* **Location**: [`src/display.cpp:2157-2175`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L2157-L2175)
* **Current Behavior**: Footer hints only render during full redraw (`settingsNeedsFullRedraw = true`). Toggling edit mode leaves footer text stuck on navigation hints.
* **Required Implementation**:
  Redraw footer hints whenever `settingsEditing` state changes.

### 4.6 Asymmetric Navigation in RTC Set Menu
* **Location**: [`src/main.cpp:2382-2385`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L2382-L2385)
* **Current Behavior**: Pressing `LEFT` on the CANCEL field redirects the user to `SYSTEM_CHECK_MENU`, even though RTC setup was accessed from `SETTINGS_MENU`.
* **Required Implementation**:
  Route `LEFT` return to `SETTINGS_MENU`.

### 4.7 Dead Code / Unreachable States
* **Location**: `COOLING_MENU`, `STAGE_PARAM_MENU`, `HEATER_TEST_PICK`, `CALIB_WIZARD`
* **Current Behavior**: These screens exist in `display.cpp` and `main.cpp` with button handlers, but their entry points were orphaned during menu reorganization.
* **Required Implementation**:
  Either link entry points into `SYSTEM CHECK` / sub-views or remove them to recover flash.

---

## 5. Priority 3: Telemetry, Web Interface, and Logging (Pending Implementation)

### 5.1 Primary Web Monitor Telemetry Parity
* **Location**: [`src/server.cpp:3-60`](file:///Users/gian/Coding/Kaong-Wine-System/src/server.cpp#L3-L60)
* **Current Behavior**: Embedded `INDEX_HTML` on the ESP32 Soft-AP/LAN displays only 8 legacy fields. Active stage, transfer progress, yeast dispensing, mixer RPM, and pasteurization temperatures are omitted.
* **Required Implementation**:
  Update `INDEX_HTML` with a multi-stage pipeline status layout matching the comprehensive `/data` JSON payload.

### 5.2 SD Card CSV Log Buffer Safety
* **Location**: [`src/logging.cpp:53-79`](file:///Users/gian/Coding/Kaong-Wine-System/src/logging.cpp#L53-L79)
* **Current Behavior**: Telemetry logging uses unbounded `sprintf` into a 320-byte buffer (`lineBuf`).
* **Required Implementation**:
  Replace with `snprintf(lineBuf, sizeof(lineBuf), ...)`.

### 5.3 1-Wire Dedicated Pin Separation (Implemented)
* **Location**: [`src/config.h:26-28`](file:///Users/gian/Coding/Kaong-Wine-System/src/config.h#L26-L28) and [`src/main.cpp:32-34`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L32-L34)
* **Implementation**: Pre-heat DS18B20 is dedicated to GPIO 26 (`ONE_WIRE_PREHEAT`). Pasteurization DS18B20 is dedicated to GPIO 33 (`ONE_WIRE_PAST`). Display touch CS on GPIO 33 has been decommissioned. Because each bus contains exactly one sensor (Index 0), probe address swapping is permanently eliminated.

---

## 6. Implementation Status Checklist

* **Liquid Transfer Drain-to-Empty Protocol**: COMPLETED and VERIFIED.
* **SD Card Hardware Format Specification**: COMPLETED (FAT32, MBR partition table, 32KB cluster).
* **Bypass Scale Auto-Tare on Active Brew Reboot**: PENDING APPROVAL.
* **Guard Return Confirmation Modal Repaint Race**: PENDING APPROVAL.
* **Low-Volume Locked Heater Status Banner**: PENDING APPROVAL.
* **Fermentation Fan Baseline Parameter Enforcement**: PENDING APPROVAL.
* **UI Spatial Bounds and Navigation Alignment**: PENDING APPROVAL.
* **Soft-AP Web Monitor Telemetry Parity**: PENDING APPROVAL.
* **SD Card CSV Log Buffer snprintf Safety**: PENDING APPROVAL.
* **1-Wire Dedicated Pin Separation (GPIO 26 & GPIO 33)**: COMPLETED and VERIFIED.
