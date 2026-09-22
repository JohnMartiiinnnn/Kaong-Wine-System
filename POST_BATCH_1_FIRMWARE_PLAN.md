# Kaong Wine Firmware & Telemetry Post-Batch-1 Comprehensive Upgrade Plan

**Created**: September 16, 2026  
**Status**: Staged for Post-Batch-1 Deployment (Ready for OTA Flash)  
**Target Repositories**: `~/Projects/Kaong-Wine/` & `~/systems/`  

---

## 1. Executive Summary & Incident Root Causes

During the live test of **Batch 1**, physical telemetry and hardware observation revealed four critical bugs:
1. **Dry Pasteurization Heating Incident**: Transfer 2 (Fermentation to Pasteurization) stalled during initial priming. Because no flow pulses registered within 5.0 seconds, the code declared transfer complete, advanced to Pasteurization, and fired the Pasteurization heater at **100% duty cycle on an empty chamber**.
2. **Premature Chamber 1 Cutoff**: Transfer 1 (Pre-Heat to Fermentation) stopped after moving only 300 mL due to an over-eager 5s dry-run timeout overriding the physical load cell scale.
3. **Fermentation Liquid Temp Lag**: The PID controller was reading `incomingData.room2Temp` (ambient air inside the enclosure) instead of `incomingData.room2LiquidTemp` (submerged DS18B20 probe), throttling the quartz heater and turning ON cooling fans prematurely.
4. **Telemetry Blindspots**: The web API (`/data`), local CSV logger, and Google Sheets lacked real-time visibility into active transfer progress, stage setpoints, yeast dispensing status/grams, and cumulative mixing runtime.

---

## 2. Comprehensive Implementation Specifications

### Phase 1: Critical Heater Safety Interlock & Stage Protection (`src/main.cpp`)

* **Strict Safety Rule**: A heating stage must **NEVER** start after an aborted or dry transfer.
* **Implementation**:
  ```cpp
  if (transferDone) {
      stageTransferring = false;
      setPump1(false);
      setPump2(false);
      pumpPreHeatFermOn = false;
      pumpFermPastOn = false;

      // CRITICAL HEATER SAFETY INTERLOCK:
      // If dry-run alarm triggered or virtually no liquid was moved (< 0.5L),
      // DO NOT advance stage and NEVER turn on the heater!
      if (transferDryRunAlarm || transferVolumeTransferred < 0.5f) {
          // Force all heaters to 0% immediately
          currentHeatingPercent = 0;
          digitalWrite(SSR_PREHEAT, LOW);
          digitalWrite(SSR_FERM, LOW);
          digitalWrite(SSR_PAST, LOW);

          // Trigger persistent alarm state on screen
          alarmActive = true;
          alarmType = ALARM_TEMP; // Or ALARM_TRANSFER_FAILED
          buzzerAlarmActive = true;
          mcp.digitalWrite(LIGHT_R, RELAY_ON);
          mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
          mcp.digitalWrite(LIGHT_G, RELAY_OFF);
          return; // Abort stage advancement!
      }

      // Only advance stage if liquid transfer was physically confirmed
      activeBrewStage = stageTransferTarget;
      stageStartMillis = millis();
      tempHistoryCount = 0;
  }
  ```

---

### Phase 2: Pump Priming Grace Window & Dual-Signal Drain Cutoff (`src/main.cpp`)

* **File**: `src/main.cpp` (Lines 2600–2635)
* **Priming Grace Window**: Give pumps **15 seconds** of run time (`TRANSFER_PRIMING_GRACE_MS = 15000UL`) before allowing any zero-pulse dry-run checks to trigger.
* **Drain Detection Logic**:
  1. **Transfer 1 (Pre-Heat -> Fermentation)**:
     * Primary Trigger: `hx711Status && currentWeight <= 0.25f` (scale confirms physical drain).
     * Flow Stall Check: Only allowed if `millis() - transferStartMs >= 15000UL` AND `millis() - transferLastPulseMs >= 10000UL` AND scale reads `< 0.5f`.
  2. **Transfer 2 (Fermentation -> Pasteurization)**:
     * Primary Trigger: Target volume reached (`transferVolumeTransferred >= transferTargetVolume`) with scale/batch verification.
     * Flow Stall Check: Only allowed if `millis() - transferStartMs >= 15000UL` AND `millis() - transferLastPulseMs >= 10000UL` AND `transferVolumeTransferred >= 0.8f * transferTargetVolume`.
  3. **Hardware Safety Ceiling**: Maintain 5-minute maximum continuous pump runtime limit.

---

### Phase 3: Fermentation Temperature Control Fix (`src/main.cpp`)

* **File**: `src/main.cpp` (Lines 2701–2710)
* **Bug Fix**: Line 2707 switches to the submerged **DS18B20 liquid probe**:
  ```cpp
  // Prioritize true DS18B20 submerged liquid probe in Fermentation Tank
  if (incomingData.room2LiquidTemp > -50.0f && incomingData.room2LiquidTemp < 100.0f) {
      liquidTemp = incomingData.room2LiquidTemp;
  } else if (incomingData.sensor2Status > 0) {
      // Safe fallback to ambient only if liquid sensor is disconnected
      liquidTemp = incomingData.room2Temp;
  }
  ```
* **Expected Result**: PID loop tracks true liquid temp, fires quartz heater until liquid hits 38.0 °C, and keeps fermentation fans OFF during heating.

---

### Phase 4: Full Telemetry & 28-Column Pipeline Sync

#### A. JSON `/data` Schema (`src/server.cpp`)
Add the following fields:
* `"xfer"`: `stageTransferring ? 1 : 0`
* `"xfer_vol"`: `transferVolumeTransferred`
* `"xfer_tgt"`: `transferTargetVolume`
* `"xfer_pct"`: `transferProgressPct`
* `"p1"` / `"p2"`: `pump1_state` / `pump2_state`
* `"tgt_ph"` / `"tgt_cool"` / `"tgt_ferm"` / `"tgt_past"`: Stage setpoints (40.0, 38.0, 38.0, 72.0 °C)
* `"yd_tgt"` / `"yd_act"` / `"yd_state"`: Yeast target grams, actual dispensed grams, dispenser status
* `"mix_mode"` / `"mix_spd"` / `"mix_cyc_sec"` / `"mix_tot_min"`: Mixer mode, speed, active cycle sec, total run min

#### B. Local CSV & Google Sheets Auto-Sync (`winebrew_sheets_sync.py`)
* Automatically formats and provisions these 28 columns into future batch tabs (`Batch 2`, `Batch 3`...).
* Keeps frozen Row 1 headers and dynamic 5,000-row border expansion.

---

### Phase 5: Pre-Sap UI Stability, E-Stop Persistence & Volume Warnings

#### A. UI Confirmation Race Condition Guard (`src/main.cpp`)
* **Problem**: When `LEFT` is pressed on `DASHBOARD_ACTIVE`, `returnConfirmState = 1` paints `drawReturnConfirmation()`, but `loop()` continues calling `updateDashboardValues()`, painting tiles over the modal and causing visual artifacting.
* **Fix**: Guard background dashboard drawing:
  ```cpp
  if (currentAppState == DASHBOARD_ACTIVE && returnConfirmState == 0) {
    updateDashboardValues();
  }
  ```

#### B. E-Stop / Hard Reboot State & Tare Preservation (`src/main.cpp`)
* **Problem**: Hardware E-stop or power flicker reboots ESP32 into `setup()`, which unconditionally executes `scale.tare(10)`, zeroing out actual liquid weight in the vat, and resets `currentAppState = START_MENU`.
* **Fix**:
  1. Load NVS brew state before scale initialization.
  2. If `activeBrewStage >= 0` (brew was in progress), bypass `scale.tare(10)` to preserve the calibrated baseline.
  3. Resume directly to `DASHBOARD_ACTIVE` instead of defaulting to `START_MENU`.

#### C. Low-Volume (< 5.0 L) Visual Dashboard Banner (`src/display.cpp`)
* **Problem**: SSR heating is silently hard-locked to 0% when chamber volume is < 5.0 L with no visual indication of why heating is idle.
* **Fix**: Render a clear status tile/badge on the dashboard: `⚠️ VOL < 5.0L (HEATER LOCKED)` when volume is below the safety threshold.

---

## 3. Deployment Checklist

1. [x] Apply `main.cpp` safety interlock (never advance stage or heat if transfer failed).
2. [x] Apply `main.cpp` 15-second pump priming grace window and drain cutoff logic.
3. [x] Apply `main.cpp` Line 2707 submerged liquid probe fix.
4. [x] Update `server.cpp` with 28-field `/data` schema.
5. [x] Swap 1-Wire DS18B20 index mapping for new pasteurizer probe (`Index 0: Preheat`, `Index 1: Pasteurization`).
6. [ ] Implement Phase 5A: `returnConfirmState` guard in `updateDashboardValues()`.
7. [ ] Implement Phase 5B: NVS boot recovery (skip auto-tare on active brew recovery).
8. [ ] Implement Phase 5C: Low-volume (< 5.0L) on-screen warning banner.
9. [ ] User confirmation prior to flashing firmware.
