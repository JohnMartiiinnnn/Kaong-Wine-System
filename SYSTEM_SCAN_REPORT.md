# Kaong Wine Brewing System — System Scan & Comprehensive Upgrade Report

**Generated**: September 24, 2026  
**Repository**: `/Users/gian/Coding/Kaong-Wine-System`  
**Git Status**: Synchronized with `origin/main` (commit `881fca4` + local transfer upgrades)  
**Compilation Status**:  
* Primary Firmware (`src/`): PlatformIO `esp32dev` — **SUCCESS** (RAM: 17.3%, Flash: 54.0%)  
* Secondary Firmware (`Secondary_Transmitter/`): PlatformIO `esp32dev` & `calibration` — **SUCCESS** (RAM: 6.7%, Flash: 22.5%)

---

## 1. Executive Summary & Liquid Transfer Upgrade

### Liquid Transfer Drain-To-Empty Protocol (Implemented)
Per user specifications, automated liquid transfers between chambers (Pre-Heat → Fermentation and Fermentation → Pasteurization) now rely **100% on flow sensor pulse activity** to verify fluid displacement and detect leftover liquids.

#### Operational Rules
1. **Flow Sensor Priority**: Neither the load cell nor fixed volume target triggers transfer completion. Liquid transfer continues until the upstream chamber physically runs dry.
2. **Priming Grace Window**: Pumps run for 15 seconds (`TRANSFER_PRIMING_GRACE_MS = 15000UL`) before zero-pulse stall detection is allowed to trigger.
3. **60-Second Settling & Drain Detection**: Once liquid empties and the flow sensor detects zero pulse changes for **60 consecutive seconds (1 minute)** (`TRANSFER_DRAIN_TIMEOUT_MS = 60000UL`), the former chamber is confirmed completely evacuated.
4. **Minimum Volume Safety Validation**:
   * A transfer is only declared successful if `transferVolumeTransferred >= minVolumeReq * 0.85f` (or `>= 0.5L` in test mode).
   * If 60 seconds of zero pulses elapse with `< 0.5L` transferred (or less than minimum volume threshold), `transferDryRunAlarm` trips immediately.
   * **Safety Interlock**: Under a dry-run or stalled condition, pumps halt immediately, heaters remain hard-locked at 0%, and the system **never** initializes the next stage.
5. **Runtime Ceiling**: Maximum continuous pump safety ceiling extended from 5 minutes to 15 minutes (`TRANSFER_MAX_SAFETY_MS = 900000UL`) to support full-batch transfers up to 50 L without premature cutoff.
6. **Live TFT Settling Feedback**: When pulses stop, the dashboard status tile dynamically displays `DRAINING (XXs)` counting down the 60-second clearing window.

---

## 2. Priority 1: Firmware Safety & Active Brew Resilience

### 2.1 Bypass Scale Auto-Tare on Active Brew Reboot (Phase 5B)
* **Location**: [`src/main.cpp:662`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L662) and [`loadBrewStateFromNVS()`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L425)
* **Problem**: In `setup()`, `scale.tare(10)` executes unconditionally before NVS brew state is restored. If an E-stop button is pressed/released or a momentary power fluctuation reboots the ESP32 during an active brew, the scale zeroes out the actual liquid in the vat to 0.0 kg. In addition, `setup()` resets `currentAppState = START_MENU` instead of resuming `DASHBOARD_ACTIVE`.
* **Fix**:
  1. Call `loadBrewStateFromNVS()` before initializing `scale`.
  2. If `activeBrewStage >= 0`, bypass `scale.tare(10)` to preserve the calibrated baseline.
  3. Resume directly into `currentAppState = DASHBOARD_ACTIVE`.

### 2.2 Dashboard Return Confirmation Modal Repaint Race Condition (Phase 5A)
* **Location**: [`src/main.cpp:3587-3589`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L3587-L3589)
* **Problem**: When `LEFT` is pressed on `DASHBOARD_ACTIVE`, `returnConfirmState = 1` renders the modal confirmation prompt (`drawReturnConfirmation()`). However, the 1-second loop ticker continues invoking `updateDashboardValues()`, drawing parameter tiles directly over the modal box and causing visual artifacting.
* **Fix**:
  ```cpp
  if (currentAppState == DASHBOARD_ACTIVE && returnConfirmState == 0) {
    updateDashboardValues();
  }
  ```

### 2.3 On-Screen Low-Volume (< 5.0 L) Lockout Banner (Phase 5C)
* **Location**: [`src/display.cpp:380-415`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L380-L415)
* **Problem**: SSR heating is safely locked out when volume is `< 5.0 L` to prevent dry firing, but the LCD displays no visual explanation why heating duty cycle is 0%.
* **Fix**: Render a distinct amber alert banner: `⚠️ VOL < 5.0L (HEATER LOCKED)` when volume safety halts heating.

### 2.4 Fermentation Fan Baseline Parameter Bug
* **Location**: [`src/main.cpp:3082-3087`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L3082-L3087)
* **Problem**: When `liquidTemp <= stageTargetTemp[1]`, the code hardcodes `setFanSpeed(20)`. Even if the operator sets `pidFanPercent = 0` in Settings, the controller forces fermentation fans ON at 20% while the quartz heater is trying to warm the liquid.
* **Fix**: Respect `pidFanPercent` (`setFanSpeed(pidFanPercent)`). If `pidFanPercent == 0`, keep fan relays de-energized during heating.

---

## 3. Priority 2: UI/UX Spatial Collisions, Boundaries & Keypad Muscle Memory

### 3.1 Sensor Monitor (`KEY VALUES`) Footer Overlap
* **Location**: [`src/display.cpp:1272-1317`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L1272-L1317)
* **Problem**: Row 8 (`RAW DEBUG`) starts at `Y = 60 + 47*8 = 436` and extends down to Y=481, completely obliterating the `RETURN: BACK` footer text.
* **Fix**: Change layout to `yStart = 54` and `yGap = 41` so all 9 rows fit between Y=54 and Y=425, keeping the footer bar (`Y = 434–480`) clean.

### 3.2 Transfer Test Panel Frame Border Clipping
* **Location**: [`src/display.cpp:2490-2494, 2544`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L2490-L2494)
* **Problem**: Lower tank frame border `drawRect(10, 251, 300, 185, ...)` extends to Y=436, cutting 2 pixels into the white footer bar (Y=434).
* **Fix**: Reduce frame height to `182` so it terminates cleanly at Y=433.

### 3.3 Active Dashboard Sub-Header vs. Wi-Fi Badge Collision
* **Location**: [`src/display.cpp:399-401`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L399-L401)
* **Problem**: When idle, `"STARTED: NOT STARTED"` text extends past X=238, colliding directly into the Wi-Fi status badge.
* **Fix**: Format as `"START: NOT SET"` or `"START: %s"`.

### 3.4 Dashboard Footer Height & Inconsistent Keypad Hint
* **Location**: [`src/display.cpp:493-496`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L493-L496)
* **Problem**: Dashboard draws footer with `fillRect(0, 444, 320, 36, TFT_WHITE)` leaving an unpainted 10px strip between Y=434 and Y=444. The label also displays `"ENTER: MENU"`, but the physical keypad button is `SELECT`.
* **Fix**: Use standard `fillRect(0, 434, 320, 46, TFT_WHITE)` and change hint to `"SELECT: MENU"`.

### 3.5 Settings Menu Footer Hints Frozen
* **Location**: [`src/display.cpp:2124-2134`](file:///Users/gian/Coding/Kaong-Wine-System/src/display.cpp#L2124-L2134)
* **Problem**: Footer hints only render on full redraw (`settingsNeedsFullRedraw = true`). When toggling in/out of editing mode with `SELECT`, the footer stays stuck on navigation hints.
* **Fix**: Redraw footer hints whenever `settingsEditing` toggles.

### 3.6 Asymmetric Navigation in `RTC_SET_MENU`
* **Location**: [`src/main.cpp:2362-2366`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L2362-L2366)
* **Problem**: Pressing `LEFT` (RETURN) on the CANCEL field drops the user into `SYSTEM_CHECK_MENU`, even though RTC setup was opened from `SETTINGS_MENU`.
* **Fix**: Route `LEFT` back to `SETTINGS_MENU`.

### 3.7 Dead Code / Unreachable States
* **Location**: `COOLING_MENU`, `STAGE_PARAM_MENU`, `HEATER_TEST_PICK`, `CALIB_WIZARD`
* **Problem**: These full UI screens exist in `display.cpp` and `main.cpp` with complete button handlers, but their entry points were orphaned or replaced in menus.
* **Recommendation**: Either link them into `SYSTEM CHECK` / sub-views or remove them to recover flash memory.

---

## 4. Priority 3: Telemetry, Web Interface & Connectivity

### 4.1 Primary Web Monitor Parity (`src/server.cpp`)
* **Problem**: The embedded web page (`INDEX_HTML`) served directly by the ESP32 at `http://192.168.1.137/` or Soft-AP `http://192.168.4.1/` displays only 8 legacy fields. While `/data` returns 36+ fields, local Wi-Fi users cannot see active stage, transfer volume, yeast status, mixer speed, or pasteurization temp on the web page.
* **Fix**: Update the embedded mobile-friendly `INDEX_HTML` to render the multi-stage pipeline, active stage banner, and actuator badges matching `master_dashboard.py`.

### 4.2 SD Card CSV Log Buffer Safety
* **Location**: [`src/logging.cpp:43-80`](file:///Users/gian/Coding/Kaong-Wine-System/src/logging.cpp#L43-L80)
* **Observation**: `lineBuf` is 320 bytes and formatted with `sprintf`. While current fields fit in ~240 bytes, using `snprintf(lineBuf, sizeof(lineBuf), ...)` prevents memory corruption if format tokens expand.

### 4.3 1-Wire DS18B20 Index Mapping Stability
* **Location**: [`src/main.cpp:3624-3636`](file:///Users/gian/Coding/Kaong-Wine-System/src/main.cpp#L3624-L3636)
* **Observation**: Probes on the shared Pin 26 bus are resolved by `getTempCByIndex(0)` (Pre-heat) and `getTempCByIndex(1)` (Pasteurization). In DallasTemperature, indices follow alphabetical hardware ROM addresses. If one probe is disconnected or replaced, the enumeration can invert.
* **Recommendation**: Store calibrated 8-byte ROM addresses for both probes in NVS or config to guarantee physical probe identity.

---

## 5. Implementation Roadmap

| Step | Action Item | Target File(s) | Status |
| :--- | :--- | :--- | :--- |
| **1** | Liquid Transfer 60s Drain-to-Empty Protocol | `config.h`, `main.cpp`, `display.cpp` | **COMPLETED & VERIFIED** |
| **2** | Bypass Scale Auto-Tare on Active Brew Reboot | `main.cpp` | Pending Approval |
| **3** | Guard Return Confirmation Modal Repaint | `main.cpp` | Pending Approval |
| **4** | Low-Volume (< 5.0L) Locked Status Banner | `display.cpp` | Pending Approval |
| **5** | Fermentation Fan Baseline Parameter Enforcement | `main.cpp` | Pending Approval |
| **6** | UI Spatial Bounds Fixes (Sensor Monitor, Transfer Test) | `display.cpp`, `main.cpp` | Pending Approval |
| **7** | Soft-AP Web Monitor Telemetry Parity | `server.cpp` | Pending Approval |
