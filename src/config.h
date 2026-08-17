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
const int SSR_PREHEAT = 13;   // Pre-heat tank heater SSR (was DIM2_SHARED)
const int SSR_FERM    = 12;   // Fermentation tank heater SSR (was DIM1_CH2)
const int SSR_PAST    = 14;   // Pasteurization tank heater SSR (was DIM1_CH1)
const int FLOW_PREHEAT_FERM = 32; // Flow sensor: pre-heat → fermentation (was AC_ZC_PIN)
const int FLOW_FERM_PAST   = 34; // Flow sensor: fermentation → pasteurization (input-only pin)
const int ONE_WIRE_BUS = 26;

// MCP23017 pin numbers (GPA = 0-7, GPB = 8-15)
static const int RELAY_PINS[8] = {8, 9, 10, 11, 12, 13, 14, 15};
const int FAN_RELAY_PIN = 7;     // GPA7 (Pre-Heating)
const int FERM_FAN_RELAY_PIN  = 8; // GPB0
const int FERM_FAN2_RELAY_PIN = 8; // GPB0 (same relay as FERM_FAN_RELAY_PIN)
const int PUMP_PREHEAT_FERM   = 9; // GPB1 — transfer pump: pre-heat → ferm
const int PUMP_FERM_PAST      = 10; // GPB2 — transfer pump: ferm → past
const int LIGHT_R = 15;          // GPB7
const int LIGHT_Y = 14;          // GPB6
const int LIGHT_G = 13;          // GPB5
const int BTN_RIGHT_PIN = 0;     // GPA0
const int BTN_LEFT_PIN = 1;      // GPA1
const int BTN_UP_PIN = 2;        // GPA2
const int BTN_DOWN_PIN = 3;      // GPA3
const int BTN_SELECT_PIN = 4;    // GPA4

#define RELAY_ON LOW
#define RELAY_OFF HIGH
#define GRAVITY_OFFSET 0.009

// ---- DRV8871 Yeast Dispenser Pinout & Calibration Parameters ----
const int DRV8871_IN1_PIN_CFG = 5; // DRV8871 IN1 -> ESP32-C3 GPIO 5 (D5)
const int DRV8871_IN2_PIN_CFG = 4; // DRV8871 IN2 -> ESP32-C3 GPIO 4 (D4)

#include "PIDController.h"
#include "YeastDispenser.h"

// ---- Immersion Heater PID Parameters (Pre-Heat & Pasteurization) ----
const float PREHEAT_PID_KP = 2.0f;
const float PREHEAT_PID_KI = 0.05f;
const float PREHEAT_PID_KD = 1.0f;
const float PREHEAT_RAMP_BAND = 5.0f;

const float PAST_PID_KP = 2.0f;
const float PAST_PID_KI = 0.05f;
const float PAST_PID_KD = 1.0f;
const float PAST_RAMP_BAND = 5.0f;

// ---- Quartz Radiant/Air Heater PID Parameters (Fermentation) ----
const float QUARTZ_PID_KP = 3.5f;
const float QUARTZ_PID_KI = 0.03f;
const float QUARTZ_PID_KD = 1.2f;
const float QUARTZ_RAMP_BAND = 3.0f;

// Shared fallback constants for legacy references
const float PID_KP = 2.0f;
const float PID_KI = 0.05f;
const float PID_KD = 1.0f;

extern PIDController preheatPid;
extern PIDController pastPid;
extern PIDController quartzPid;
extern PIDController trackingPid;


const int PWM_PIN = 25;
const int SD_CS_PIN = 5;
const int pwmFreq = 25000;
const int pwmChannel = 0;
const int pwmResolution = 8;
const int MOTOR_PWM_PIN     = -1; // TODO: No free output GPIO — resolve pin before production use
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
  RELAY_TEST_PICK,
  RELAY_TEST_MENU,
  MOTOR_TEST_MENU,
  STAGE_PARAM_MENU,
  HEATER_TEST_MENU,
  SD_VERIFY_MENU,
  UART_MONITOR_MENU,
  RTC_SET_MENU,
  BREW_SUMMARY_MENU,
  TRANSFER_TEST_MENU,
  FLOW_CAL_MENU,
  HEATER_TEST_PICK,
  CALIB_WIZARD,
  RAPT_TEST_MENU,
  PH_FERM_MENU,
  PID_TRACKING_MENU,
  DISPENSER_TEST_MENU,
  SETTINGS_MENU,
  GRAPH_PICK_MENU,
  PID_CHAMBER_PICK,
  PID_CONFIG_MENU
};

extern bool settingsNeedsFullRedraw;
extern int settingsSelection;
extern bool settingsEditing;
extern AppState prevLoadCellState;

extern long rawHX711;

const uint32_t MIXER_ON_MS     = 5UL * 60 * 1000;
const uint32_t MIXER_OFF_MS    = 355UL * 60 * 1000;
const uint32_t PREHEAT_HOLD_MS = 15UL * 60 * 1000;  // 15 min sterilization hold at 80°C
const uint32_t PAST_HOLD_MS    = 15UL * 60 * 1000;  // 15 min pasteurization hold

extern float    yeastPitchGrams;
extern uint32_t fermDurationMs;

// ---- Motor Command Struct (Main -> Secondary via UART) ----
typedef struct __attribute__((packed)) {
  uint32_t signature; // 0xC0DEBABE
  uint8_t  motorSpeed; // 0-100 (Mixing Impeller)
  uint8_t  motorCW;    // 1=CW, 0=CCW
  uint8_t  yeastCmd;   // 0=Idle, 1=DispenseDuration, 2=DispenseGrams, 3=Stop, 4=Diagnostic, 5=ReverseDuration
  uint32_t yeastVal;   // durationMs or (uint32_t)(grams * 1000)
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
  float motorSenseVolts; // Volts on BTS7960 current sense
  uint8_t sensor2Status;
  uint8_t adsStatus;
  uint8_t ds18Status;
  uint8_t bleStatus;
  uint8_t pillBattery;
  int16_t pillRSSI;
  uint8_t checksum;
} struct_message;
static_assert(sizeof(struct_message) == 40, "struct_message size must be 40 bytes");


struct RaptLog {
  float gravity;
  float temp;
  int16_t rssi;
  char timeStr[12];
};

extern RaptLog raptLogs[10];
extern int raptLogCount;
extern bool raptTestNeedsFullRedraw;
extern bool phFermNeedsFullRedraw;

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
extern float preheatTempOffset;
extern float pastTempOffset;
extern float fermTempOffset;
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
extern bool bypassWeightCheck;
extern bool dashNeedsFullRedraw;
extern int dashSelection;
extern bool dashGraphSelected;
extern int dashGraphPlotType;
extern bool graphPickNeedsFullRedraw;
extern int graphPickSelection;
extern bool moduleViewActive;
extern int lastDashSelection;
extern uint32_t lastPhaseViewNavMs;

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
extern bool relayTestPickNeedsFullRedraw;
extern int relayTestPickSelection;
extern bool relayTestNeedsFullRedraw;
extern int  relayTestSelection;
extern bool relayTestAuto;
extern uint32_t relayTestTimer;
extern bool testRelayStates[9];
extern bool motorTestNeedsFullRedraw;
extern int  motorTestSpeed;
extern bool motorTestCW;
extern bool motorTestOn;
extern bool dispenserTestNeedsFullRedraw;
extern int  dispenserTestSelection;
extern bool dispenserTestEditing;
extern bool dispenserTestOn;
extern int  dispenserTestSpeed;
extern bool dispenserTestCW;
extern bool pidTestNeedsFullRedraw;
extern bool pidConfigNeedsFullRedraw;
extern bool pidConfigEditing;
extern int pidTestChoice;
extern float pidTestHeatTarget;
extern float pidTestCoolTarget;
extern int pidTestTargetSelection;
extern bool pidTestRunning;
extern bool pidTestSuccess;
extern uint32_t pidTestStableStart;
extern uint32_t pidTestStartMs;

// ---- PID Thermal Tracking Test ----
struct PidTrackingMetrics {
  float startTemp;
  float targetTemp;
  float peakTemp;
  float steadyStateError;
  int riseTimeSec;     // -1 if not reached
  int settlingTimeSec; // -1 if not settled
  float overshootDeg;  // Peak above target
  char stabilityStr[12];
};

extern bool pidTrackNeedsFullRedraw;
extern bool pidTrackRunning;
extern uint32_t pidTrackStartMs;
extern uint32_t pidTrackLastSampleMs;
extern float pidTrackTargetTemp;
extern float pidTrackHistory[600];
extern int pidTrackHistoryCount;
extern int pidTrackSampleIntervalSec;
extern PidTrackingMetrics pidTrackMetrics;
extern char pidLogFileName[32];

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
extern bool stageParamEditing;
extern int stageParamStage;
extern bool preHeatSterilized;
extern bool preHeatCooled;
extern bool preHeatHolding;
extern uint32_t preHeatHoldStart;
extern bool pastSterilized;
extern bool pastHolding;
extern uint32_t pastHoldStart;
extern bool phAlertActive;
extern float simTempOverride[3];
extern bool  simDynamic[3];
extern bool  simManual[3];
extern bool     heaterTestNeedsFullRedraw;
extern int      heaterTestStage;
extern int      heaterTestPercent;
extern bool     heaterTestRunning;
extern int      heaterTestSelection;
extern bool     heaterTestEditing;
extern uint32_t heaterTestStart;
extern bool     sdVerifyNeedsFullRedraw;
extern int      sdVerifyResult;
extern bool     transferTestNeedsFullRedraw;
extern int      transferTestSelection;
extern bool     pumpPreHeatFermOn;
extern bool     pumpFermPastOn;
extern volatile uint32_t flowPulse1;
extern volatile uint32_t flowPulse2;
extern float     flowKFactor[2];
extern int       flowCalSensor;
extern float     flowCalKnownVolume;
extern int       flowCalSelection;
extern bool      flowCalNeedsFullRedraw;
extern bool      flowCalEditing;
extern int      pidFanPercent;
extern int      pidFermSensor;
extern int      pidPreHeatSensor;
extern bool     uartMonitorNeedsFullRedraw;
extern uint32_t uartPacketCount;
extern uint32_t uartChecksumErrors;
extern bool     rtcSetNeedsFullRedraw;
extern bool     brewSummaryNeedsFullRedraw;
extern int      rtcSetField;
extern int      rtcSetYear;
extern int      rtcSetMonth;
extern int      rtcSetDay;
extern int      rtcSetHour;
extern int      rtcSetMinute;
extern uint32_t stageElapsedMs[3];

// ---- Temperature Graph ----
#define TEMP_GRAPH_W 288
extern float    tempHistory[TEMP_GRAPH_W];
extern int      tempHistoryCount;

// ---- Stage Transfer ----
extern bool     stageTransferring;
extern int      stageTransferTarget;
extern uint32_t transferStartMs;
extern float    transferStartWeight;


// ---- Physical Test Run Settings ----
extern bool     skipPreheatHeater;
extern float    minVolumeReq;
extern bool     wizardEditing;

// ---- Calibration Wizard Settings ----
extern bool     calibNeedsFullRedraw;
extern int      calibSelection;
extern int      calibTarget;
extern float    calibVolume;
extern bool     calibRunning;
extern bool     calibCompleted;
extern uint32_t calibCompleteTime;
extern bool     calibTareDone;
extern long     calibRawTareValue;

// ---- Temperature Calibration Helpers ----
float getPreheatTemp();
float getPastTemp();
float getFermTemp();

// ---- Motor & Yeast Dispenser Command Sender ----
void sendMotorCommand(int speed, bool cw, uint8_t yeastCmd = 0, uint32_t yeastVal = 0);


