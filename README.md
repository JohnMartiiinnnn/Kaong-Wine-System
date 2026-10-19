# Automated Kaong Wine Brewing System

[![PlatformIO CI](https://img.shields.io/badge/PlatformIO-ESP32-orange.svg)](https://platformio.org/)
[![Dual-ESP32 Architecture](https://img.shields.io/badge/Architecture-Dual--ESP32%20UART-blue.svg)]()
[![OTA Enabled](https://img.shields.io/badge/OTA-Enabled-green.svg)]()

Production and thesis repository for the automated, sensor-monitored Kaong (Sugar Palm) Wine Brewing System.

---

## Quick Navigation

* [SYSTEMS.md](file:///home/dave/Projects/Kaong-Wine/SYSTEMS.md) : Master architecture, hardware topology, and deployment commands.
* [FERMENTATION_RUN_ALIGNMENT_RECORD.md](file:///home/dave/Projects/Kaong-Wine/FERMENTATION_RUN_ALIGNMENT_RECORD.md) : Comprehensive alignment record, CvSU 2014 study analysis, and dual-batch fermentation protocol.
* [SYSTEM_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/SYSTEM_GUIDE.md) : Complete user manual covering all screens, keypad controls, and brewing workflows.
* [PINOUT.md](file:///home/dave/Projects/Kaong-Wine/PINOUT.md) : Comprehensive electrical pin mappings for both ESP32 controllers.
* [CALIBRATION_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/CALIBRATION_GUIDE.md) : Sensor calibration procedures for pH, load cell, and flow meters.
* [YEAST_DISPENSER_GUIDE.md](file:///home/dave/Projects/Kaong-Wine/YEAST_DISPENSER_GUIDE.md) : Automated yeast dispensing motor and calibration guide.
* [AGENTS.md](file:///home/dave/Projects/Kaong-Wine/AGENTS.md) : System rules, display guidelines, and agent instructions.

---

## Deployment Quick Reference

### Primary ESP32 (Wireless OTA)
```bash
# Automated one-click OTA deployment from Beelink (pulls git, compiles, flashes, verifies telemetry)
flash-winebrew

# Direct PlatformIO OTA via LAN IP or mDNS
~/.platformio/penv/bin/pio run -t upload --upload-port 192.168.1.137
~/.platformio/penv/bin/pio run -t upload --upload-port winebrew-main.local
```

### Primary ESP32 (USB Auto-Detect)
```bash
~/.platformio/penv/bin/pio run -t upload
```

### Secondary ESP32 (USB)
```bash
~/.platformio/penv/bin/pio run -d Secondary_Transmitter -e esp32dev -t upload
```

### Live Telemetry (CLI)
```bash
curl -s http://192.168.1.137/data
curl -s http://winebrew-main.local/data
```

---

## System Architecture

The brewing machine operates on a distributed dual-ESP32 architecture interconnected via hardware UART (Serial2, 115200 baud, 8N1):

* **Primary Controller (Master - `src/`)**
  * Hardware: ESP32 DevKit v1 with 4MB Flash and dual-OTA partitions (`min_spiffs.csv`).
  * Peripherals: 3.5" ILI9488 SPI TFT LCD (320x480), MCP23017 I2C GPIO expander (5-button keypad and 8-channel relay manifold), DS3231 precision RTC, SD card SPI logger, and three high-power SSR drivers (GPIO 12, 13, 14).
  * Network: Multi-AP STA failover (`Bedroom Deco`, `Living-Room-WiFi`, `Infinix Hot 60 Pro` hotspot, `CvSU-ICT-WiFi`) plus fallback SoftAP `WineBrew_System` (192.168.4.1).
  * Responsibilities: Brewing state machine orchestration, thermal PID loops, automated fluid transfer logic, safety interlocks, and local HTTP REST telemetry.

* **Secondary Controller (Sensor & Actuator Node - `Secondary_Transmitter/`)**
  * Hardware: ESP32 DevKit v1 / ESP32-C3 inside the fermentation vessel.
  * Peripherals: ADS1115 16-bit ADC reading PH4502C analog probe, DS18B20 OneWire liquid probe, BME280 ambient sensor, NimBLE RAPT Pill digital hydrometer scanner, BTS7960 43A motor driver (mixing impeller), and DRV8871 H-bridge (automated yeast dispenser).
  * Responsibilities: High-resolution analog sensor burst sampling, outlier filtering, BLE packet decoding, and continuous telemetry transmission to Primary.

---

## Signal Processing & Safety Interlocks

* **Dual-Layer pH Filtering & Terminal Acidity Debounce**
  * Layer 1 (Secondary Transmitter): Executes 10-sample ADC burst reads at 5ms spacing on ADS1115 channel 0. Applies a trimmed mean by discarding the two lowest and two highest samples, then averages the central six. An Exponential Moving Average (EMA, alpha 0.30) smooths drift.
  * Layer 2 (Primary Controller): Feeds incoming UART telemetry into an 11-sample rolling circular window median filter and assigns the filtered output directly to incomingData.phValue, delivering stable, noise-immune readings across all UI screens (PH & FERM TEMP, Live Dashboard) and web REST endpoints.
  * Persistence Debounce: Terminal acidity cutoff (pH <= fermTargetPH, nominal target 3.50 for finished wine vs 3.00 vinegar boundary) requires a sustained 10.0-second persistence timer before tripping Stage 1 advancement to pasteurization.
* **5.0L Chamber Volume Interlock**
  * Enforced at the hardware driver level before any SSR can fire. Chamber 1 requires scale reading >= 5.0 kg; Chamber 2 and Chamber 3 require accumulated flow transfer volume >= 5.0 L.
* **Dynamic 2.0 °C / Second Rate-of-Rise (RoR) Cutoff**
  * Continuous heater temperature monitoring trips `dryElementAlarm` if temperature climbs by >= 2.0 °C in one second, shutting down heating to prevent dry-fire coil damage.
* **Drain-to-Empty Flow Verification (< 0.15L Delta)**
  * Fluid transfers rely on continuous flow sensor pulses. A 15-second rolling settle window with volume delta < 0.05L confirms vessel evacuation before pump disengagement.
* **Pasteurization Trigger Interlock (Test 13 / Table 20)**
  * Automated 10-cycle system check verifying heater activation and cutoff at 60.0 °C setpoint with rate-of-rise validation.
* **RAPT Pill Telemetry Validation Bounds & Freshness Timeout**
  * Specific gravity strictly validated within physical hydrometer bounds (0.980 to 1.150 SG) and temperature within -5.0 °C to 60.0 °C. Stale BLE packets exceeding the freshness timeout are invalidated across TFT displays, REST endpoints (/data), and SD CSV telemetry logs to prevent freeze-state errors.

---

## Dual-Batch Testing Protocol & 2014 CvSU Study Baseline

To satisfy thesis Objective 4 and provide rigorous empirical data for the Independent Samples t-Test, the experimental design compares two distinct batches:

* **Batch 1: Traditional Wild Control (CvSU 2014 Baseline)**
  * Method: 100% natural wild fermentation using indigenous sap microflora, matching the 2014 CvSU thesis study (FERMENTATION-VESSELS-FOR-SUGAR-PALM-WINE-PRODUCTION.pdf).
  * Inoculation: Zero added yeast, zero pre-heating.
  * Thermal Profile: Unregulated ambient room temperature (27 °C to 31 °C).
  * Stabilization: Cold-stabilized below 4 °C upon reaching target acidity (pH 3.6 to 3.8) to halt acetobacter proliferation.
  * Baseline Insight: Confirms natural fermentation dynamics without thermal shock, validating traditional Cavite table wine production.

* **Batch 2: Automated Prototype (Engineered Inoculation)**
  * Method: Closed-loop automated brewing process inside the dual-ESP32 prototype.
  * Sterilization: 80 °C pre-heat sterilization in Chamber 1, followed by automated fan cooling to 28 °C to 30 °C.
  * Inoculation: Automated DRV8871 dispenser pitches pure Saccharomyces cerevisiae (Lalvin EC-1118) wine yeast.
  * Thermal Profile: Closed-loop PID control maintaining 25 °C to 30 °C via radiant quartz heater and cross-flow fans.
  * Thermal Arrest: Automated inter-chamber transfer to Chamber 3 for pasteurization at 60 °C (10 to 15 minute hold) upon reaching target pH 3.5.

* **Acidity Threshold Clarification (Wine vs Vinegar)**
  * Kaong Wine Target: pH 3.40 to 3.90 (optimal cutoff at pH 3.50).
  * Kaong Vinegar Boundary: pH 2.00 to 3.00 (sukang kaong). Fermentation allowed to progress below pH 3.20 rapidly sour as acetic acid bacteria consume ethanol.
  * Rejection of Preheat-Without-Yeast Setup: Preheating without yeast inoculation kills natural flora and halts fermentation entirely, spoiling raw sap. Automated pitching post-cooling is mandatory.

* **Aseptic Sampling & Telemetry Measurement Protocol**
  * Day 0 Baseline (Oct 17, 2026 Kickoff): 2.5L CvSU SPRINT sap thawed to 27 °C ambient. Verified parameters: starting density 1.070 Original Gravity (17.1 °Brix / 9.2% Potential ABV) and baseline pH 5.80 to 6.29. Inoculated with 0.50 g/L Lalvin EC-1118 across dual 1.0L Sprite vessels with 1/4-turn loose cap CO2 venting.
  * Day 1 Fermentation Telemetry (Oct 18, 2026): Dual 4-pad universal pH test strips confirmed terminal wine acidity window at pH 3.60 to 3.80 with dense surface krausen/foam (~1.5–2 inches) and vigorous CO2 effervescence, validating rapid palm sap acidification kinetics. Reaffirmed cold-stabilization protocol (< 4 °C in refrigeration) for Traditional Control Setup A without heat pasteurization, preserving authentic unheated baseline while preventing acetobacter vinegar progression. Heat pasteurization (60 °C hold) remains strictly dedicated to Automated Prototype Setup B (Chamber 3).
  * Day 2 Fermentation Telemetry (Oct 19, 2026, 10:49 PHT): Measurement #4 via dual 4-pad universal pH test strips confirmed acidity stably holding at pH 3.60 to 3.80 (Pad 1 amber-orange, Pad 2 golden yellow, Pad 3 cyan, Pad 4 olive green matching between columns 3 and 4). Physical krausen foam head has completely subsided into a wall residue ring, and liquid has settled into calm, cloudy must with fine carbonation bubbles, indicating transition into post-peak attenuation with zero vinegar souring.
  * Active Fermentation Checks (Traditional Control): Hydrometer extraction during active fermentation is strictly prohibited to prevent volume depletion and acetic oxidation. Daytime tracking uses micro-pours of 5 mL to 10 mL into a sanitized vessel to submerge the digital pH pen bulb. The sample is discarded immediately after logging.
  * Active Fermentation Checks (Automated Prototype): 100% non-invasive continuous telemetry. The submerged RAPT Pill broadcasts floating specific gravity and temperature over BLE to the telemetry logger without fluid loss.
  * Terminal Endpoint (Finished Wine): Upon reaching terminal attenuation (stabilized SG <= 1.010 and pH target), a final 200 mL sample is racked for terminal hydrometer reading (Final Gravity), final pH verification, and Alcohol by Volume (ABV) computation via (OG - FG) * 131.25.

* **Manuscript Alignment & Verification Status**
  * Live Google Doc Table Sequence: Continuous sequence verified from Table 1 through Table 28 with zero duplicate numbers and zero missing entries.
  * Single Comparative Run Specification: Section 3.8 explicitly establishes a single comparative trial run per method (Automated vs Traditional Control).
  * Process Consistency Definition: Clarified as continuous temporal standard deviation of pH, temperature, and specific gravity across the active fermentation curve.

---

## Field Network Configuration

When deploying at CvSU or remote field testing:
* **Mobile Hotspot**: Connect to phone hotspot SSID `Infinix Hot 60 Pro` (Password: `aaaaaaaa`).
* **Campus Wi-Fi**: Pre-configured for `CvSU-ICT-WiFi` (Password: `#cvsuictfreewifi`).
* **Standalone SoftAP**: Connect directly to `WineBrew_System` (Password: `12345678`) at `http://192.168.4.1/data`.

