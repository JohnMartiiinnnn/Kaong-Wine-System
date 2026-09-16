# Kaong Wine Firmware & Telemetry Post-Batch-1 Comprehensive Upgrade Plan

**Created**: September 16, 2026  
**Status**: Staged for Post-Batch-1 Deployment (Do Not Flash During Active Run)  
**Target Repository**: `~/Projects/Kaong-Wine/`  

---

## 1. Executive Summary & Problem Diagnosis

During the live execution of **Batch 1**, physical telemetry revealed three critical operational anomalies:
1. **Fermentation Liquid Temperature Lag**: Chamber 2 ambient air reached the 38.0 °C setpoint, but actual liquid remained at ~34.4 °C because the PID controller was mistakenly assigned the ambient BME280 sensor instead of the submerged DS18B20 liquid probe, triggering cooling fans and throttling the heater prematurely.
2. **Premature Transfer Cutoff**: Liquid transfer from Chamber 1 (Pre-Heat) to Chamber 2 (Fermentation) cut off prematurely because a 5-second pulse stall after moving just 0.3 L triggered the "empty tank" dry-run failsafe while the load cell scale still held liquid.
3. **Telemetry Blindspots**: The web API (`/data`), local CSV logger, and Google Sheets lacked real-time visibility into active transfer progress, stage setpoints, yeast dispensing status/grams, and cumulative mixing motor runtime.

---

## 2. Comprehensive Implementation Specifications

### Phase 1: Fermentation Temperature Control Fix (`src/main.cpp`)

* **File**: `src/main.cpp` (Lines 2701–2710)
* **Bug**: Line 2707 assigns `liquidTemp = incomingData.room2Temp;` (ambient air).
* **Fix**:
  ```cpp
  // Prioritize true DS18B20 submerged liquid probe in Fermentation Tank
  if (incomingData.room2LiquidTemp > -50.0f && incomingData.room2LiquidTemp < 100.0f) {
      liquidTemp = incomingData.room2LiquidTemp;
  } else if (incomingData.sensor2Status > 0) {
      // Safe fallback to ambient only if liquid sensor is disconnected
      liquidTemp = incomingData.room2Temp;
  }
  ```
* **Expected Result**: PID loop observes true 34.4 °C liquid temp, drives quartz heater to 100% duty cycle, and keeps fermentation cooling fans OFF until the liquid itself crosses 38.0 °C.

---

### Phase 2: Dual-Signal Liquid Transfer & Drain Reliability (`src/main.cpp`)

* **File**: `src/main.cpp` (Lines 2600–2635)
* **Fix Strategy**:
  1. **Primary Completion (Scale Drain)**: For Transfer 1 (Pre-Heat to Fermentation), transfer is only marked complete when `hx711Status && currentWeight <= 0.25f` (scale confirms physical drain).
  2. **Flow Sensor Dry-Run Debounce**: Increase zero-pulse timeout from 5s to **10s** (`TRANSFER_DRYRUN_TIMEOUT_MS = 10000UL`), and require scale weight `< 0.5f` before declaring a dry tank.
  3. **Transfer 2 (Fermentation to Pasteurization)**: Use flow sensor target volume (`transferVolumeTransferred >= transferTargetVolume`) with 10s zero-pulse tail debounce.
  4. **Hardware Safety Ceiling**: Maintain 5-minute maximum continuous pump runtime limit.

---

### Phase 3: Telemetry & State Exposure (`src/server.cpp` & `src/main.cpp`)

#### A. Explicit Stage String Mapping
When `stageTransferring == true`, map the active stage name to:
* `XFER_PREHEAT_TO_FERM` (Transfer 1)
* `XFER_FERM_TO_PAST` (Transfer 2)

#### B. JSON `/data` Schema Expansion (`src/server.cpp`)
Add the following fields to `handleData()`:
* `"xfer"`: `stageTransferring ? 1 : 0`
* `"xfer_vol"`: `transferVolumeTransferred` (Liters moved, 2 decimals)
* `"xfer_tgt"`: `transferTargetVolume` (Target liters, 2 decimals)
* `"xfer_pct"`: `(transferTargetVolume > 0) ? (int)((transferVolumeTransferred / transferTargetVolume) * 100) : 0`
* `"p1"`: `pumpPreHeatFermOn ? 1 : 0`
* `"p2"`: `pumpFermPastOn ? 1 : 0`
* `"tgt_ph"`: `stageTargetTemp[0]` (Preheat Target, 40.0 °C)
* `"tgt_cool"`: `preheatCoolTarget` (Preheat Cool Setpoint, 38.0 °C)
* `"tgt_ferm"`: `stageTargetTemp[1]` (Fermentation Target, 38.0 °C)
* `"tgt_past"`: `stageTargetTemp[2]` (Pasteurization Target, 72.0 °C)
* `"yd_tgt"`: `targetYeastGrams` (Target Yeast, e.g. 24.0g)
* `"yd_act"`: `actualYeastDispensedGrams` (Dispensed Yeast, grams)
* `"yd_state"`: `yeastDispenserStateString` (`IDLE`, `DISPENSING`, `COMPLETE`)
* `"mix_mode"`: `currentMixerMode` (`OFF`, `MANUAL`, `AUTO`)
* `"mix_spd"`: `mixerSpeedPercent` (0–100%)
* `"mix_cyc_sec"`: `mixerCycleElapsedSeconds` (Seconds in active ON cycle)
* `"mix_tot_min"`: `mixerTotalRuntimeMinutes` (Cumulative mixing minutes)

---

### Phase 4: Local CSV & Google Sheets Pipeline Synchronization

#### A. Local Logger (`logger_daemon.py` & `logging.cpp`)
Update standard CSV headers to include the new 28-column telemetry schema.

#### B. Google Sheets Streamer (`winebrew_sheets_sync.py`)
* Update column headers in auto-tab creation (`Batch 1`, `Batch 2`...).
* Standardize column alignments (Date/Time/Stage centered, measurements right-aligned).
* Maintain the 5,000-row dynamic border extension and Row 1 frozen header.

---

## 3. Post-Batch-1 Deployment Checklist

When Batch 1 finishes:
1. [ ] Apply `main.cpp` Line 2707 sensor assignment fix.
2. [ ] Apply `main.cpp` transfer cutoff debounce and scale drain logic.
3. [ ] Update `server.cpp` with expanded 28-field `/data` JSON schema.
4. [ ] Build firmware locally:
   ```bash
   cd /home/dave/Projects/Kaong-Wine
   pio run -e esp32dev
   ```
5. [ ] Perform Over-The-Air (OTA) Flash to primary ESP32:
   ```bash
   flash-winebrew
   ```
6. [ ] Update and restart `winebrew-sheets-sync.service` on the Beelink.
7. [ ] Run `Batch 2` and verify real-time yeast grams, mixing timers, transfer progress, and true liquid heating curve in Google Sheets.
