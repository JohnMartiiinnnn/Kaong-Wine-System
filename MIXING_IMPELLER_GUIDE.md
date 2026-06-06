# Mixing Impeller — Setup, Pinout & Testing Guide

**System:** Automated Kaong Wine Brewing System  
**Component:** JGY370 12 V DC Worm Gear Motor + TB6612FN Motor Driver

Hardware testing is done on the **Secondary ESP32** using the `Motor_Test/` project. The production firmware (`src/`) runs on the Primary ESP32 — UI tests (Tests 3–5) verify software behavior only; the motor will not spin during those tests until the Primary ESP32 GPIO assignment is resolved.

---

## 1. Components Required

| Item | Specification |
| :--- | :--- |
| ESP32 (bench test) | Secondary ESP32-WROOM-32 (30-pin DevKit) |
| Motor driver | TB6612FNG dual H-bridge |
| Motor | JGY370 12 V DC worm gear, 40–55 RPM |
| Power supply | 12 V DC, minimum 2 A |
| Logic supply | 3.3 V (from ESP32 onboard regulator) |
| Wires | Jumper wires, 22 AWG or heavier for motor leads |
| Multimeter | For voltage verification |
| USB cable + PC | For Serial Monitor |
| Optional | Oscilloscope or logic analyser (PWM verification) |

---

## 2. Complete Pinout & Wiring

### 2.1 TB6612FN Pin Description

```
TB6612FN (top view, DIP/breakout orientation)
┌─────────────────────────────┐
│ VM   VCC  GND  GND  AO1    │  ← Motor A output 1
│ STBY AIN1 AIN2 PWMA AO2    │  ← Motor A output 2
│                   BIN1 BO1  │  (Channel B — unused)
│                   BIN2 BO2  │
│                   PWMB GND  │
└─────────────────────────────┘
```

### 2.2 Full Wiring Table (Bench Test — Secondary ESP32)

| TB6612FN Pin | Connects to | Wire color (suggested) | Notes |
| :--- | :--- | :--- | :--- |
| **VM** | 12 V supply (+) | Red | Motor power — must be ≥ 12 V |
| **VCC** | 3.3 V (ESP32 3V3 pin) | Orange | Logic power |
| **GND** | Common GND | Black | Tie to ESP32 GND and 12 V supply GND |
| **STBY** | 3.3 V (ESP32 3V3 pin) | Orange | Hardwired HIGH — driver always active |
| **AIN1** | **GPIO26** (Secondary ESP32) | Yellow | Direction — HIGH = CW, LOW = CCW |
| **AIN2** | **GPIO27** (Secondary ESP32) | Yellow | Direction — LOW = CW, HIGH = CCW |
| **PWMA** | **GPIO25** (Secondary ESP32) | Blue | PWM speed signal from firmware |
| **AO1** | Motor **+** terminal | Red | JGY370 positive lead |
| **AO2** | Motor **–** terminal | Black | JGY370 negative lead |

> Channel B (BIN1, BIN2, PWMB, BO1, BO2) is **not used**. Leave floating or tie to GND.

### 2.3 GPIO25 Location on Secondary ESP32 DevKit (30-pin)

GPIO25 is on the **left side**, 9th pin from the top (counting 3V3 as pin 1).

```
Secondary ESP32 — 30-pin DevKit
         3V3  [ 1]  [30] GND
          EN  [ 2]  [29] GPIO23
       GPIO36 [ 3]  [28] GPIO22
       GPIO39 [ 4]  [27] GPIO1  (TX0)
       GPIO34 [ 5]  [26] GPIO3  (RX0)
       GPIO35 [ 6]  [25] GPIO21
       GPIO32 [ 7]  [24] GND
       GPIO33 [ 8]  [23] GPIO19
       GPIO25 [ 9]  [22] GPIO18  ← PWMA connects here
       GPIO26 [10]  [21] GPIO5
       GPIO27 [11]  [20] GPIO17
       GPIO14 [12]  [19] GPIO16
       GPIO12 [13]  [18] GPIO4
         GND  [14]  [17] GPIO0
       GPIO13 [15]  [16] GPIO2
```

### 2.4 Power Supply Wiring

```
12 V DC Supply
  (+) ──────── VM  (TB6612FN)
  (–) ──────── GND (TB6612FN)
       │
       └──────── GND (ESP32)   ← common ground is required

3.3 V (from ESP32 3V3 pin)
  ──────────── VCC  (TB6612FN)
  ──────────── STBY (TB6612FN)

GPIO26 ──────── AIN1 (TB6612FN)   ← firmware-controlled direction
GPIO27 ──────── AIN2 (TB6612FN)   ← firmware-controlled direction
```

### 2.5 Wiring Diagram — Bench Test (Secondary ESP32, GPIO25)

```
Secondary ESP32              TB6612FN                JGY370 Motor
───────────────              ────────                ────────────
3V3  ──────────────────────► VCC
3V3  ──────────────────────► STBY
GPIO26 ────────────────────► AIN1
GPIO27 ────────────────────► AIN2
GND  ──────────────────────► GND
GPIO25 ────────────────────► PWMA
                             AO1  ─────────────────► Motor (+)
                             AO2  ─────────────────► Motor (–)

12 V supply (+) ───────────► VM
12 V supply (–) ───────────► GND (same rail as ESP32 GND)
```

---

## 3. Firmware Overview

### 3.1 Bench Test Firmware (`Motor_Test/`)

Flashed to the **Secondary ESP32**. No sensors, no BLE — motor control only.

| Parameter | Value |
| :--- | :--- |
| Project folder | `Motor_Test/` |
| GPIO pin | 25 |
| LEDC channel | 0 |
| PWM frequency | 1000 Hz |
| Resolution | 8-bit (0–255) |
| Interface | Serial Monitor at 115200 baud |

**Serial commands:**

| Input | Action |
| :--- | :--- |
| `0` – `100` + Enter | Set speed to that percentage instantly |
| `a` or `A` | Ramp cycle CW: 0 % → 100 % → 0 % in 10 % steps |
| `r` or `R` | Ramp cycle CCW: same pattern, returns to CW after |
| `s` or `S` | Stop motor immediately |
| `h` or `H` | Print help |

### 3.2 Production Firmware (`src/`)

Flashed to the **Primary ESP32**. Full system with TFT display and keypad.

| Parameter | Value |
| :--- | :--- |
| Project folder | `src/` |
| GPIO pin | TBD — Primary ESP32 has no free output pin currently assigned |
| LEDC channel | 1 |
| PWM frequency | 1000 Hz |
| Resolution | 8-bit (0–255) |
| Interface | TFT display + keypad |

**Mixer modes:**

| Mode | Behaviour |
| :--- | :--- |
| **OFF** | Motor stopped, duty = 0 |
| **MANUAL** | Speed set in 10 % steps via UP/DOWN buttons |
| **AUTO** | 5 min ON at 100 %, then 355 min OFF, repeating |

---

## 4. Pre-Test Checklist (Bench Test)

Run through this before powering up.

- [ ] 12 V supply GND and ESP32 GND share a common connection
- [ ] STBY confirmed connected to 3.3 V
- [ ] **GPIO26** connected to AIN1, **GPIO27** connected to AIN2
- [ ] **GPIO25** connected to PWMA
- [ ] Motor leads AO1/AO2 connected to JGY370 (polarity sets spin direction)
- [ ] No short between VM (12 V) and VCC (3.3 V)
- [ ] 12 V supply rated ≥ 2 A (motor stall current ~1.5 A)
- [ ] `Motor_Test/` firmware flashed to Secondary ESP32

---

## 5. Test Procedures

### Test 1 — Bare Hardware Test (no firmware, no ESP32)

Verifies the TB6612FN and motor before any firmware is involved.

**Steps:**
1. Wire TB6612FN as in Section 2 but **leave PWMA disconnected**.
2. Apply 12 V to VM and 3.3 V to VCC, STBY, AIN1.
3. Using a jumper wire, manually connect **PWMA to 3.3 V**.
   - **Expected:** Motor spins. Confirm direction is CW when viewed from above the shaft.
4. Remove the jumper from PWMA.
   - **Expected:** Motor stops.
5. If direction is wrong, swap **AO1 and AO2** at the motor terminals.

**Pass criteria:** Motor runs when PWMA is HIGH, stops when PWMA is LOW.

---

### Test 2 — Speed Control Test (Secondary ESP32, `Motor_Test/` firmware)

Verifies firmware PWM control over Serial.

**Setup:**
1. Flash `Motor_Test/` to the **Secondary ESP32**.
2. Wire TB6612FN with PWMA → **GPIO25** (Secondary ESP32).
3. Open Serial Monitor at **115200 baud**.
4. Apply 12 V to the motor supply.

**Steps:**
1. Type `50` + Enter.
   - **Expected:** Motor runs at approximately half speed. Serial prints `[MOTOR] Speed set to 50%`.
2. Type `100` + Enter.
   - **Expected:** Motor runs at full speed. Serial prints `[MOTOR] Speed set to 100%`.
3. Type `0` + Enter.
   - **Expected:** Motor stops. Serial prints `[MOTOR] Speed set to 0%`.
4. Type `a` + Enter.
   - **Expected:** Motor ramps from 0 % to 100 % in 10 % steps (800 ms per step), holds at 100 % for 3 s, then ramps back down to 0 %. Each step is printed to Serial.
5. Type `s` + Enter at any time during the ramp.
   - **Expected:** Motor stops immediately.

**Pass criteria:** Speed changes are audible/visible on the motor and each step is logged to Serial.

---

### Test 3 — Auto Schedule Test (Primary ESP32, UI only)

> **Note:** This test verifies the timing logic via the TFT display. The motor will not spin unless the Primary ESP32 GPIO assignment is resolved and the TB6612FN is wired to it.

To verify the scheduler without waiting 6 hours, temporarily shorten the timing constants in `src/config.h`:

```cpp
// Default (production):
const uint32_t MIXER_ON_MS  = 5UL * 60 * 1000;
const uint32_t MIXER_OFF_MS = 355UL * 60 * 1000;

// Temporary test values:
const uint32_t MIXER_ON_MS  = 10UL * 1000;   // 10 seconds ON
const uint32_t MIXER_OFF_MS = 20UL * 1000;   // 20 seconds OFF
```

Flash the modified firmware to the Primary ESP32, navigate to **MIXER CONTROL**, and set mode to **AUTO**. The display should show `AUTO: RUNNING` for 10 s, then `AUTO: STANDBY` for 20 s, cycling automatically.

Restore `MIXER_ON_MS` and `MIXER_OFF_MS` to production values and reflash before use.

---

### Test 4 — Full UI Test (Primary ESP32, `src/` firmware)

> **Note:** Verifies the display and keypad interface. Motor will not spin until Primary ESP32 GPIO is assigned and wired.

**UI navigation to reach MIXER CONTROL:**
```
MAIN MENU
  └─ NEW BREW or CONTINUE BREW
       └─ DASHBOARD
            └─ Navigate to FERMENTATION tab  (RIGHT or DOWN button)
                 └─ SELECT  →  enter module view
                      └─ SELECT  →  MIXER CONTROL
```

**Steps:**
1. Open MIXER CONTROL as above.
2. Press SELECT → mode changes to **MANUAL**.
   - **Expected:** Display shows `MANUAL`.
3. Press DOWN three times → speed decreases to 70 %.
   - **Expected:** Display shows `70%`.
4. Press DOWN until 0 %.
   - **Expected:** Display shows `MANUAL: 0%`.
5. Press UP twice → speed increases to 20 %.
   - **Expected:** Display shows `MANUAL: 20%`.
6. Press LEFT → exit to Fermentation module view.
   - **Expected:** Live MIXER row shows `MANUAL: 20%`.
7. Press SELECT again → re-enter MIXER CONTROL.
8. Press SELECT → mode changes to **AUTO**.
   - **Expected:** Display shows `AUTO` and status tile shows `RUNNING`.
9. Press LEFT to exit.
   - **Expected:** MIXER row shows `AUTO: RUNNING`.

**Pass criteria:** Speed and mode changes reflect correctly on the display.

---

### Test 5 — Emergency Stop Test (Primary ESP32 only)

**Steps:**
1. Set mixer to MANUAL at any non-zero speed.
2. Press the **ESTOP button** (MCP23017 GPB6 on Primary ESP32).
   - **Expected:** Confirmation screen appears.
3. Press ESTOP again to confirm halt.
   - **Expected:** Motor stops immediately (if wired). Screen turns red showing `SYSTEM HALTED`.
4. Confirm motor does not restart.
5. Press EN/RST to reboot.
   - **Expected:** System restarts normally. Mixer defaults to OFF.

**Pass criteria:** Mixer mode resets to OFF on E-Stop and does not resume until manually re-enabled after reboot.

---

## 6. Debugging

### 6.1 Motor Does Not Spin

| Symptom | Likely cause | Action |
| :--- | :--- | :--- |
| Silent at any speed command | PWMA wire disconnected or wrong GPIO | Confirm the wire is on **GPIO25** on the Secondary ESP32. Probe with a multimeter — at 50 % speed it should read ~1.65 V average. |
| Silent, TB6612FN warm/hot | STBY or AIN1 not HIGH | Measure STBY and AIN1. Both must read 3.3 V. |
| Silent, TB6612FN cool | 12 V rail absent | Measure VM — should read 12 V. Check supply and wiring. |
| Driver shuts down (hot to touch) | Sustained stall | Check impeller for mechanical obstruction. |

### 6.2 Motor Runs Wrong Direction

Swap **AO1 and AO2** at the motor terminals. AIN1/AIN2 are hardwired and cannot be changed in firmware.

### 6.3 Bench Test — Serial Commands Not Working

- Confirm baud rate is **115200** in Serial Monitor.
- Confirm Serial Monitor line ending is set to **Newline** (or **Both NL & CR**).
- Type just the number (e.g. `75`) and press Enter — no extra characters.

### 6.4 UI Test — Speed Control Has No Effect on Display

- Confirm mode is **MANUAL**. UP/DOWN only adjusts speed in MANUAL mode.
- In AUTO mode the firmware controls speed internally (always 100 % when ON).

### 6.5 PWM Verification with Oscilloscope (Bench Test)

| GPIO | Speed | Expected waveform |
| :--- | :--- | :--- |
| 25 | 0 % | Constant LOW (0 V) |
| 25 | 50 % | 1 kHz square wave, 50 % duty (~1.65 V avg) |
| 25 | 100 % | Constant HIGH (3.3 V) |

---

## 7. Known Constraints

| Constraint | Detail |
| :--- | :--- |
| **Primary ESP32 GPIO pending** | All output-capable GPIO pins on the Primary ESP32 are occupied by existing peripherals. The production motor PWM pin assignment must be resolved before hardware motor testing on the Primary is possible. |
| **Coast stop only** | At 0 % duty (PWMA LOW), the driver enters coast mode, not active brake. The worm gear self-locks, so the impeller will not back-drive. |
| **Single channel** | Only TB6612FN channel A is used (1.2 A continuous, 3.2 A peak). Channel B is unused. Bridge both channels in parallel for 2.4 A if needed. |
| **Fixed direction** | AIN1/AIN2 are hardwired for CW only. To reverse, swap AO1 and AO2 at the motor terminals. |
| **No auto-restart after E-Stop** | E-Stop sets mixer to OFF. Must be manually re-enabled from the UI after rebooting the Primary ESP32. |
| **Bench test has no auto schedule** | The `Motor_Test/` sketch does not implement the 5 min/355 min cycle. Auto schedule behavior is verified through the Primary ESP32 UI (Test 3). |
