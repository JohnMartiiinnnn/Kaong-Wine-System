# Kaong Wine System — Primary ESP32 Pinout

## Direct ESP32 GPIO

| GPIO | Signal | Notes |
|------|--------|-------|
| 2 | TFT LCD DC | SPI |
| 4 | TFT LCD RST | SPI |
| 5 | SD  Card CS | SPI |
| 12 | Fermentation heater SSR (`SSR_FERM`) | SSR control signal |
| 13 | Pre-heat heater SSR (`SSR_PREHEAT`) | SSR control signal |
| 14 | Pasteurization heater SSR (`SSR_PAST`) | SSR control signal |
| 15 | TFT LCD CS | SPI |
| 16 | UART2 RX | Serial2 — from Secondary ESP32 |
| 17 | UART2 TX | Serial2 — to Secondary ESP32 |
| 18 | SPI SCLK | Shared: SD, TFT, Touchscreen |
| 19 | SPI MISO | SD Card (TFT MISO disconnected) |
| 21 | I2C SDA | RTC DS3231 + MCP23017 |
| 22 | I2C SCL | RTC DS3231 + MCP23017 |
| 23 | SPI MOSI | Shared: SD, TFT, Touchscreen |
| 25 | Heater/Fan PWM (`PWM_PIN`) | LEDC ch0, 25 kHz, 8-bit |
| 26 | OneWire Bus (`ONE_WIRE_BUS`) | DS18B20 (shared bus, both liquid temp sensors) |
| 27 | HX711 SCK (`HX711_SCK_PIN`) | Load cell clock |
| 32 | Flow sensor 1 (`FLOW_PREHEAT_FERM`) | INPUT_PULLUP, pulse input — pre-heat → ferm |
| 34 | Flow sensor 2 (`FLOW_FERM_PAST`) | INPUT_PULLUP, pulse input — ferm → past (input-only pin) |
| 33 | Touchscreen CS | SPI |
| 36 | HX711 DT (`HX711_DT_PIN`) | Load cell data (input-only pin) |
| — | Motor PWM (`MOTOR_PWM_PIN`) | Unassigned (`-1`); no free output GPIO available on Primary ESP32 |

---

## HX711 Load Cell

### HX711 → ESP32

| HX711 Pin | ESP32 GPIO | config.h name |
|-----------|-----------|---------------|
| DT (DOUT) | GPIO 36 | `HX711_DT_PIN` |
| SCK (PD_SCK) | GPIO 27 | `HX711_SCK_PIN` |
| VCC | 3.3 V | — |
| GND | GND | — |

> GPIO 36 is input-only (no internal pullup). HX711 drives DOUT actively so this is fine.

### Load Cell → HX711

| Load cell wire | HX711 terminal | Role |
|----------------|----------------|------|
| Red | E+ | Excitation + |
| Black | E- | Excitation – |
| White | A+ | Signal + |
| Green | A– | Signal – |

> Use Channel A (128× gain, default). Channel B is unused.

---

## SSR Heater Control

| config.h name | ESP32 Pin | Tank |
|---------------|-----------|------|
| `SSR_PREHEAT` | GPIO 13 | Pre-heating tank |
| `SSR_FERM` | GPIO 12 | Fermentation tank |
| `SSR_PAST` | GPIO 14 | Pasteurization tank |

All three are driven with slow PWM (time-proportional, 2 s window) via `digitalWrite()`. SSRs switch on the HIGH pulse within each window; no zero-cross input required.

## Flow Sensors

| config.h name | ESP32 Pin | Transfer | Notes |
|---------------|-----------|----------|-------|
| `FLOW_PREHEAT_FERM` | GPIO 32 | Pre-heat → Fermentation | INPUT_PULLUP, pulse counting |
| `FLOW_FERM_PAST` | GPIO 34 | Fermentation → Pasteurization | INPUT_PULLUP, pulse counting (input-only pin) |

---

## MCP23017 I/O Expander (I2C, default address 0x20)

### GPA — Buttons & Fan

| MCP Pin | config.h name | Function |
|---------|---------------|----------|
| GPA0 (0) | `BTN_RIGHT_PIN` | Keypad RIGHT |
| GPA1 (1) | `BTN_LEFT_PIN` | Keypad LEFT |
| GPA2 (2) | `BTN_UP_PIN` | Keypad UP |
| GPA3 (3) | `BTN_DOWN_PIN` | Keypad DOWN |
| GPA4 (4) | `BTN_SELECT_PIN` | Keypad SELECT |
| GPA5 (5) | — | Unused |
| GPA6 (6) | `ESTOP_BUTTON_PIN` | Emergency stop |
| GPA7 (7) | `FAN_RELAY_PIN` | Pre-heating fan relay |

### GPB — Relay Module (8-channel)

| MCP Pin | config.h name | Function |
|---------|---------------|----------|
| GPB0 (8) | `FERM_FAN_RELAY_PIN` / `FERM_FAN2_RELAY_PIN` | Fermentation fans (same relay) |
| GPB1 (9) | `PUMP_PREHEAT_FERM` | Transfer pump: pre-heat → fermentation |
| GPB2 (10) | `PUMP_FERM_PAST` | Transfer pump: fermentation → pasteurization |
| GPB3 (11) | `RELAY_PINS[3]` | Unassigned |
| GPB4 (12) | `RELAY_PINS[4]` | Unassigned |
| GPB5 (13) | `LIGHT_G` | Status light — Green |
| GPB6 (14) | `LIGHT_Y` | Status light — Yellow |
| GPB7 (15) | `LIGHT_R` | Status light — Red |

---

## Secondary ESP32 (Sensor Node)

The Secondary transmits over UART2 (115200 baud) to the Primary. Source: `Secondary_Transmitter/src/main.cpp`.

### Direct GPIO

| GPIO | Signal | Notes |
|------|--------|-------|
| 13 | OneWire Bus (`ONE_WIRE_BUS`) | DS18B20 fermentation liquid temp; 4.7 kΩ pullup to 3.3 V |
| 16 | UART2 RX | Serial2 — from Primary ESP32 (crosses to Primary TX=17) |
| 17 | UART2 TX | Serial2 — to Primary ESP32 (crosses to Primary RX=16) |
| 21 | I2C SDA | BME280/BMP280 + ADS1115 |
| 22 | I2C SCL | BME280/BMP280 + ADS1115 |

> **BLE:** NimBLE uses the built-in radio — no external GPIO required.

---

### I2C Devices

| Device | Address | Function | Address config |
|--------|---------|----------|---------------|
| BME280 or BMP280 | 0x77 | Fermentation ambient temp & pressure | CSB → 3.3 V, SDO → 3.3 V |
| ADS1115 | 0x48 | 16-bit ADC for pH sensor | ADDR → GND (default) |

### Analog Input

| ADS1115 Channel | Source | Function |
|-----------------|--------|----------|
| A0 | PH4502C `Po` pin | pH voltage output |

---

### BTS7960 Motor Driver

| BTS7960 Pin | Secondary ESP32 GPIO | Role |
|-------------|----------------------|------|
| LPWM | GPIO 25 | Forward PWM Speed Control |
| RPWM | GPIO 26 | Reverse PWM Speed Control |
| L_EN | 3.3V / 5V | Left Enable (must be HIGH) |
| R_EN | 3.3V / 5V | Right Enable (must be HIGH) |
| VCC | 3.3V / 5V | ESP32 Logic Power |
| GND | GND | Common Ground |
| R_IS | NC | Right Current Sense (Optional) |
| L_IS | NC | Left Current Sense (Optional) |

> **Note:** `L_EN` and `R_EN` are typically shorted together with a jumper. You can leave the jumper attached and connect a single wire from either pin to 3.3V on the ESP32.

