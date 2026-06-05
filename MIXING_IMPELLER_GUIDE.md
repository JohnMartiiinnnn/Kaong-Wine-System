# Mixing Impeller — Setup, Pinout & Testing Guide

**System:** Automated Kaong Wine Brewing System  
**Component:** JGY370 12 V DC Worm Gear Motor + TB6612FN Motor Driver  
**Controller:** Primary ESP32 (main firmware in `src/`)

---

## 1. Components Required

| Item | Specification |
| :--- | :--- |
| Microcontroller | ESP32-WROOM-32 (30-pin DevKit) |
| Motor driver | TB6612FNG dual H-bridge |
| Motor | JGY370 12 V DC worm gear, 40–55 RPM |
| Power supply | 12 V DC, minimum 2 A |
| Logic supply | 3.3 V (from ESP32 onboard regulator) |
| Wires | Jumper wires, 22 AWG or heavier for motor leads |
| Multimeter | For voltage verification |
| Optional | Oscilloscope or logic analyser (for PWM verification) |

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

### 2.2 Full Wiring Table

| TB6612FN Pin | Connects to | Wire color (suggested) | Notes |
| :--- | :--- | :--- | :--- |
| **VM** | 12 V supply (+) | Red | Motor power — must be ≥ 12 V |
| **VCC** | 3.3 V (ESP32 3V3 pin) | Orange | Logic power |
| **GND** | Common GND | Black | Tie to ESP32 GND and 12 V supply GND |
| **STBY** | 3.3 V (ESP32 3V3 pin) | Orange | Hardwired HIGH — driver always active |
| **AIN1** | 3.3 V (ESP32 3V3 pin) | Yellow | Hardwired HIGH — sets CW direction |
| **AIN2** | GND | Black | Hardwired LOW — sets CW direction |
| **PWMA** | ESP32 **GPIO0** | Blue | PWM speed signal from firmware |
| **AO1** | Motor **+** terminal | Red | JGY370 positive lead |
| **AO2** | Motor **–** terminal | Black | JGY370 negative lead |

> Channel B (BIN1, BIN2, PWMB, BO1, BO2) is **not used**. Leave floating or tie to GND.

### 2.3 ESP32 Pin Reference

```
ESP32 DevKit (30-pin), relevant pins only:

                    ┌──────────┐
              GND ──┤          ├── 3V3
              ...   │ ESP32    │   ...
        GPIO0 (0) ──┤  WROOM   ├── ...   ← PWMA to TB6612FN
              ...   │   32     │   ...
              GND ──┤          ├── 3V3
                    └──────────┘

GPIO0 location: left column, near the bottom of the 30-pin DevKit.
Label on board: "0" or "D0" depending on manufacturer.
```

### 2.4 Power Supply Wiring

```
12 V DC Supply
  (+) ─────────────────────── VM  (TB6612FN)
  (–) ─────────────────────── GND (TB6612FN)
                  │
                  └─────────── GND (ESP32)   ← common ground required

3.3 V (from ESP32 3V3 pin)
  ─────────────────────────── VCC  (TB6612FN)
  ─────────────────────────── STBY (TB6612FN)
  ─────────────────────────── AIN1 (TB6612FN)
```

### 2.5 Complete Wiring Diagram (ASCII)

```
ESP32                        TB6612FN                JGY370 Motor
──────                       ────────                ────────────
3V3  ──────────────────────► VCC
3V3  ──────────────────────► STBY
3V3  ──────────────────────► AIN1
GND  ──────────────────────► AIN2
GND  ──────────────────────► GND
GPIO0 ─────────────────────► PWMA
                             AO1  ─────────────────► Motor (+)
                             AO2  ─────────────────► Motor (–)

12 V supply (+) ───────────► VM
12 V supply (–) ───────────► GND (same rail as ESP32 GND)
```

---

## 3. Firmware Overview

The motor is controlled via LEDC (hardware PWM) on channel 1:

| Parameter | Value |
| :--- | :--- |
| GPIO pin | 0 |
| LEDC channel | 1 |
| PWM frequency | 1000 Hz |
| Resolution | 8-bit (0–255) |
| 0 % speed | duty = 0 (motor stopped) |
| 100 % speed | duty = 255 (full rated RPM) |

The firmware exposes three mixer modes:

| Mode | Behaviour |
| :--- | :--- |
| **OFF** | Motor stopped, duty = 0 |
| **MANUAL** | Operator sets speed in 10 % increments via UP/DOWN buttons |
| **AUTO** | 5 minutes ON at 100 %, then 355 minutes (5 h 55 m) OFF, repeating |

Auto mode starts the first ON period immediately upon activation.

---

## 4. Pre-Test Checklist

Before powering up, verify the following:

- [ ] 12 V supply GND and ESP32 GND share a common connection
- [ ] STBY, AIN1 confirmed connected to 3.3 V
- [ ] AIN2 confirmed connected to GND
- [ ] GPIO0 wire runs from ESP32 to TB6612FN PWMA
- [ ] Motor leads AO1/AO2 connected to JGY370 (polarity sets direction)
- [ ] No short between VM (12 V) and VCC (3.3 V)
- [ ] 12 V supply rated ≥ 2 A (motor stall current is ~1.5 A)

---

## 5. Test Procedures

### Test 1 — Bench Power Test (no firmware required)

Verifies the TB6612FN and motor are wired correctly before involving the ESP32.

**Setup:**
1. Connect TB6612FN as in Section 2 but **do not connect GPIO0 yet**.
2. Apply 12 V and 3.3 V power.

**Steps:**
1. With a jumper wire, manually connect **PWMA to 3.3 V**.
2. **Expected:** Motor spins. Confirm direction is CW when viewed from above the shaft.
3. Remove the jumper from PWMA (leave floating / connect to GND).
4. **Expected:** Motor stops.
5. If direction is wrong, swap the AO1 and AO2 leads at the motor terminals.

**Pass criteria:** Motor runs when PWMA is HIGH, stops when PWMA is LOW.

---

### Test 2 — Manual Mode Test (firmware running)

Verifies firmware control via the UI.

**Setup:**
1. Flash the firmware and connect GPIO0 to PWMA.
2. Power on the ESP32 and 12 V supply.

**Steps:**
1. On the TFT display, navigate: `MAIN MENU → CONTINUE BREW (or NEW BREW)`
2. Enter the `DASHBOARD` and navigate to the **FERMENTATION** tab (press RIGHT or DOWN).
3. Press **SELECT** to enter the Fermentation module view.
4. Press **SELECT** again to open **MIXER CONTROL**.
5. Press **SELECT** → mode changes to **MANUAL**. Motor starts at 100 %.
   - **Expected:** Motor runs, display shows `MANUAL`.
6. Press **DOWN** three times → speed decreases to 70 %.
   - **Expected:** Motor slows noticeably. Display shows `70%`.
7. Press **DOWN** until speed reaches 0 %.
   - **Expected:** Motor stops. Display shows `MANUAL: 0%`.
8. Press **UP** twice → speed increases to 20 %.
   - **Expected:** Motor resumes. Display shows `MANUAL: 20%`.
9. Press **LEFT** → returns to Fermentation module view.
   - **Expected:** Motor continues at 20 %. Live MIXER row shows `MANUAL: 20%`.

**Pass criteria:** Speed changes are reflected in motor behaviour and display in real time.

---

### Test 3 — Auto Mode Test (short-cycle)

Verifies the auto-mixing scheduler. The default schedule (5 min ON / 355 min OFF) is too long to observe directly, so temporarily shorten the cycle times in firmware.

**Firmware change (revert after test):**

In `src/config.h`, temporarily change:
```cpp
// Default (production):
const uint32_t MIXER_ON_MS  = 5UL * 60 * 1000;
const uint32_t MIXER_OFF_MS = 355UL * 60 * 1000;

// Temporary test values:
const uint32_t MIXER_ON_MS  = 10UL * 1000;   // 10 seconds ON
const uint32_t MIXER_OFF_MS = 20UL * 1000;   // 20 seconds OFF
```

Flash the modified firmware.

**Steps:**
1. Navigate to MIXER CONTROL and press SELECT twice → mode is **AUTO**.
   - **Expected:** Motor starts immediately. Display shows `AUTO: RUNNING`.
2. Wait 10 seconds.
   - **Expected:** Motor stops. Display shows `AUTO: STANDBY`.
3. Wait 20 seconds.
   - **Expected:** Motor restarts. Display shows `AUTO: RUNNING`.
4. Confirm the cycle repeats at least twice.
5. Press SELECT → mode cycles back to **OFF**. Motor stops.

**After test:** Restore `MIXER_ON_MS` and `MIXER_OFF_MS` to production values and reflash.

**Pass criteria:** Motor follows the ON/OFF schedule automatically without any button input.

---

### Test 4 — Emergency Stop Test

Verifies the E-Stop kills the motor immediately.

**Steps:**
1. Set mixer to MANUAL at any speed. Confirm motor is running.
2. Press the **ESTOP button** (MCP23017 GPB6).
   - **Expected:** System prompts confirmation screen.
3. Press ESTOP again to confirm halt.
   - **Expected:** Motor stops immediately. Screen turns red showing `SYSTEM HALTED`.
4. Confirm motor does not restart.
5. Reboot the ESP32 (press EN/RST button).
   - **Expected:** System restarts normally. Mixer defaults to OFF.

**Pass criteria:** Motor halts within one loop iteration of E-Stop confirmation.

---

### Test 5 — Fermentation Module Live Display

Verifies the MIXER status row updates on the dashboard.

**Steps:**
1. Enter Fermentation module view (SELECT on FERMENTATION tab).
2. Open MIXER CONTROL and set to MANUAL at 60 %.
3. Exit back to Fermentation module view (LEFT).
4. Observe the bottom of the module view.
   - **Expected:** MIXER row shows `MANUAL: 60%`, updating every ~1 second.
5. Open MIXER CONTROL again, switch to AUTO.
6. Exit back to Fermentation module view.
   - **Expected:** MIXER row shows `AUTO: RUNNING` or `AUTO: STANDBY`.

**Pass criteria:** Live MIXER row reflects current mode and speed without requiring menu re-entry.

---

## 6. Debugging

### 6.1 Motor Does Not Spin

| Symptom | Likely cause | Action |
| :--- | :--- | :--- |
| Silent in MANUAL at 100 % | GPIO0 PWM not reaching driver | Probe GPIO0 with multimeter — should read ~2.5 V average at 50 % duty. If 0 V, check `ledcAttachPin(MOTOR_PWM_PIN, MOTOR_PWM_CHANNEL)` in `setup()`. |
| Silent, TB6612FN hot | STBY or AIN1 not HIGH | Measure STBY and AIN1 pins. Both must read 3.3 V. |
| Silent, TB6612FN cool | 12 V rail absent | Measure VM pin. Should be 12 V. Check power supply and wiring. |
| Spins briefly at boot then stops | Normal — GPIO0 boot state | Not a fault. Motor starts only after `setMixerSpeed(0)` runs in `setup()`. |
| Driver shuts down (thermal) | Sustained stall | Check impeller for mechanical obstruction. Stall current is ~1.5 A, within driver rating, but sustained stall will overheat. |

### 6.2 Motor Runs Wrong Direction

Swap **AO1 and AO2** at the motor connector. Do not change AIN1/AIN2 — they are hardwired.

### 6.3 Speed Control Has No Effect

- Confirm mode is **MANUAL**. UP/DOWN buttons only adjust speed in MANUAL mode.
- In AUTO mode, the firmware controls speed (always 100 % when running).
- In OFF mode, speed is locked at 0 %.

### 6.4 Auto Mode Never Starts

1. Confirm `MIXER_OFF_MS` in `config.h` is `355UL * 60 * 1000` (not accidentally 0 or negative).
2. When AUTO is first activated, `mixerCycleTimer` is set to 0, so the first ON period starts immediately. If the motor still does not start, check that the mode toggle reached AUTO (display should show `AUTO`).
3. Verify `MIXER_ON_MS` is non-zero.

### 6.5 GPIO0 Causes Boot Issues

If the ESP32 enters bootloader mode instead of running firmware on power-on:

- GPIO0 is being held LOW at boot time.
- Check that the TB6612FN is not pulling GPIO0 LOW. The PWMA input on the TB6612FN is typically high-impedance; it should not load GPIO0 below the boot threshold (~0.8 V).
- If using a long wire to PWMA, add a 10 kΩ pull-up resistor from GPIO0 to 3.3 V to ensure it is HIGH during boot.

### 6.6 PWM Verification with Oscilloscope

Connect oscilloscope probe to GPIO0 (and GND to ESP32 GND):

| Mixer state | Expected waveform |
| :--- | :--- |
| OFF / 0 % | Constant LOW (0 V) |
| MANUAL 50 % | 1 kHz square wave, 50 % duty (~1.65 V average) |
| MANUAL 100 % | Constant HIGH (3.3 V) |
| AUTO: RUNNING | Constant HIGH (3.3 V) |
| AUTO: STANDBY | Constant LOW (0 V) |

---

## 7. Known Constraints

| Constraint | Detail |
| :--- | :--- |
| **GPIO0 is boot pin** | Do not hold GPIO0 LOW externally during power-on reset. Disconnect TB6612FN PWMA or add a 10 kΩ pull-up if boot issues occur. |
| **Coast stop only** | At 0 % duty (PWMA LOW), the driver enters coast mode — not active brake. The worm gear's self-locking prevents back-driving, so this is safe for the impeller. |
| **Single channel** | Only TB6612FN channel A is used (1.2 A continuous, 3.2 A peak). Channel B is unused. For higher current, bridge both channels in parallel for 2.4 A continuous. |
| **Fixed direction** | AIN1/AIN2 are hardwired for CW only. Reversing direction requires swapping AO1/AO2 at the motor terminals — no firmware change needed. |
| **No auto-restart after E-Stop** | E-Stop sets `currentMixerMode = MIXER_OFF`. The mixer must be manually re-enabled from the UI after a system reboot. |
