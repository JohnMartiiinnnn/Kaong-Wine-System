#pragma once

// ---- Library Includes ----
#include <Adafruit_BME280.h>
#include <Adafruit_MCP23X17.h>
#include <Arduino.h>
#include <DallasTemperature.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <HX711.h>
#include <OneWire.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

// ---- Pin Constants ----
const int DIM1_CH1 = 14;
const int DIM1_CH2 = 12;
const int DIM2_SHARED = 13;
const int AC_ZC_PIN = 32;
const int ONE_WIRE_BUS = 26;

// MCP23017 pin numbers (GPA = 0-7, GPB = 8-15)
static const int RELAY_PINS[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int FAN_RELAY_PIN = 15;    // GPB7
const int BTN_RIGHT_PIN = 8;     // GPB0
const int BTN_LEFT_PIN = 9;      // GPB1
const int BTN_UP_PIN = 10;       // GPB2
const int BTN_DOWN_PIN = 11;     // GPB3
const int BTN_SELECT_PIN = 12;   // GPB4
const int ESTOP_BUTTON_PIN = 14; // GPB6

#define RELAY_ON LOW
#define RELAY_OFF HIGH

const int PWM_PIN = 25;
const int SD_CS_PIN = 5;
const int pwmFreq = 25000;
const int pwmChannel = 0;
const int pwmResolution = 8;
const int HX711_DT_PIN = 36;
const int HX711_SCK_PIN = 27;
const int CENTER_X = 160;

// ---- Enums ----
enum FanMode { FAN_OFF, FAN_ON, FAN_AUTO };

enum AppState {
  SYSTEM_INIT,
  START_MENU,
  NEW_BREW_WIZARD,
  DASHBOARD_ACTIVE,
  COOLING_MENU,
  SENSOR_MONITOR,
  CALIBRATION_MODE
};

// ---- UART Data Struct (shared with Secondary) ----
typedef struct __attribute__((packed)) {
  uint32_t signature;
  float pillTemp;
  float pillGravity;
  float room2Temp;
  float room2Pres;
  float phValue;
  float room2LiquidTemp;
  uint8_t sensor2Status;
  uint8_t adsStatus;
  uint8_t ds18Status;
  uint8_t bleStatus;
  uint8_t pillBattery;
  int16_t pillRSSI;
  uint8_t checksum;
} struct_message;

// ---- Hardware Objects (defined in main.cpp) ----
extern Adafruit_MCP23X17 mcp;
extern RTC_DS3231 rtc;
extern TFT_eSPI tft;
extern Adafruit_BME280 bme1;
extern OneWire sharedOneWire;
extern DallasTemperature sharedLiquidSensors;
extern HX711 scale;
extern WebServer server;

// ---- Sensor Status ----
extern bool bme1Status;
extern bool liquid1Status;
extern bool liquid2Status;
extern bool remoteStatusReceived;
extern bool rtcStatus;
extern bool sdStatus;
extern bool hx711Status;

// ---- Sensor & Brew Data ----
extern struct_message incomingData;
extern uint32_t lastDataReceivedMillis;
extern float currentWeight;
extern float calibrationFactor;
extern String currentLogFile;
extern char lastLogTime[10];
extern char brewStartTime[32];

// ---- Actuator State ----
extern int currentSpeedPercent;
extern int currentHeatingPercent;
extern bool isFanOn;
extern FanMode currentFanMode;
extern bool isSystemHalted;
extern int estopState;
extern uint32_t estopTimer;
extern int returnConfirmState;
extern uint32_t returnConfirmTimer;

// ---- UI / App State ----
extern AppState currentAppState;
extern bool menuNeedsFullRedraw;
extern int menuSelection;
extern bool wizardNeedsFullRedraw;
extern int wizardSelection;
extern bool dashNeedsFullRedraw;
extern int dashSelection;
extern bool moduleViewActive;
extern int lastDashSelection;
extern bool monitorNeedsFullRedraw;
extern bool calNeedsFullRedraw;
extern int calSelection;

// ---- Button Latch State ----
extern bool ljRight, ljLeft, ljUp, ljDown, ljSelect;
extern bool lastLjRight, lastLjLeft, lastLjUp, lastLjDown, lastLjSelect;
