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
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Wire.h>

// ---- Pin Constants ----
const int DIM1_CH1 = 14;
const int DIM1_CH2 = 12;
const int DIM2_SHARED = 13;
const int AC_ZC_PIN = 32;
const int ONE_WIRE_BUS = 26;

// MCP23017 pin numbers (GPA = 0-7, GPB = 8-15)
static const int RELAY_PINS[8] = {8, 9, 10, 11, 12, 13, 14, 15};
const int FAN_RELAY_PIN = 7;     // GPA7 (Pre-Heating)
const int FERM_FAN_RELAY_PIN  = 8; // GPB0
const int FERM_FAN2_RELAY_PIN = 9; // GPB1
const int LIGHT_R = 15;          // GPB7
const int LIGHT_Y = 14;          // GPB6
const int LIGHT_G = 13;          // GPB5
const int BTN_RIGHT_PIN = 0;     // GPA0
const int BTN_LEFT_PIN = 1;      // GPA1
const int BTN_UP_PIN = 2;        // GPA2
const int BTN_DOWN_PIN = 3;      // GPA3
const int BTN_SELECT_PIN = 4;    // GPA4
const int ESTOP_BUTTON_PIN = 6;  // GPA6

#define RELAY_ON LOW
#define RELAY_OFF HIGH
#define GRAVITY_OFFSET 0.009

const float PID_KP = 2.0f;
const float PID_KI = 0.05f;
const float PID_KD = 1.0f;
const float PID_THROTTLE_TEMP = 75.0f;

const int PWM_PIN = 25;
const int SD_CS_PIN = 5;
const int pwmFreq = 25000;
const int pwmChannel = 0;
const int pwmResolution = 8;
// TODO: No free output GPIO exists on Primary ESP32 — resolve pin before production use.
const int MOTOR_PWM_PIN     = -1;
const int MOTOR_PWM_CHANNEL = 1;
const int MOTOR_PWM_FREQ    = 1000;
const int HX711_DT_PIN = 36;
const int HX711_SCK_PIN = 27;
const int CENTER_X = 160;

// ---- Enums ----
enum FanMode { FAN_OFF, FAN_ON, FAN_AUTO };
enum MixerMode { MIXER_OFF, MIXER_MANUAL, MIXER_AUTO };

enum AppState {
  SYSTEM_INIT,
  START_MENU,
  NEW_BREW_WIZARD,
  DASHBOARD_ACTIVE,
  COOLING_MENU,
  MIXER_MENU,
  SENSOR_MONITOR,
  CALIBRATION_MODE,
  LOAD_CELL_PAGE,
  SYSTEM_CHECK_MENU,
  FAN_TEST_PICK,
  FAN_TEST_MENU,
  LIGHT_TEST_MENU,
  RELAY_TEST_MENU,
  MOTOR_TEST_MENU,
  STAGE_PARAM_MENU,
  PID_TEST_PICK,
  PID_TEST_MENU
};

extern long rawHX711;

const uint32_t MIXER_ON_MS  = 5UL * 60 * 1000;
const uint32_t MIXER_OFF_MS = 355UL * 60 * 1000;

// ---- Motor Command Struct (Main -> Secondary via UART) ----
typedef struct __attribute__((packed)) {
  uint32_t signature; // 0xC0DEBABE
  uint8_t  motorSpeed; // 0-100
  uint8_t  motorCW;    // 1=CW, 0=CCW
  uint8_t  checksum;
} motor_cmd_t;

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
extern float originalGravity;
extern bool ogCapturing;
extern int ogSampleCount;
extern float ogSampleSum;
extern String currentLogFile;
extern char lastLogTime[10];
extern char brewStartTime[32];

// ---- Actuator State ----
extern int currentSpeedPercent;
extern int currentHeatingPercent;
extern bool isFanOn;
extern bool isFermFanOn;
extern bool isLight1On;
extern bool isLight2On;
extern bool isLight3On;
extern FanMode currentFanMode;
extern bool isSystemHalted;
extern int estopState;
extern uint32_t estopTimer;
extern int returnConfirmState;
extern uint32_t returnConfirmTimer;
extern int mixerSpeedPercent;
extern MixerMode currentMixerMode;
extern bool mixerRunning;
extern uint32_t mixerOnTimer;
extern uint32_t mixerCycleTimer;

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
extern bool loadCellNeedsFullRedraw;
extern int loadCellSelection;
extern int systemCheckSelection;
extern int fanTestSelection;
extern int fanTestRow;
extern int fanTestFanChoice;
extern int fanTestSpeed;
extern int lightTestSelection;
extern bool systemCheckNeedsFullRedraw;
extern bool fanTestNeedsFullRedraw;
extern bool lightTestNeedsFullRedraw;
extern bool relayTestNeedsFullRedraw;
extern int  relayTestChannel;
extern uint32_t relayTestTimer;
extern bool motorTestNeedsFullRedraw;
extern int  motorTestSpeed;
extern bool motorTestCW;
extern bool pidTestNeedsFullRedraw;
extern int pidTestChoice;
extern float pidTestTarget;
extern bool pidTestRunning;
extern bool pidTestSuccess;
extern uint32_t pidTestStableStart;

// ---- Button Latch State ----
extern bool ljRight, ljLeft, ljUp, ljDown, ljSelect;
extern bool lastLjRight, lastLjLeft, lastLjUp, lastLjDown, lastLjSelect;

// ---- Brew Stage & Stage Params ----
extern int activeBrewStage;
extern uint32_t stageStartMillis;
extern float stageTargetTemp[3];
extern float fermTargetPH;
extern float fermTargetGravity;
extern int stageParamSelection;
extern bool stageParamNeedsFullRedraw;
extern int stageParamStage;
extern bool preHeatSterilized;
extern bool preHeatHolding;
extern uint32_t preHeatHoldStart;
extern bool pastSterilized;
extern bool pastHolding;
extern uint32_t pastHoldStart;
extern bool phAlertActive;
extern float simTempOverride[3];
extern bool  simDynamic[3];
