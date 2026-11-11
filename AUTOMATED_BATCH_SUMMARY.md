# Automated Prototype Batch 1 (BCH-01) Benchtop Verification
**Date:** October 23, 2026 (14:03 PHT)
**Batch Code:** BCH-01 (Automated Machine Run)
**Sap Medium:** 6.86 L Fresh Kaong (*Arenga pinnata*) Sap
**Inoculation:** 3.43 g Lalvin EC-1118 (*Saccharomyces cerevisiae*)
**Runtime:** 39.0 Hours

---

### Physicochemical Parameter Scorecard

• **Alcohol by Volume (ABV)**: **7.04% ABV** (RAPT Pill continuous wireless hydrometer)
• **Starting Original Gravity (OG)**: **1.0805 SG** (~19.4 °Brix)
• **Final Specific Gravity (FG)**: **1.0177 SG** (Stabilized terminal plateau)
• **Apparent Sugar Attenuation**: **78.0%** (Full conversion of fermentable simple sugars)
• **Terminal Benchtop pH (Handheld Calibrated Meter)**: **3.95 pH** (Photo verified: photo_1792735301.jpg)
• **In-Line ADS1115 Tank pH**: ~5.12 to 5.51 pH (Subject to tank ground-loop potential offset)
• **Standard Compliance (PNS/BAFP 183:2010 - Fruit Wine)**:
  • Terminal pH: 3.80 - 4.00 Target -> **3.95 pH (100% COMPLIANT)**
  • ABV: 7.0% - 10.0% Target -> **7.04% ABV (100% COMPLIANT)**
  • Attenuation: 75% - 80% Expected -> **78.0% (100% COMPLIANT)**

---

# Automated Prototype Batch 2 (BCH-02) Mid-Run Incident & Architecture Hardening
**Date:** November 11, 2026 (Active Inoculation & Fermentation)
**Batch Code:** BCH-02 (Automated Machine Run 2)
**Telemetry Reference:** `brew_wifi_20261111_055355.csv` / Google Sheets Tab `Automated Run 2`
**Sap Medium:** 5.90 L Fresh Indang Kaong (*Arenga pinnata*) Sap
**Inoculation:** Lalvin EC-1118 (*Saccharomyces cerevisiae*)
**Operating Setpoints:** Pre-heat 63.0 °C / Cool 33.0 °C; Fermentation 30.0 °C; Pasteurization 60.0 °C

---

### Physicochemical Telemetry & Attenuation Kinetics

• **Starting Original Gravity (OG)**: **1.0809 SG** (~19.5 °Brix, 07:31 PHT)
• **Mid-Run Live Gravity**: **1.0672 SG** (Continuous downward attenuation verified post-retransfer)
• **Live Liquid Temperature**: **29.3 °C – 30.4 °C** (Closed-loop quartz PID regulation targeting 30.0 °C)
• **Current Live pH**: **5.18 – 5.24 pH** (Healthy, active primary fermentation)
• **RAPT Pill Telemetry**: BLE signal -51 to -56 dBm, battery 46%

---

### Mid-Run Hydraulic Incident Analysis & Empirical Verification

1. **Premature Auto-Advance Trigger (13:54 PHT):**
   • An electrical probe fluctuation lasting ~40 seconds dipped instantaneous pH to 3.15.
   • Because the legacy firmware evaluated only a 60-second rolling average (`ph1MinAvg <= 3.50`) without a minimum runtime check, the state machine prematurely initiated `startLiquidTransfer(2)` to empty Chamber 2 into Chamber 3.
2. **Hardware Dry-Run Safety Verification:**
   • Flow Sensor 2 (`FLOW_FERM_PAST` on GPIO 34) registered only 0.01 L of flow due to sap viscosity and floating turbine stall.
   • Pump 2 ran for ~75 seconds (45s priming grace + 30s settle window) before the firmware tripped `transferDryRunAlarm`.
   • The critical safety interlock immediately killed Pump 2 (leaving ~half the sap in Chamber 2 and ~half in Chamber 3) and forced `SSR_PAST` to 0.0% duty cycle.
   • **Heater Verification:** Telemetry confirms `PastLiquid_C` stayed strictly at 29.5 °C – 29.8 °C. The 1.5 kW pasteurization heating element **never energized**, preventing dry tank burnout.
3. **RAPT Pill Mechanical Grounding Artifact:**
   • Draining half the fluid dropped fluid depth below the minimum free-float threshold, causing the RAPT Pill tilt hydrometer to rest on the tank floor.
   • The physical angle of the grounded capsule was decoded as an artificial jump to 1.1528 SG.
   • Once the sap was poured back from Chamber 3 into Chamber 2, the buoyant depth was restored, and the hydrometer immediately normalized back to 1.0671 SG.
4. **Firmware Architecture Upgrades Deployed (Commits `fee086c` & `ab04888`):**
   • **Hardened 3-Tier Cutoff:** Enforces (1) 18-hour minimum fermentation runtime gate, (2) 15-minute continuous sustained window <= 3.50 pH with instant bounce reset, and (3) hydrometer dual-signal check (`SG <= 1.030`).
   • **Stage Reversion Controls:** Added on-device `DOWN` keypad binding to revert stages, `LEFT` keypad binding to abort transfer alarms back to Fermentation, and a Web Monitor `Revert Stage` button (`/stage/revert`).
   • **Boot Auto-Recovery:** Automatic detection and rollback to Stage 1 on boot if an interrupted transfer (<0.5 L) is detected.
   • **OTA Deployment:** Successfully flashed over-the-air to the Primary ESP32 (`192.168.1.137:3232`).
5. **Data Harmonization:**
   • Reconciled 1,680 rows in `brew_wifi_20261111_055355.csv` and Google Sheets (`Automated Run 2` & `Test Syncs v3`) from `PASTEURIZATION` to `FERMENTATION`, and interpolated the 38 grounded hydrometer samples to provide a continuous, publication-grade attenuation profile.
