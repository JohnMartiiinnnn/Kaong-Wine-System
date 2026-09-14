# Automated Kaong Wine Brewing System — UI/UX Audit & Formal Verification Framework

## 1. Executive Summary

This document captures the complete audit of visual, navigational, and perceptual inconsistencies across the ILI9488 3.5" TFT display firmware (`src/display.cpp`, `src/main.cpp`) and outlines the formal mathematical verification framework to prevent future regressions.

---

## 2. Inventory of Discovered Inconsistencies

### A. Critical Spatial Collisions & Boundary Clipping
* **Sensor Monitor (Key Values)** (`display.cpp:975-1020`): Row 8 (`RAW DEBUG`) starts at Y=436 and ends at Y=481, completely obliterating the `RETURN: BACK` footer text.
* **Transfer Test Section Frame** (`display.cpp:2130-2134`): Lower tank frame rectangle is drawn with height 185 at Y=251, ending at Y=436 and cutting 2 pixels into the top of the white footer bar (Y=434).
* **Scale Calibration** (`display.cpp:950-955`): The `SAVE & EXIT` tile starts at Y=380 with 60px height, ending at Y=440 and crossing into the footer boundary.
* **Dashboard Sub-header vs Wi-Fi Badge** (`display.cpp:334-336`): When idle, `"STARTED: NOT STARTED"` text extends past X=238, colliding directly into the Wi-Fi status badge.
* **Footer Text Screen Clipping** (`display.cpp:2922, 2992`): The 54-character footer strings in `PID Control` and `PID Config` measure 324px in width, clipping 2 pixels off both the left and right edges of the 320px screen.

### B. Missing Footer Background Boxes (`fillRect 0, 434, 320, 46, TFT_WHITE`)
8 screens omit the white footer wipe, causing remnants of previous screens to bleed through into the navigation bar:
* Main Menu (`display.cpp:486`)
* Load Cell (`display.cpp:545`)
* Light Indicators (`display.cpp:762`)
* Motor Test (`display.cpp:810`)
* Fan Test Pick & Relay Test Pick (`display.cpp:624`)
* Sensor Monitor (`display.cpp:971`)
* Scale Calibration (`display.cpp:943`)
* Active Dashboard (`display.cpp:428`): Draws footer at Y=444 with height 36 instead of Y=434 with height 46, leaving an unpainted 10px strip.

### C. Inconsistent Header Colors
* 26 screens use Header Green (`0x03E0`).
* 6 screens use Navy (`TFT_NAVY`): Main Menu, New Brew Wizard, Active Dashboard, Key Values, Mixer Control, PID Tracking.

### D. Keypad Hints & Muscle Memory
* **Dashboard Footer**: Labeled as `"ENTER: MENU"`, but the physical keypad button is `SELECT`.
* **Load Cell**: Uses `"RETURN : GO BACK"` at Y=450 in font 2 instead of standard `"RETURN: BACK"` at Y=458 in font 1.
* **Relay Test**: Uses `"RETURN: BACK TO MENU"` instead of standard `"RETURN: BACK"`.
* **Settings Menu**: Footer hints only render on full redraw; when toggling in/out of editing mode with `SELECT`, the footer stays stuck on navigation hints.

### E. Redraw Flicker & Repaint Inefficiencies
* `drawMixerMenu`: Has no `mixerNeedsFullRedraw` gate. Repaints all 153,600 pixels on every button press.
* `drawStageParamMenu`: Full redraw flag is cleared without wiping the screen background.
* `drawStartMenu`: Does not use `prevSel` selective tile diffing; repaints all 4 large tiles on every UP/DOWN press.

### F. Primary Web Monitor Desync
* The Primary web monitor at `http://192.168.1.137/` in `src/server.cpp` only displays 8 legacy fields. It lacks active stage indicators, pasteurization temp, mixer speed, dispenser status, and system health badges.

---

## 3. Formal Finite State Machine (FSM) Graph Proof

Using graph reachability analysis on `src/main.cpp`, all 34 states were evaluated for Reachability from `START_MENU` and Reversibility back to `START_MENU`:

### Unreachable States (Dead Code)
1. `COOLING_MENU`: Full UI and button controls exist, but navigation hook was overwritten.
2. `MIXER_MENU`: Full UI and button controls exist, but navigation hook was overwritten.
3. `STAGE_PARAM_MENU`: Full simulation UI exists, but cannot be opened from Dashboard.
4. `HEATER_TEST_PICK` & `HEATER_TEST_MENU`: Replaced by PID Control in System Check; currently inaccessible.
5. `CALIB_WIZARD`: 166 lines of automated flow and load cell wizard code unlinked in menus.
6. `CALIBRATION_MODE`: Replaced by `LOAD_CELL_PAGE`; orphaned.

### Asymmetric Navigation
* In `RTC_SET_MENU`: Pressing `SELECT` on CANCEL returns to `SETTINGS_MENU`, but pressing `LEFT` (RETURN) dumps the user into `SYSTEM_CHECK_MENU`.

---

## 4. UI/UX Verification Principles

1. **Zero-Flicker Perceptual Performance**:
   * Screen background cleared only once on entry (`needsFullRedraw = true`).
   * Moving cursor only un-highlights tile A and highlights tile B using `prevSel`.
   * Dynamic values update in place using `setTextPadding()`.
2. **Cognitive State Clarity**:
   * Navigating: Single dark-grey border (`TFT_DARKGREY`).
   * Focused: Single white border (`TFT_WHITE`).
   * Active Editing: Double-line white border.
   * Dynamic footer hints confirm active mode (`ADJUST VALUE` vs `NAVIGATE`).
3. **Spatial Muscle Memory**:
   * UP/DOWN: Vertical navigation or numeric adjustment.
   * LEFT: Back / Cancel (never destructive).
   * RIGHT: Forward / Sub-tab.
   * SELECT: Confirm / Toggle / Edit.
4. **Semantic Color Tokens**:
   * Green (`0x03E0`): Navigation context.
   * Blue (`0x3566`): Focused tile.
   * Emerald (`0x0400`): Active / energized.
   * Charcoal (`0x4208`): Off / standby.
   * Ruby (`0xF800`): Error / alert / stopped.
   * Amber (`0xFFE0`): Caution notice.
