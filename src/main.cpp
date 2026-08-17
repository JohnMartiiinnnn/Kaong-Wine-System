/*
 * WIRING FOR MAIN ESP32:
 * SD Card: CS->5, MISO->19 (LCD MISO Disconnected), MOSI->23, SCLK->18
 * OneWire (Liquid Temps, shared bus): Data->26
 * RTC (DS3231): SDA->21, SCL->22
 * TFT LCD: CS->15, DC->2, RST->4, MOSI->23, SCLK->18
 * TOUCHSCREEN: CS->33, CLK->18, DIN->23, DO->19
 * SSR PREHEAT:  GPIO13
 * SSR FERM:     GPIO12
 * SSR PAST:     GPIO14
 * FLOW SENSOR 1 (Pre-heat→Ferm): GPIO32
 * FLOW SENSOR 2 (Ferm→Past):     GPIO34
 * MCP23017 GPB0-7: 8-Channel Relay Module
 * FAN RELAY: GPA7 (MCP)
 * KEYPAD: RIGHT->GPA0, LEFT->GPA1, UP->GPA2, DOWN->GPA3, SELECT->GPA4 (MCP)
 * EMERGENCY STOP: GPA6 (MCP)
 */

#include "config.h"
#include "display.h"
#include "logging.h"
#include "server.h"
// RBDdimmer removed. Using Slow PWM (Time-Proportional Control)
const uint32_t PID_WINDOW_MS = 3000;
uint32_t pidWindowStart = 0;

// ---- Hardware Object Definitions ----
Adafruit_MCP23X17 mcp;
RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
Adafruit_BME280 bme1;
OneWire sharedOneWire(ONE_WIRE_BUS);
DallasTemperature sharedLiquidSensors(&sharedOneWire);
HX711 scale;
WebServer server(80);

// ---- Sensor Status ----
bool bme1Status = false;
bool liquid1Status = false;
bool liquid2Status = false;
bool remoteStatusReceived = false;
bool rtcStatus = false;
bool sdStatus = false;
bool hx711Status = false;
long rawHX711 = 0;

// ---- Sensor & Brew Data ----
struct_message incomingData = {};
uint32_t lastDataReceivedMillis = 0;
float currentWeight = 0.0;
float calibrationFactor = 23012.45; // Calibrated: 9L known weight, raw=207112
RaptLog raptLogs[10] = {};
int raptLogCount = 0;
bool raptTestNeedsFullRedraw = true;
bool phFermNeedsFullRedraw = true;
float preheatTempOffset =
    0.0f; // Calibration offset for Pre-heat probe (Index 1)
float pastTempOffset =
    0.0f; // Calibration offset for Pasteurization probe (Index 0)
float fermTempOffset =
    0.0f; // Calibration offset for Fermentation probe (remote via UART)
float originalGravity = 0.0;
bool ogCapturing = false;
int ogSampleCount = 0;
float ogSampleSum = 0.0f;
const int OG_SAMPLES = 5;
String currentLogFile = "/data_log.csv";
char lastLogTime[10] = "--:--";
char brewStartTime[32] = "NOT STARTED";

// ---- Actuator State ----
int currentSpeedPercent = 0;
int currentHeatingPercent = 0;
bool isFanOn = false;
bool isFermFanOn = false;
bool isLight1On = false;
bool isLight2On = false;
bool isLight3On = false;
FanMode currentFanMode = FAN_OFF;
int returnConfirmState = 0;
uint32_t returnConfirmTimer = 0;
int mixerSpeedPercent = 0;
MixerMode currentMixerMode = MIXER_OFF;
bool mixerRunning = false;
uint32_t mixerOnTimer = 0;
uint32_t mixerCycleTimer = 0;

// ---- UI / App State ----
AppState currentAppState = SYSTEM_INIT;
bool menuNeedsFullRedraw = true;
int menuSelection = 0;
bool wizardNeedsFullRedraw = true;
int wizardSelection = 0;
bool bypassWeightCheck = false;
bool settingsNeedsFullRedraw = true;
int settingsSelection = 0;
bool settingsEditing = false;
bool dashNeedsFullRedraw = true;
int dashSelection = 0;
bool dashGraphSelected = false;
int dashGraphPlotType = 0;
bool graphPickNeedsFullRedraw = true;
int graphPickSelection = 0;
bool moduleViewActive = false;
int lastDashSelection = -1;
uint32_t lastPhaseViewNavMs = 0;

bool monitorNeedsFullRedraw = true;
bool calNeedsFullRedraw = true;
int calSelection = 0;
bool loadCellNeedsFullRedraw = true;
int loadCellSelection = 0;
int systemCheckSelection = 0;
int fanTestSelection = 0;
int fanTestRow = 0;
int fanTestFanChoice = 0;
int fanTestSpeed = 0;
int lightTestSelection = 0;
bool systemCheckNeedsFullRedraw = true;
bool fanTestNeedsFullRedraw = true;
bool lightTestNeedsFullRedraw = true;
bool relayTestPickNeedsFullRedraw = true;
int relayTestPickSelection = 0; // 0=Auto, 1=Manual
bool relayTestNeedsFullRedraw = true;
int relayTestSelection = 0; // 1..9=Relays (no mode row needed now)
bool relayTestAuto = true;  // defaults to auto-sequencing
uint32_t relayTestTimer = 0;
bool testRelayStates[9] = {false, false, false, false, false,
                           false, false, false, false};
bool motorTestNeedsFullRedraw = true;
int motorTestSpeed = 0;
bool motorTestCW = true;
bool motorTestOn = false;
bool dispenserTestNeedsFullRedraw = true;
int  dispenserTestSelection = 0;
bool dispenserTestEditing = false;
bool dispenserTestOn = false;
int  dispenserTestSpeed = 50;
bool dispenserTestCW = true;

void sendDispenserTestCommand() {
  uint32_t yval = ((uint32_t)(dispenserTestOn ? 1 : 0) << 16) |
                  (1UL << 8) |
                  (uint32_t)dispenserTestSpeed;
  sendMotorCommand(0, true, 6, yval);
}
AppState prevLoadCellState = SYSTEM_CHECK_MENU;
// ---- PID Test State ----
bool pidTestNeedsFullRedraw = true;
bool pidConfigNeedsFullRedraw = true;
bool pidConfigEditing = false;
int pidTestChoice = 0;
float pidTestHeatTarget = 40.0f;
float pidTestCoolTarget = 28.5f;
int pidTestTargetSelection = 0;
bool pidTestRunning = false;
bool pidTestSuccess = false;
uint32_t pidTestStableStart = 0;
uint32_t pidTestStartMs = 0;
int pidFanPercent = 0;
int pidFermSensor = 1;
int pidPreHeatSensor = 0;

PIDController preheatPid(PREHEAT_PID_KP, PREHEAT_PID_KI, PREHEAT_PID_KD, PREHEAT_RAMP_BAND, 0.0f, 100.0f);
PIDController pastPid(PAST_PID_KP, PAST_PID_KI, PAST_PID_KD, PAST_RAMP_BAND, 0.0f, 100.0f);
PIDController quartzPid(QUARTZ_PID_KP, QUARTZ_PID_KI, QUARTZ_PID_KD, QUARTZ_RAMP_BAND, 0.0f, 100.0f);
PIDController trackingPid(PREHEAT_PID_KP, PREHEAT_PID_KI, PREHEAT_PID_KD, PREHEAT_RAMP_BAND, 0.0f, 100.0f);

// ---- PID Thermal Tracking Test State ----
bool pidTrackNeedsFullRedraw = true;
bool pidTrackRunning = false;
uint32_t pidTrackStartMs = 0;
uint32_t pidTrackLastSampleMs = 0;
float pidTrackTargetTemp = 80.0f;
float pidTrackHistory[600];
int pidTrackHistoryCount = 0;
int pidTrackSampleIntervalSec = 2;
PidTrackingMetrics pidTrackMetrics = {0.0f, 80.0f, 0.0f, 0.0f, -1, -1, 0.0f, "IDLE"};
char pidLogFileName[32] = "/pid_track.csv";

// ---- Brew Stage & Stage Params ----
int activeBrewStage = -1;
uint32_t stageStartMillis = 0;
float stageTargetTemp[3] = {30.0f, 28.5f, 80.0f};
float fermTargetPH = 3.0f;
float fermTargetGravity = 1.010f;
int stageParamSelection = 0;
bool stageParamNeedsFullRedraw = true;
bool stageParamEditing = false;
int stageParamStage = 0;
bool preHeatSterilized = false;
bool preHeatCooled = false;
bool preHeatHolding = false;
uint32_t preHeatHoldStart = 0;
bool pastSterilized = false;
bool pastHolding = false;
uint32_t pastHoldStart = 0;
bool phAlertActive = false;
float simTempOverride[3] = {0.0f, 0.0f, 0.0f};
bool simDynamic[3] = {false, false, false};
bool simManual[3] = {false, false, false};
uint32_t stageElapsedMs[3] = {0, 0, 0};
bool heaterTestNeedsFullRedraw = true;
int heaterTestStage = 0;
int heaterTestPercent = 0;
bool heaterTestRunning = false;
int heaterTestSelection = 0;
bool heaterTestEditing = false;
uint32_t heaterTestStart = 0;
bool sdVerifyNeedsFullRedraw = true;
int sdVerifyResult = -1;
bool transferTestNeedsFullRedraw = true;
int transferTestSelection = 0;
bool pumpPreHeatFermOn = false;
bool pumpFermPastOn = false;
volatile bool sysPump1Active = false;
volatile bool sysPump2Active = false;
volatile uint32_t flowPulse1 = 0;
volatile uint32_t flowPulse2 = 0;

void setPump1(bool state) {
  sysPump1Active = state;
  mcp.digitalWrite(PUMP_PREHEAT_FERM, state ? RELAY_ON : RELAY_OFF);
}

void setPump2(bool state) {
  sysPump2Active = state;
  mcp.digitalWrite(PUMP_FERM_PAST, state ? RELAY_ON : RELAY_OFF);
}
float flowKFactor[2] = {450.0f, 450.0f};
int flowCalSensor = 0;
float flowCalKnownVolume = 1.0f;
int flowCalSelection = 0;
bool flowCalNeedsFullRedraw = true;
bool flowCalEditing = false;
bool uartMonitorNeedsFullRedraw = true;
uint32_t uartPacketCount = 0;
uint32_t uartChecksumErrors = 0;
bool rtcSetNeedsFullRedraw = true;
bool brewSummaryNeedsFullRedraw = true;
bool simRunActive = false;
float tempHistory[TEMP_GRAPH_W] = {};
int tempHistoryCount = 0;
bool stageTransferring = false;
int stageTransferTarget = -1;
uint32_t transferStartMs = 0;
float transferStartWeight = 10.0f;

bool skipPreheatHeater = false;
float minVolumeReq = 10.0f;
bool wizardEditing = false;
bool calibNeedsFullRedraw = true;
int calibSelection = 0;
int calibTarget = 0;
float calibVolume = 5.0f;
bool calibRunning = false;
bool calibCompleted = false;
uint32_t calibCompleteTime = 0;
uint32_t calibStartMs = 0;
bool calibTareDone = false;
long calibRawTareValue = 0;
uint32_t calibLedTimer = 0;
bool calibLedState = false;
uint32_t calibLastPulseCount = 0;
int rtcSetField = 0;
int rtcSetYear = 2026;
int rtcSetMonth = 7;
int rtcSetDay = 22;
int rtcSetHour = 0;
int rtcSetMinute = 0;

// ---- Button Latch State ----
bool ljRight = false, ljLeft = false, ljUp = false, ljDown = false,
     ljSelect = false;
bool lastLjRight = false, lastLjLeft = false, lastLjUp = false,
     lastLjDown = false, lastLjSelect = false;

// ---- HX711 EMA seed flag (file-scope so tare handler can reset it) ----
static bool hx711WeightSeeded = false;

// ---- Fan Speed Helper ----
void setFanSpeed(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  currentSpeedPercent = percent;
  bool isFerm = isFermFanOn ||
                (currentAppState == FAN_TEST_MENU && fanTestFanChoice == 1);
  ledcWrite(pwmChannel, isFerm ? map(percent, 0, 100, 0, 255)
                               : map(percent, 0, 100, 255, 0));
}

// ---- Motor Command Sender ----
void sendMotorCommand(int speed, bool cw, uint8_t yeastCmd, uint32_t yeastVal) {
  motor_cmd_t cmd;
  cmd.signature = 0xC0DEBABE;
  cmd.motorSpeed = (uint8_t)speed;
  cmd.motorCW = cw ? 1 : 0;
  cmd.yeastCmd = yeastCmd;
  cmd.yeastVal = yeastVal;
  cmd.checksum = 0;
  const uint8_t *p = (const uint8_t *)&cmd;
  for (size_t i = 0; i < sizeof(motor_cmd_t) - 1; i++)
    cmd.checksum ^= p[i];
  Serial2.write((uint8_t *)&cmd, sizeof(cmd));
}


// ---- Mixer Speed Helper ----
void setMixerSpeed(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  mixerSpeedPercent = percent;
  ledcWrite(MOTOR_PWM_CHANNEL, map(percent, 0, 100, 0, 255));
  sendMotorCommand(percent, true);
}

// ---- Relay Test Helper ----
void setRelayTestChannel(int idx, bool state) {
  if (idx < 0 || idx >= 9)
    return;
  testRelayStates[idx] = state;
  if (idx == 0) { // FERM FAN
    isFermFanOn = state;
    mcp.digitalWrite(FERM_FAN_RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
    mcp.digitalWrite(FERM_FAN2_RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
    setFanSpeed(state ? 100 : 0);
  } else if (idx == 1) { // PUMP 1
    setPump1(state);
  } else if (idx == 2) { // PUMP 2
    setPump2(state);
  } else if (idx < 8) { // CH3..CH7 (general relays and status lights)
    mcp.digitalWrite(RELAY_PINS[idx], state ? RELAY_ON : RELAY_OFF);
  } else if (idx == 8) { // PRE FAN
    isFanOn = state;
    mcp.digitalWrite(FAN_RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
    setFanSpeed(state ? 100 : 0);
  }
}

// ---- Flow Sensor ISRs ----
void IRAM_ATTR flowISR1() {
  if (!sysPump1Active)
    return;
  static uint32_t lastPulse1 = 0;
  uint32_t now = millis();
  if (now - lastPulse1 > 5) {
    flowPulse1++;
    lastPulse1 = now;
  }
}

void IRAM_ATTR flowISR2() {
  if (!sysPump2Active)
    return;
  static uint32_t lastPulse2 = 0;
  uint32_t now = millis();
  if (now - lastPulse2 > 5) {
    flowPulse2++;
    lastPulse2 = now;
  }
}

// ---- Setup ----
void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, HIGH);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  digitalWrite(MOTOR_PWM_PIN, LOW);

  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(19, INPUT_PULLUP);

  delay(100);
  if (SD.begin(SD_CS_PIN, SPI, 400000)) {
    if (SD.cardType() != CARD_NONE)
      sdStatus = true;
  }
  tft.init();
  tft.setRotation(0);
  drawSplashScreen();

  Wire.begin();

  if (rtc.begin()) {
    rtcStatus = true;
    DateTime now = rtc.now();
    if (rtc.lostPower() || now.year() < 2026) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  if (bme1.begin(0x76))
    bme1Status = true;

  sharedLiquidSensors.begin();
  sharedLiquidSensors.setWaitForConversion(false);
  int deviceCount = sharedLiquidSensors.getDeviceCount();
  if (deviceCount > 0)
    liquid1Status = true;
  if (deviceCount > 1)
    liquid2Status = true;
  sharedLiquidSensors.requestTemperatures();

  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  {
    uint32_t t0 = millis();
    while (!scale.is_ready() && millis() - t0 < 2000)
      delay(10);
  }
  if (scale.is_ready()) {
    hx711Status = true;
    scale.set_scale(calibrationFactor);
    scale.tare(10);
  }

  if (mcp.begin_I2C()) {
    for (int i = 0; i < 8; i++) {
      mcp.pinMode(RELAY_PINS[i], OUTPUT);
      mcp.digitalWrite(RELAY_PINS[i], RELAY_OFF);
    }
    mcp.pinMode(FAN_RELAY_PIN, OUTPUT);
    mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
    mcp.pinMode(FERM_FAN_RELAY_PIN, OUTPUT);
    mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
    mcp.pinMode(FERM_FAN2_RELAY_PIN, OUTPUT);
    mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
    mcp.pinMode(PUMP_PREHEAT_FERM, OUTPUT);
    setPump1(false);
    mcp.pinMode(PUMP_FERM_PAST, OUTPUT);
    setPump2(false);
    mcp.pinMode(LIGHT_R, OUTPUT);
    mcp.digitalWrite(LIGHT_R, RELAY_OFF);
    mcp.pinMode(LIGHT_Y, OUTPUT);
    mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
    mcp.pinMode(LIGHT_G, OUTPUT);
    mcp.digitalWrite(LIGHT_G, RELAY_OFF);
    mcp.pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
    mcp.pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
    mcp.pinMode(BTN_UP_PIN, INPUT_PULLUP);
    mcp.pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
    mcp.pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
  }
  pinMode(FLOW_PREHEAT_FERM, INPUT_PULLUP);
  pinMode(FLOW_FERM_PAST, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PREHEAT_FERM), flowISR1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_FERM_PAST), flowISR2, RISING);

  pinMode(SSR_PREHEAT, OUTPUT);
  digitalWrite(SSR_PREHEAT, LOW);
  pinMode(SSR_FERM, OUTPUT);
  digitalWrite(SSR_FERM, LOW);
  pinMode(SSR_PAST, OUTPUT);
  digitalWrite(SSR_PAST, LOW);

  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(PWM_PIN, pwmChannel);
  setFanSpeed(0);
  ledcSetup(MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ, pwmResolution);
  ledcAttachPin(MOTOR_PWM_PIN, MOTOR_PWM_CHANNEL);
  setMixerSpeed(0);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("WineBrew_System", "12345678");
  WiFi.begin("Ejerciatdo Residence", "Ejercitado05");
  {
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 8000)
      delay(100);
  }

  if (MDNS.begin("winebrew"))
    MDNS.addService("http", "tcp", 80);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();

  delay(2000);
  currentAppState = START_MENU;
  drawStartMenu();
}

// ---- Main Loop ----
void loop() {
  server.handleClient();

  // ---- Calibration Wizard LED UI ----
  if (currentAppState == CALIB_WIZARD) {
    if (calibCompleted) {
      if (millis() - calibCompleteTime < 3000UL) {
        digitalWrite(2, HIGH);
      } else {
        digitalWrite(2, LOW);
        calibCompleted = false;
        calibNeedsFullRedraw = true;
        drawCalibWizard();
      }
    } else if (calibRunning) {
      bool isActive = false;
      if (calibTarget == 0 || calibTarget == 1) {
        volatile uint32_t currentPulses =
            (calibTarget == 0) ? flowPulse1 : flowPulse2;
        if (currentPulses != calibLastPulseCount) {
          isActive = true;
          calibLastPulseCount = currentPulses;
        }
      } else {
        static long lastRaw = 0;
        if (abs(rawHX711 - lastRaw) > 500) {
          isActive = true;
          lastRaw = rawHX711;
        }
      }
      uint32_t interval = isActive ? 100 : 500;
      if (millis() - calibLedTimer >= interval) {
        calibLedState = !calibLedState;
        digitalWrite(2, calibLedState ? HIGH : LOW);
        calibLedTimer = millis();
      }
    } else {
      digitalWrite(2, LOW);
    }
  } else {
    digitalWrite(2, LOW);
  }

  static uint32_t ld = 0, ll = 0, ls = 0, lw = 0;
  static bool lsd = !sdStatus;
  char buf[64];

  // Button reads
  bool rawRight = (mcp.digitalRead(BTN_RIGHT_PIN) == LOW);
  bool rawLeft = (mcp.digitalRead(BTN_LEFT_PIN) == LOW);
  bool rawUp = (mcp.digitalRead(BTN_UP_PIN) == LOW);
  bool rawDown = (mcp.digitalRead(BTN_DOWN_PIN) == LOW);
  bool rawSelect = (mcp.digitalRead(BTN_SELECT_PIN) == LOW);

  static int countRi_high = 0, countRi_low = 0;
  static int countLe_high = 0, countLe_low = 0;
  static int countUp_high = 0, countUp_low = 0;
  static int countDown_high = 0, countDown_low = 0;
  static int countS_high = 0, countS_low = 0;
  static bool latchedRight = false;
  static bool latchedLeft = false;
  static bool latchedUp = false;
  static bool latchedDown = false;
  static bool latchedSelect = false;

  // Debounce: Right
  if (rawRight) {
    countRi_high++;
    countRi_low = 0;
  } else {
    countRi_low++;
    countRi_high = 0;
  }
  if (countRi_high >= 3)
    latchedRight = true;
  else if (countRi_low >= 3)
    latchedRight = false;

  // Debounce: Left
  if (rawLeft) {
    countLe_high++;
    countLe_low = 0;
  } else {
    countLe_low++;
    countLe_high = 0;
  }
  if (countLe_high >= 3)
    latchedLeft = true;
  else if (countLe_low >= 3)
    latchedLeft = false;

  // Debounce: Up
  if (rawUp) {
    countUp_high++;
    countUp_low = 0;
  } else {
    countUp_low++;
    countUp_high = 0;
  }
  if (countUp_high >= 3)
    latchedUp = true;
  else if (countUp_low >= 3)
    latchedUp = false;

  // Debounce: Down
  if (rawDown) {
    countDown_high++;
    countDown_low = 0;
  } else {
    countDown_low++;
    countDown_high = 0;
  }
  if (countDown_high >= 3)
    latchedDown = true;
  else if (countDown_low >= 3)
    latchedDown = false;

  // Debounce: Select
  if (rawSelect) {
    countS_high++;
    countS_low = 0;
  } else {
    countS_low++;
    countS_high = 0;
  }
  if (countS_high >= 3)
    latchedSelect = true;
  else if (countS_low >= 3)
    latchedSelect = false;

  bool cRight = latchedRight;
  bool cLeft = latchedLeft;
  bool cUp = latchedUp;
  bool cDown = latchedDown;
  bool cSelect = latchedSelect;

  // Return confirm state
  if (returnConfirmState == 1) {
    if (millis() - returnConfirmTimer > 3000) {
      returnConfirmState = 0;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    }
    if (cLeft && !ljLeft) {
      returnConfirmState = 0;
      simRunActive = false;
      stageTransferring = false;
      activeBrewStage = -1;
      currentHeatingPercent = 0;
      isFanOn = false;
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      setFanSpeed(0);
      isFermFanOn = false;
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(LIGHT_R, RELAY_OFF);
      mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
      mcp.digitalWrite(LIGHT_G, RELAY_OFF);
      // Clear all sim/demo state so sensor checks show real data
      simTempOverride[0] = simTempOverride[1] = simTempOverride[2] = 0.0f;
      simDynamic[0] = simDynamic[1] = simDynamic[2] = false;
      simManual[0] = simManual[1] = simManual[2] = false;
      incomingData.bleStatus = 0;
      incomingData.sensor2Status = 0;
      incomingData.adsStatus = 0;
      incomingData.ds18Status = 0;
      incomingData.pillGravity = 0.0f;
      incomingData.phValue = 0.0f;
      incomingData.room2LiquidTemp = 0.0f;
      incomingData.room2Temp = 0.0f;
      incomingData.pillRSSI = 0;
      incomingData.pillBattery = 0;
      remoteStatusReceived = false;
      currentAppState = START_MENU;
      menuNeedsFullRedraw = true;
      drawStartMenu();
    }
    ljLeft = cLeft;
    return;
  }

  // Navigation: Right / Down
  if (cDown && !ljDown) {
    if (currentAppState == START_MENU) {
      menuSelection = (menuSelection + 1) % 4;
      drawStartMenu();
    } else if (currentAppState == NEW_BREW_WIZARD) {
      if (cDown && !ljDown && !wizardEditing) {
        wizardSelection = (wizardSelection + 1) % 2;
        drawNewBrewWizard();
      }
    } else if (currentAppState == SETTINGS_MENU) {
      if (cDown && !ljDown) {
        if (settingsEditing) {
          if (settingsSelection == 0) {
            minVolumeReq = max(1.0f, minVolumeReq - 0.5f);
          } else if (settingsSelection == 1) {
            pidFanPercent = max(0, (pidFanPercent > 0 ? pidFanPercent : 20) - 5);
          }
        } else {
          settingsSelection = (settingsSelection + 1) % 4;
        }
        drawSettingsMenu();
      }
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      dashGraphSelected = !dashGraphSelected;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == GRAPH_PICK_MENU) {
      int maxChoices = (dashSelection == 1) ? 3 : 1;
      graphPickSelection = (graphPickSelection + 1) % maxChoices;
      drawGraphPickMenu();
    } else if (currentAppState == COOLING_MENU) {
      currentSpeedPercent -= 10;
      if (currentSpeedPercent < 0)
        currentSpeedPercent = 100;
      setFanSpeed(currentSpeedPercent);
      drawCoolingMenu();
    } else if (currentAppState == MIXER_MENU &&
               currentMixerMode == MIXER_MANUAL) {
      mixerSpeedPercent -= 10;
      if (mixerSpeedPercent < 0)
        mixerSpeedPercent = 100;
      setMixerSpeed(mixerSpeedPercent);
      drawMixerMenu();
    } else if (currentAppState == CALIBRATION_MODE) {
      if (!wizardEditing) {
        calSelection = (calSelection + 1) % 3;
        drawCalibrationPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      if (!wizardEditing) {
        loadCellSelection = (loadCellSelection + 1) % 2;
        drawLoadCellPage();
      }
    } else if (currentAppState == CALIB_WIZARD) {
      if (cDown && !ljDown && !calibRunning) {
        calibSelection = (calibSelection + 1) % 3;
        drawCalibWizard();
      }
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      systemCheckSelection = (systemCheckSelection + 1) % 11;
      drawSystemCheckMenu();
    } else if (currentAppState == DISPENSER_TEST_MENU) {
      if (dispenserTestSelection == 1 && dispenserTestEditing) {
        dispenserTestSpeed += 10;
        if (dispenserTestSpeed > 100) dispenserTestSpeed = 100;
        sendDispenserTestCommand();
      } else {
        dispenserTestSelection = (dispenserTestSelection + 1) % 2;
      }
      drawDispenserTestMenu();
    } else if (currentAppState == PID_CHAMBER_PICK) {
      pidTestChoice = (pidTestChoice + 1) % 3;
      drawPidChamberPick();
    } else if (currentAppState == PID_CONFIG_MENU) {
      if (pidConfigEditing) {
        if (pidTestTargetSelection == 0) { pidTestHeatTarget -= 1.0f; if (pidTestHeatTarget < 20.0f) pidTestHeatTarget = 20.0f; }
        else if (pidTestTargetSelection == 1) { pidTestCoolTarget -= 1.0f; if (pidTestCoolTarget < 10.0f) pidTestCoolTarget = 10.0f; }
      } else {
        pidTestTargetSelection = (pidTestTargetSelection + 1) % 3;
      }
      drawPidConfigMenu();
    } else if (currentAppState == FAN_TEST_PICK) {
      fanTestFanChoice = (fanTestFanChoice + 1) % 2;
      drawFanTestPick();
    } else if (currentAppState == FAN_TEST_MENU) {
      fanTestSpeed -= 10;
      if (fanTestSpeed < 0)
        fanTestSpeed = 100;
      setFanSpeed(fanTestSpeed);
      drawFanTestMenu();
    } else if (currentAppState == LIGHT_TEST_MENU) {
      lightTestSelection = (lightTestSelection + 1) % 3;
      drawLightTestMenu();
    } else if (currentAppState == MOTOR_TEST_MENU && cDown) {
      motorTestSpeed -= 25;
      if (motorTestSpeed < 0)
        motorTestSpeed = 0;
      if (motorTestOn) sendMotorCommand(motorTestSpeed, true);
      drawMotorTestMenu();
    } else if (currentAppState == STAGE_PARAM_MENU) {
      if (cDown && !ljDown) {
        if (stageParamEditing) {
          stageParamEditing = false;
        } else {
          int maxRows = (stageParamStage == 1) ? 5 : 3;
          stageParamSelection = (stageParamSelection + 1) % maxRows;
        }
        drawStageParamMenu();
      }
    } else if (currentAppState == HEATER_TEST_PICK) {
      if (cDown && !ljDown) {
        heaterTestStage = (heaterTestStage + 1) % 3;
        drawHeaterTestPick();
      }
    } else if (currentAppState == HEATER_TEST_MENU) {
      if (cDown && !ljDown) {
        if (heaterTestEditing) {
          if (heaterTestPercent >= 5)
            heaterTestPercent -= 5;
          else
            heaterTestPercent = 100;
        } else {
          heaterTestSelection = (heaterTestSelection + 1) % 2;
        }
        drawHeaterTestMenu();
      }
    } else if (currentAppState == RTC_SET_MENU) {
      if (cDown && !ljDown) {
        rtcSetField = (rtcSetField + 1) % 7;
        drawRtcSetMenu();
      }
    } else if (currentAppState == TRANSFER_TEST_MENU) {
      if (cDown && !ljDown) {
        transferTestSelection = (transferTestSelection + 1) % 4;
        drawTransferTestMenu();
      }
    } else if (currentAppState == FLOW_CAL_MENU) {
      if (cDown && !ljDown) {
        if (flowCalEditing) {
          flowCalKnownVolume -= 0.5f;
          if (flowCalKnownVolume < 0.5f)
            flowCalKnownVolume = 0.5f;
        } else {
          flowCalSelection = (flowCalSelection + 1) % 3;
        }
        drawFlowCalMenu();
      }
    } else if (currentAppState == RELAY_TEST_PICK) {
      relayTestPickSelection = (relayTestPickSelection + 1) % 2;
      drawRelayTestPick();
    } else if (currentAppState == RELAY_TEST_MENU) {
      if (!relayTestAuto) {
        relayTestSelection = (relayTestSelection + 1) % 9;
        drawRelayTestMenu();
      }
    }
  }

  // Navigation: Up
  if (cUp && !ljUp) {
    if (currentAppState == START_MENU) {
      menuSelection = (menuSelection + 3) % 4;
      drawStartMenu();
    } else if (currentAppState == NEW_BREW_WIZARD) {
      if (!wizardEditing) {
        wizardSelection = (wizardSelection + 1) % 2;
        drawNewBrewWizard();
      }
    } else if (currentAppState == SETTINGS_MENU) {
      if (settingsEditing) {
        if (settingsSelection == 0) {
          minVolumeReq = min(50.0f, minVolumeReq + 0.5f);
        } else if (settingsSelection == 1) {
          pidFanPercent = min(50, (pidFanPercent > 0 ? pidFanPercent : 20) + 5);
        }
      } else {
        settingsSelection = (settingsSelection + 3) % 4;
      }
      drawSettingsMenu();
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      dashGraphSelected = !dashGraphSelected;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == GRAPH_PICK_MENU) {
      int maxChoices = (dashSelection == 1) ? 3 : 1;
      graphPickSelection = (graphPickSelection - 1 + maxChoices) % maxChoices;
      drawGraphPickMenu();
    } else if (currentAppState == COOLING_MENU) {
      currentSpeedPercent += 10;
      if (currentSpeedPercent > 100)
        currentSpeedPercent = 0;
      setFanSpeed(currentSpeedPercent);
      drawCoolingMenu();
    } else if (currentAppState == MIXER_MENU &&
               currentMixerMode == MIXER_MANUAL) {
      mixerSpeedPercent += 10;
      if (mixerSpeedPercent > 100)
        mixerSpeedPercent = 0;
      setMixerSpeed(mixerSpeedPercent);
      drawMixerMenu();
    } else if (currentAppState == CALIBRATION_MODE) {
      if (!wizardEditing) {
        calSelection = (calSelection + 2) % 3;
        drawCalibrationPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      if (!wizardEditing) {
        loadCellSelection = (loadCellSelection + 1) % 2;
        drawLoadCellPage();
      }
    } else if (currentAppState == CALIB_WIZARD) {
      if (cUp && !ljUp && !calibRunning) {
        calibSelection = (calibSelection + 2) % 3;
        drawCalibWizard();
      }
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      systemCheckSelection = (systemCheckSelection + 10) % 11;
      drawSystemCheckMenu();
    } else if (currentAppState == DISPENSER_TEST_MENU) {
      if (dispenserTestSelection == 1 && dispenserTestEditing) {
        dispenserTestSpeed -= 10;
        if (dispenserTestSpeed < 0) dispenserTestSpeed = 0;
        sendDispenserTestCommand();
      } else {
        dispenserTestSelection = (dispenserTestSelection + 1) % 2;
      }
      drawDispenserTestMenu();
    } else if (currentAppState == PID_CHAMBER_PICK) {
      pidTestChoice = (pidTestChoice + 2) % 3;
      drawPidChamberPick();
    } else if (currentAppState == PID_CONFIG_MENU) {
      if (pidConfigEditing) {
        if (pidTestTargetSelection == 0) { pidTestHeatTarget += 1.0f; if (pidTestHeatTarget > 100.0f) pidTestHeatTarget = 100.0f; }
        else if (pidTestTargetSelection == 1) { pidTestCoolTarget += 1.0f; if (pidTestCoolTarget > 100.0f) pidTestCoolTarget = 100.0f; }
      } else {
        pidTestTargetSelection = (pidTestTargetSelection + 2) % 3;
      }
      drawPidConfigMenu();
    } else if (currentAppState == FAN_TEST_PICK) {
      fanTestFanChoice = (fanTestFanChoice + 1) % 2;
      drawFanTestPick();
    } else if (currentAppState == FAN_TEST_MENU) {
      fanTestSpeed += 10;
      if (fanTestSpeed > 100)
        fanTestSpeed = 0;
      setFanSpeed(fanTestSpeed);
      drawFanTestMenu();
    } else if (currentAppState == LIGHT_TEST_MENU) {
      lightTestSelection = (lightTestSelection + 2) % 3;
      drawLightTestMenu();
    } else if (currentAppState == MOTOR_TEST_MENU) {
      motorTestSpeed += 25;
      if (motorTestSpeed > 100)
        motorTestSpeed = 100;
      if (motorTestOn) sendMotorCommand(motorTestSpeed, true);
      drawMotorTestMenu();
    } else if (currentAppState == STAGE_PARAM_MENU) {
      if (!stageParamEditing) {
        int maxRows = (stageParamStage == 1) ? 5 : 3;
        stageParamSelection = (stageParamSelection + maxRows - 1) % maxRows;
        drawStageParamMenu();
      }
    } else if (currentAppState == HEATER_TEST_PICK) {
      heaterTestStage = (heaterTestStage + 2) % 3;
      drawHeaterTestPick();
    } else if (currentAppState == HEATER_TEST_MENU) {
      if (heaterTestEditing) {
        heaterTestPercent += 5;
        if (heaterTestPercent > 100)
          heaterTestPercent = 0;
      } else {
        heaterTestSelection = (heaterTestSelection + 1) % 2;
      }
      drawHeaterTestMenu();
    } else if (currentAppState == RTC_SET_MENU) {
      rtcSetField = (rtcSetField + 6) % 7;
      drawRtcSetMenu();
    } else if (currentAppState == TRANSFER_TEST_MENU) {
      transferTestSelection = (transferTestSelection + 3) % 4;
      drawTransferTestMenu();
    } else if (currentAppState == FLOW_CAL_MENU) {
      if (flowCalEditing) {
        flowCalKnownVolume += 0.5f;
        if (flowCalKnownVolume > 20.0f)
          flowCalKnownVolume = 20.0f;
      } else {
        flowCalSelection = (flowCalSelection + 2) % 3;
      }
      drawFlowCalMenu();
    } else if (currentAppState == RELAY_TEST_PICK) {
      relayTestPickSelection = (relayTestPickSelection + 1) % 2;
      drawRelayTestPick();
    } else if (currentAppState == RELAY_TEST_MENU) {
      if (!relayTestAuto) {
        relayTestSelection = (relayTestSelection + 8) % 9;
        drawRelayTestMenu();
      }
    }
  }

  // Navigation: Select
  if (cSelect && !ljSelect) {
    if (currentAppState == START_MENU) {
      if (menuSelection == 0) {
        if (activeBrewStage >= 0) {
          currentAppState = DASHBOARD_ACTIVE;
          dashNeedsFullRedraw = true;
          moduleViewActive = false;
          drawDashboardLayout();
        } else {
          currentAppState = NEW_BREW_WIZARD;
          wizardNeedsFullRedraw = true;
          wizardSelection = 0;
          wizardEditing = false;
          drawNewBrewWizard();
        }
      } else if (menuSelection == 1) {
        currentAppState = SETTINGS_MENU;
        settingsNeedsFullRedraw = true;
        settingsSelection = 0;
        settingsEditing = false;
        drawSettingsMenu();
      } else if (menuSelection == 2) {
        currentAppState = SYSTEM_CHECK_MENU;
        systemCheckNeedsFullRedraw = true;
        systemCheckSelection = 0;
        drawSystemCheckMenu();
      } else if (menuSelection == 3) {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == NEW_BREW_WIZARD) {
      if (wizardSelection == 0) {
        bypassWeightCheck = !bypassWeightCheck;
        drawNewBrewWizard();
      } else if (wizardSelection == 1) {
        if (bypassWeightCheck || currentWeight >= minVolumeReq) {
          if (rtcStatus) {
            DateTime now = rtc.now();
            int h12 = now.hour() % 12;
            if (h12 == 0) h12 = 12;
            const char* ampm = (now.hour() >= 12) ? "PM" : "AM";
            sprintf(brewStartTime, "%02d/%02d %d:%02d%s", now.day(), now.month(),
                    h12, now.minute(), ampm);
          }
          originalGravity = 0.0f;
          ogSampleSum = 0.0f;
          ogSampleCount = 0;
          ogCapturing = true;
          activeBrewStage = 0;
          stageStartMillis = millis();
          tempHistoryCount = 0;
          mcp.digitalWrite(LIGHT_R, RELAY_ON);
          mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
          mcp.digitalWrite(LIGHT_G, RELAY_OFF);
          preHeatSterilized = skipPreheatHeater;
          preHeatCooled = skipPreheatHeater;
          preHeatHolding = false;
          pastSterilized = false;
          pastHolding = false;
          phAlertActive = false;
          simTempOverride[0] = 0.0f;
          simTempOverride[1] = 0.0f;
          simTempOverride[2] = 0.0f;
          simDynamic[0] = simDynamic[1] = simDynamic[2] = false;
          simManual[0] = simManual[1] = simManual[2] = false;
          stageElapsedMs[0] = stageElapsedMs[1] = stageElapsedMs[2] = 0;
          stageParamEditing = false;
          currentAppState = DASHBOARD_ACTIVE;
          dashNeedsFullRedraw = true;
          moduleViewActive = false;
          drawDashboardLayout();
        }
      }
    } else if (currentAppState == BREW_SUMMARY_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      dashNeedsFullRedraw = true;
      moduleViewActive = false;
      drawDashboardLayout();
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      if (activeBrewStage == -1) {
        currentAppState = BREW_SUMMARY_MENU;
        brewSummaryNeedsFullRedraw = true;
        drawBrewSummaryMenu();
      } else if (dashGraphSelected) {
        currentAppState = GRAPH_PICK_MENU;
        graphPickNeedsFullRedraw = true;
        graphPickSelection = dashGraphPlotType;
        drawGraphPickMenu();
      }
    } else if (currentAppState == GRAPH_PICK_MENU) {
      dashGraphPlotType = graphPickSelection;
      currentAppState = DASHBOARD_ACTIVE;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == MIXER_MENU) {
      if (currentMixerMode == MIXER_OFF) {
        currentMixerMode = MIXER_MANUAL;
        mixerSpeedPercent = 100;
        setMixerSpeed(100);
      } else if (currentMixerMode == MIXER_MANUAL) {
        currentMixerMode = MIXER_AUTO;
        mixerRunning = true;
        mixerOnTimer = millis();
        mixerCycleTimer = 0;
        mixerSpeedPercent = 100;
        setMixerSpeed(100);
      } else {
        currentMixerMode = MIXER_OFF;
        mixerRunning = false;
        mixerSpeedPercent = 0;
        setMixerSpeed(0);
      }
      drawMixerMenu();
    } else if (currentAppState == COOLING_MENU) {
      if (currentFanMode == FAN_OFF) {
        currentFanMode = FAN_ON;
        isFanOn = true;
      } else if (currentFanMode == FAN_ON) {
        currentFanMode = FAN_AUTO;
        isFanOn = true;
      } else {
        currentFanMode = FAN_OFF;
        isFanOn = false;
      }
      mcp.digitalWrite(FAN_RELAY_PIN, isFanOn ? RELAY_ON : RELAY_OFF);
      drawCoolingMenu();
    } else if (currentAppState == RELAY_TEST_PICK) {
      relayTestAuto = (relayTestPickSelection == 0);
      for (int i = 0; i < 9; i++) {
        setRelayTestChannel(i, false);
      }
      relayTestSelection = 0;
      relayTestTimer = millis();
      if (relayTestAuto) {
        setRelayTestChannel(0, true);
      }
      currentAppState = RELAY_TEST_MENU;
      relayTestNeedsFullRedraw = true;
      drawRelayTestMenu();
    } else if (currentAppState == RELAY_TEST_MENU) {
      if (!relayTestAuto) {
        int idx = relayTestSelection;
        setRelayTestChannel(idx, !testRelayStates[idx]);
        drawRelayTestMenu();
      }
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      if (systemCheckSelection == 0) {
        currentAppState = FAN_TEST_PICK;
        fanTestNeedsFullRedraw = true;
        fanTestFanChoice = 0;
        drawFanTestPick();
      } else if (systemCheckSelection == 1) {
        currentAppState = LIGHT_TEST_MENU;
        lightTestNeedsFullRedraw = true;
        lightTestSelection = 0;
        drawLightTestMenu();
      } else if (systemCheckSelection == 2) {
        relayTestPickSelection = 0;
        relayTestPickNeedsFullRedraw = true;
        currentAppState = RELAY_TEST_PICK;
        drawRelayTestPick();
      } else if (systemCheckSelection == 3) {
        motorTestSpeed = 0;
        motorTestCW = true;
        motorTestOn = false;
        motorTestNeedsFullRedraw = true;
        sendMotorCommand(0, true);
        currentAppState = MOTOR_TEST_MENU;
        drawMotorTestMenu();
      } else if (systemCheckSelection == 4) {
        pidTestChoice = 0;
        pidTestNeedsFullRedraw = true;
        currentAppState = PID_CHAMBER_PICK;
        drawPidChamberPick();
      } else if (systemCheckSelection == 5) {
        sdVerifyResult = -1;
        sdVerifyNeedsFullRedraw = true;
        currentAppState = SD_VERIFY_MENU;
        drawSdVerifyMenu();
      } else if (systemCheckSelection == 6) {
        uartMonitorNeedsFullRedraw = true;
        currentAppState = UART_MONITOR_MENU;
        drawUartMonitorMenu();
      } else if (systemCheckSelection == 7) {
        transferTestNeedsFullRedraw = true;
        currentAppState = TRANSFER_TEST_MENU;
        drawTransferTestMenu();
      } else if (systemCheckSelection == 8) {
        currentAppState = RAPT_TEST_MENU;
        raptTestNeedsFullRedraw = true;
        raptLogCount = 0;
        memset(raptLogs, 0, sizeof(raptLogs));
        drawRaptTestPage();
      } else if (systemCheckSelection == 9) {
        currentAppState = PH_FERM_MENU;
        phFermNeedsFullRedraw = true;
        drawPhFermMenu();
      } else if (systemCheckSelection == 10) {
        dispenserTestNeedsFullRedraw = true;
        dispenserTestSelection = 0;
        dispenserTestEditing = false;
        dispenserTestOn = false;
        dispenserTestSpeed = 50;
        dispenserTestCW = true;
        sendDispenserTestCommand();
        currentAppState = DISPENSER_TEST_MENU;
        drawDispenserTestMenu();
      }
    } else if (currentAppState == DISPENSER_TEST_MENU) {
      if (dispenserTestSelection == 0) {
        dispenserTestOn = !dispenserTestOn;
        sendDispenserTestCommand();
      } else if (dispenserTestSelection == 1) {
        dispenserTestEditing = !dispenserTestEditing;
      }
      drawDispenserTestMenu();
    } else if (currentAppState == PID_CHAMBER_PICK) {
      if (pidTestChoice == 0) { pidTestHeatTarget = 80.0f; pidTestCoolTarget = 75.0f; }
      else if (pidTestChoice == 1) { pidTestHeatTarget = 27.0f; pidTestCoolTarget = 29.0f; }
      else { pidTestHeatTarget = 72.0f; pidTestCoolTarget = 68.0f; }
      pidTestTargetSelection = 0;
      pidConfigEditing = false;
      pidConfigNeedsFullRedraw = true;
      currentAppState = PID_CONFIG_MENU;
      drawPidConfigMenu();
    } else if (currentAppState == PID_CONFIG_MENU) {
      if (pidConfigEditing) {
        pidConfigEditing = false;
        pidConfigNeedsFullRedraw = true;
        drawPidConfigMenu();
      } else if (pidTestTargetSelection == 2) {
        pidTrackTargetTemp = pidTestHeatTarget;
        pidTrackRunning = true;
        pidTrackStartMs = millis();
        pidTrackLastSampleMs = 0;
        pidTrackHistoryCount = 0;
        pidTrackSampleIntervalSec = 2;
        float initTemp = 25.0f;
        if (pidTestChoice == 0 && liquid2Status) initTemp = getPreheatTemp();
        else if (pidTestChoice == 1 && incomingData.sensor2Status > 0) initTemp = incomingData.room2Temp;
        else if (pidTestChoice == 2 && liquid1Status) initTemp = getPastTemp();
        if (pidTestChoice == 1) {
          trackingPid.setGains(QUARTZ_PID_KP, QUARTZ_PID_KI, QUARTZ_PID_KD);
          trackingPid.setRampBand(QUARTZ_RAMP_BAND);
          isFermFanOn = true;
          mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_ON);
          mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_ON);
          setFanSpeed(20);
        } else if (pidTestChoice == 2) {
          trackingPid.setGains(PAST_PID_KP, PAST_PID_KI, PAST_PID_KD);
          trackingPid.setRampBand(PAST_RAMP_BAND);
        } else {
          trackingPid.setGains(PREHEAT_PID_KP, PREHEAT_PID_KI, PREHEAT_PID_KD);
          trackingPid.setRampBand(PREHEAT_RAMP_BAND);
        }
        pidTrackMetrics.startTemp = initTemp;
        pidTrackMetrics.targetTemp = pidTrackTargetTemp;
        pidTrackMetrics.peakTemp = initTemp;
        pidTrackMetrics.steadyStateError = fabs(initTemp - pidTrackTargetTemp);
        pidTrackMetrics.riseTimeSec = -1;
        pidTrackMetrics.settlingTimeSec = -1;
        pidTrackMetrics.overshootDeg = 0.0f;
        strcpy(pidTrackMetrics.stabilityStr, "TESTING");
        trackingPid.reset();
        if (rtcStatus) {
          DateTime now = rtc.now();
          sprintf(pidLogFileName, "/pid_%02d%02d_%02d%02d.csv", now.month(), now.day(), now.hour(), now.minute());
        } else {
          int runNum = 1;
          while (runNum < 999) {
            sprintf(pidLogFileName, "/pid_run_%02d.csv", runNum);
            if (!sdStatus || !SD.exists(pidLogFileName)) break;
            runNum++;
          }
        }
        if (sdStatus) {
          File f = SD.open(pidLogFileName, FILE_WRITE);
          if (f) {
            f.println("RTC_Time,Elapsed_s,Temp_C,Setpoint_C,PWM_pct,Fan_pct,Error_C");
            f.close();
          }
        }
        pidTrackNeedsFullRedraw = true;
        currentAppState = PID_TRACKING_MENU;
        drawPidTrackingMenu();
      } else {
        pidConfigEditing = true;
        pidConfigNeedsFullRedraw = true;
        drawPidConfigMenu();
      }
    } else if (currentAppState == PID_TRACKING_MENU) {
      if (pidTrackRunning) {
        pidTrackRunning = false;
        currentHeatingPercent = 0;
        digitalWrite(SSR_PREHEAT, LOW);
        digitalWrite(SSR_FERM, LOW);
        digitalWrite(SSR_PAST, LOW);
        setFanSpeed(0);
        isFermFanOn = false;
        isFanOn = false;
        mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
        mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
        mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
        strcpy(pidTrackMetrics.stabilityStr, "STOPPED");
      }
      pidTrackNeedsFullRedraw = true;
      drawPidTrackingMenu();
    } else if (currentAppState == HEATER_TEST_PICK) {
      heaterTestPercent = 0;
      heaterTestRunning = false;
      heaterTestSelection = 0;
      heaterTestEditing = false;
      heaterTestNeedsFullRedraw = true;
      currentAppState = HEATER_TEST_MENU;
      drawHeaterTestMenu();
    } else if (currentAppState == HEATER_TEST_MENU) {
      if (heaterTestEditing) {
        heaterTestEditing = false;
      } else if (heaterTestSelection == 1) {
        if (!heaterTestRunning) {
          heaterTestRunning = true;
        } else {
          heaterTestRunning = false;
          currentHeatingPercent = 0;
        }
      } else {
        heaterTestEditing = true;
      }
      drawHeaterTestMenu();
    } else if (currentAppState == TRANSFER_TEST_MENU) {
      if (transferTestSelection == 0) {
        pumpPreHeatFermOn = !pumpPreHeatFermOn;
        setPump1(pumpPreHeatFermOn);
        drawTransferTestMenu(false, true);
      } else if (transferTestSelection == 1) {
        flowCalSensor = 0;
        flowCalSelection = 0;
        flowCalEditing = false;
        flowCalNeedsFullRedraw = true;
        currentAppState = FLOW_CAL_MENU;
        drawFlowCalMenu();
      } else if (transferTestSelection == 2) {
        pumpFermPastOn = !pumpFermPastOn;
        setPump2(pumpFermPastOn);
        drawTransferTestMenu(false, true);
      } else {
        flowCalSensor = 1;
        flowCalSelection = 0;
        flowCalEditing = false;
        flowCalNeedsFullRedraw = true;
        currentAppState = FLOW_CAL_MENU;
        drawFlowCalMenu();
      }
    } else if (currentAppState == FLOW_CAL_MENU) {
      if (flowCalSelection == 0) {
        flowCalEditing = !flowCalEditing;
      } else if (flowCalSelection == 1) {
        if (flowCalSensor == 0)
          flowPulse1 = 0;
        else
          flowPulse2 = 0;
      } else if (flowCalSelection == 2) {
        uint32_t pulses = (flowCalSensor == 0) ? flowPulse1 : flowPulse2;
        if (pulses > 0 && flowCalKnownVolume > 0.0f)
          flowKFactor[flowCalSensor] = (float)pulses / flowCalKnownVolume;
        flowCalNeedsFullRedraw = true;
      }
      drawFlowCalMenu();
    } else if (currentAppState == SD_VERIFY_MENU) {
      if (sdStatus) {
        const char *testStr = "SDVERIFY_OK";
        File f = SD.open("/sdverify.tmp", FILE_WRITE);
        if (f) {
          f.print(testStr);
          f.close();
          f = SD.open("/sdverify.tmp", FILE_READ);
          if (f) {
            char rbuf[16] = {};
            f.readBytes(rbuf, strlen(testStr));
            f.close();
            SD.remove("/sdverify.tmp");
            sdVerifyResult =
                (strncmp(rbuf, testStr, strlen(testStr)) == 0) ? 1 : 0;
          } else {
            sdVerifyResult = 0;
          }
        } else {
          sdVerifyResult = 0;
        }
      }
      drawSdVerifyMenu();
    } else if (currentAppState == RTC_SET_MENU) {
      if (rtcSetField <= 4) {
        rtcSetField = (rtcSetField + 1) % 7;
        drawRtcSetMenu();
      } else if (rtcSetField == 5) {
        if (rtcStatus) {
          rtc.adjust(DateTime(rtcSetYear, rtcSetMonth, rtcSetDay, rtcSetHour,
                              rtcSetMinute, 0));
        }
        currentAppState = SETTINGS_MENU;
        settingsNeedsFullRedraw = true;
        drawSettingsMenu();
      } else if (rtcSetField == 6) {
        currentAppState = SETTINGS_MENU;
        settingsNeedsFullRedraw = true;
        drawSettingsMenu();
      }
    } else if (currentAppState == SETTINGS_MENU) {
      if (settingsSelection == 0) {
        settingsEditing = !settingsEditing;
        drawSettingsMenu();
      } else if (settingsSelection == 1) {
        settingsEditing = !settingsEditing;
        drawSettingsMenu();
      } else if (settingsSelection == 2) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          rtcSetYear = now.year();
          if (rtcSetYear < 2026) rtcSetYear = 2026;
          rtcSetMonth = now.month();
          rtcSetDay = now.day();
          rtcSetHour = now.hour();
          rtcSetMinute = now.minute();
        }
        currentAppState = RTC_SET_MENU;
        rtcSetNeedsFullRedraw = true;
        rtcSetField = 0;
        drawRtcSetMenu();
      } else if (settingsSelection == 3) {
        prevLoadCellState = SETTINGS_MENU;
        currentAppState = LOAD_CELL_PAGE;
        loadCellNeedsFullRedraw = true;
        loadCellSelection = 0;
        wizardEditing = false;
        drawLoadCellPage();
      }
    } else if (currentAppState == FAN_TEST_PICK) {
      currentAppState = FAN_TEST_MENU;
      fanTestNeedsFullRedraw = true;
      fanTestRow = 0;
      fanTestSpeed = 0;
      setFanSpeed(0);
      drawFanTestMenu();
    } else if (currentAppState == FAN_TEST_MENU) {
      if (currentFanMode == FAN_OFF) {
        currentFanMode = FAN_ON;
        if (fanTestFanChoice == 0) {
          isFanOn = true;
          isFermFanOn = false;
        } else {
          isFermFanOn = true;
          isFanOn = false;
        }
      } else {
        currentFanMode = FAN_OFF;
        isFanOn = false;
        isFermFanOn = false;
      }
      mcp.digitalWrite(FAN_RELAY_PIN, isFanOn ? RELAY_ON : RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, isFermFanOn ? RELAY_ON : RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, isFermFanOn ? RELAY_ON : RELAY_OFF);
      drawFanTestMenu();
    } else if (currentAppState == LIGHT_TEST_MENU) {
      if (lightTestSelection == 0) {
        isLight1On = !isLight1On;
        mcp.digitalWrite(LIGHT_R, isLight1On ? RELAY_ON : RELAY_OFF);
      } else if (lightTestSelection == 1) {
        isLight2On = !isLight2On;
        mcp.digitalWrite(LIGHT_Y, isLight2On ? RELAY_ON : RELAY_OFF);
      } else {
        isLight3On = !isLight3On;
        mcp.digitalWrite(LIGHT_G, isLight3On ? RELAY_ON : RELAY_OFF);
      }
      drawLightTestMenu();
    } else if (currentAppState == MOTOR_TEST_MENU) {
      motorTestOn = !motorTestOn;
      sendMotorCommand(motorTestOn ? motorTestSpeed : 0, true);
      drawMotorTestMenu();
    } else if (currentAppState == SENSOR_MONITOR) {
      // Removed LOAD_CELL_PAGE trigger from SENSOR_MONITOR select key
      // Pressing SELECT inside SENSOR_MONITOR now does nothing.
    } else if (currentAppState == CALIBRATION_MODE) {
      if (calSelection == 0) {
        if (hx711Status) {
          scale.tare();
          currentWeight = 0.0f;
          hx711WeightSeeded = false;
        }
        drawCalibrationPage();
      } else if (calSelection == 1) {
        wizardEditing = !wizardEditing;
        drawCalibrationPage();
      } else if (calSelection == 2) {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      if (hx711Status) {
        scale.tare();
        currentWeight = 0.0f;
        hx711WeightSeeded = false;
        drawLoadCellPage();
      }
    } else if (currentAppState == CALIB_WIZARD) {
      if (calibSelection == 0 || calibSelection == 1) {
        wizardEditing = !wizardEditing;
        drawCalibWizard();
      } else if (calibSelection == 2) {
        // Action Button clicked!
        if (calibTarget == 0 || calibTarget == 1) {
          // Flow Sensor
          if (!calibRunning) {
            calibRunning = true;
            if (calibTarget == 0) {
              flowPulse1 = 0;
              setPump1(true);
            } else {
              flowPulse2 = 0;
              setPump2(true);
            }
            calibStartMs = millis();
            calibLastPulseCount = 0;
            calibCompleted = false;
            drawCalibWizard();
          } else {
            calibRunning = false;
            setPump1(false);
            setPump2(false);
            // Calculate new K-factor
            uint32_t pulses = (calibTarget == 0) ? flowPulse1 : flowPulse2;
            if (pulses > 0) {
              flowKFactor[calibTarget] = (float)pulses / calibVolume;
            }
            calibCompleted = true;
            calibCompleteTime = millis();
            drawCalibWizard();
          }
        } else {
          // Load Cell
          if (!calibTareDone) {
            if (hx711Status) {
              scale.tare();
              calibRawTareValue = scale.get_value(5);
              currentWeight = 0.0f;
              hx711WeightSeeded = false;
              calibTareDone = true;
            }
            drawCalibWizard();
          } else {
            if (hx711Status) {
              long rawVal = scale.get_value(5);
              float diff = (float)(rawVal - calibRawTareValue);
              if (abs(diff) > 10.0f) {
                calibrationFactor = diff / calibVolume;
                scale.set_scale(calibrationFactor);
              }
              calibCompleted = true;
              calibCompleteTime = millis();
              calibTareDone = false;
            }
            drawCalibWizard();
          }
        }
      }
    } else if (currentAppState == STAGE_PARAM_MENU) {
      int lastRow = (stageParamStage == 1) ? 4 : 2;
      if (stageParamEditing) {
        stageParamEditing = false;
        drawStageParamMenu();
      } else if (stageParamSelection == 0) {
        if (simTempOverride[stageParamStage] == 0.0f) {
          simTempOverride[stageParamStage] = 25.0f;
          simDynamic[stageParamStage] = false;
          simManual[stageParamStage] = false;
        } else if (!simDynamic[stageParamStage] &&
                   !simManual[stageParamStage]) {
          simDynamic[stageParamStage] = true;
        } else if (simDynamic[stageParamStage]) {
          simDynamic[stageParamStage] = false;
          simManual[stageParamStage] = true;
        } else {
          simTempOverride[stageParamStage] = 0.0f;
          simManual[stageParamStage] = false;
        }
        drawStageParamMenu();
      } else if (stageParamSelection > 0 && stageParamSelection < lastRow) {
        stageParamEditing = true;
        drawStageParamMenu();
      } else if (stageParamSelection == lastRow) {
        if (activeBrewStage == stageParamStage) {
          // Tear down all actuators before leaving the stage
          currentHeatingPercent = 0;
          isFanOn = false;
          mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
          setFanSpeed(0);
          isFermFanOn = false;
          mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
          mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
          stageElapsedMs[activeBrewStage] = millis() - stageStartMillis;
          if (activeBrewStage < 2) {
            stageTransferring = true;
            stageTransferTarget = activeBrewStage + 1;
            transferStartMs = millis();
            transferStartWeight = (hx711Status && currentWeight > 0.0f) ? currentWeight : 10.0f;

            mcp.digitalWrite(LIGHT_R, RELAY_OFF);
            mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
            mcp.digitalWrite(LIGHT_G, RELAY_OFF);
          } else {
            activeBrewStage = -1;
            mcp.digitalWrite(LIGHT_R, RELAY_ON);
            mcp.digitalWrite(LIGHT_Y, RELAY_ON);
            mcp.digitalWrite(LIGHT_G, RELAY_ON);
          }
        }
        currentAppState = DASHBOARD_ACTIVE;
        moduleViewActive = false;
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
      }
    }
  }

  if (cRight && !ljRight && currentAppState == DASHBOARD_ACTIVE) {
    if (millis() - lastPhaseViewNavMs > 350UL) {
      dashSelection = (dashSelection + 1) % 3;
      dashGraphSelected = false;
      dashGraphPlotType = 0;
      lastPhaseViewNavMs = millis();
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    }
  }

  if (cRight && !ljRight && currentAppState == NEW_BREW_WIZARD) {
    if (wizardSelection == 0 && wizardEditing) {
      minVolumeReq += 1.0f;
      if (minVolumeReq > 50.0f)
        minVolumeReq = 50.0f;
      drawNewBrewWizard();
    }
  }

  // Calibration factor adjust via Right/Left while on factor row
  if (cRight && !ljRight && currentAppState == CALIBRATION_MODE &&
      calSelection == 1 && wizardEditing) {
    calibrationFactor += 10.0;
    scale.set_scale(calibrationFactor);
    drawCalibrationPage();
  }
  // Removed LOAD_CELL_PAGE manual cal adjust from right key

  if (cRight && !ljRight && currentAppState == CALIB_WIZARD && !calibRunning) {
    if (calibSelection == 0 && wizardEditing) {
      calibTarget = (calibTarget + 1) % 3;
      calibCompleted = false;
      calibTareDone = false;
      drawCalibWizard();
    } else if (calibSelection == 1 && wizardEditing) {
      calibVolume += 0.5f;
      if (calibVolume > 50.0f)
        calibVolume = 50.0f;
      calibCompleted = false;
      drawCalibWizard();
    }
  }

  // Removed duplicate cRight DASHBOARD_ACTIVE block to fix double-stepping phase jump

  if (cRight && !ljRight && currentAppState == STAGE_PARAM_MENU) {
    if (stageParamSelection == 0 && simTempOverride[stageParamStage] > 0.0f) {
      simTempOverride[stageParamStage] += 1.0f;
      if (simTempOverride[stageParamStage] > 100.0f)
        simTempOverride[stageParamStage] = 100.0f;
      drawStageParamMenu();
    } else if (stageParamEditing) {
      if (stageParamSelection == 1)
        stageTargetTemp[stageParamStage] += 1.0f;
      else if (stageParamStage == 1 && stageParamSelection == 2)
        fermTargetPH += 0.1f;
      else if (stageParamStage == 1 && stageParamSelection == 3)
        fermTargetGravity += 0.001f;
      drawStageParamMenu();
    }
  }

  if (cRight && !ljRight && currentAppState == RTC_SET_MENU) {
    if (rtcSetField == 0) {
      rtcSetYear++;
      if (rtcSetYear > 2099) rtcSetYear = 2026;
    } else if (rtcSetField == 1) {
      rtcSetMonth = (rtcSetMonth % 12) + 1;
    } else if (rtcSetField == 2) {
      rtcSetDay = (rtcSetDay % 31) + 1;
    } else if (rtcSetField == 3) {
      rtcSetHour = (rtcSetHour + 1) % 24;
    } else if (rtcSetField == 4) {
      rtcSetMinute = (rtcSetMinute + 1) % 60;
    } else if (rtcSetField == 5) {
      rtcSetField = 6;
    } else if (rtcSetField == 6) {
      rtcSetField = 5;
    }
    drawRtcSetMenu();
  }

  if (cRight && !ljRight && currentAppState == SETTINGS_MENU) {
    if (settingsEditing) {
      if (settingsSelection == 0) {
        minVolumeReq = min(50.0f, minVolumeReq + 0.5f);
      } else if (settingsSelection == 1) {
        pidFanPercent = min(50, (pidFanPercent > 0 ? pidFanPercent : 20) + 5);
      }
      drawSettingsMenu();
    } else {
      if (settingsSelection == 0 || settingsSelection == 1) {
        settingsEditing = true;
        drawSettingsMenu();
      } else if (settingsSelection == 2) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          rtcSetYear = max((uint16_t)2026, now.year());
          rtcSetMonth = now.month();
          rtcSetDay = now.day();
          rtcSetHour = now.hour();
          rtcSetMinute = now.minute();
        }
        currentAppState = RTC_SET_MENU;
        rtcSetNeedsFullRedraw = true;
        rtcSetField = 0;
        drawRtcSetMenu();
      } else if (settingsSelection == 3) {
        prevLoadCellState = SETTINGS_MENU;
        currentAppState = LOAD_CELL_PAGE;
        loadCellNeedsFullRedraw = true;
        loadCellSelection = 0;
        wizardEditing = false;
        drawLoadCellPage();
      }
    }
  }
  // Navigation: Left / Return
  if (cLeft && !ljLeft) {
    if (currentAppState == NEW_BREW_WIZARD) {
      currentAppState = START_MENU;
      menuNeedsFullRedraw = true;
      drawStartMenu();
    } else if (currentAppState == GRAPH_PICK_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      if (moduleViewActive) {
        moduleViewActive = false;
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
      } else {
        returnConfirmState = 1;
        returnConfirmTimer = millis();
        drawReturnConfirmation();
      }
    } else if (currentAppState == BREW_SUMMARY_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      dashNeedsFullRedraw = true;
      moduleViewActive = false;
      drawDashboardLayout();
    } else if (currentAppState == COOLING_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      moduleViewActive = true;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == MIXER_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      moduleViewActive = true;
      dashNeedsFullRedraw = true;
      drawDashboardLayout();
    } else if (currentAppState == SENSOR_MONITOR) {
      currentAppState = START_MENU;
      menuNeedsFullRedraw = true;
      drawStartMenu();
    } else if (currentAppState == CALIBRATION_MODE) {
      if (calSelection == 1 && wizardEditing) {
        calibrationFactor -= 10.0;
        scale.set_scale(calibrationFactor);
        drawCalibrationPage();
      } else {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      currentAppState = prevLoadCellState;
      if (prevLoadCellState == SETTINGS_MENU) {
        settingsNeedsFullRedraw = true;
        drawSettingsMenu();
      } else {
        systemCheckNeedsFullRedraw = true;
        drawSystemCheckMenu();
      }
    } else if (currentAppState == RAPT_TEST_MENU) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == PH_FERM_MENU) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == CALIB_WIZARD) {
      if (calibSelection == 0 && wizardEditing) {
        calibTarget = (calibTarget + 2) % 3;
        calibCompleted = false;
        calibTareDone = false;
        drawCalibWizard();
      } else if (calibSelection == 1 && wizardEditing) {
        calibVolume -= 0.5f;
        if (calibVolume < 0.5f)
          calibVolume = 0.5f;
        calibCompleted = false;
        drawCalibWizard();
      } else {
        if (calibTareDone) {
          calibTareDone = false;
          drawCalibWizard();
        } else {
          setPump1(false);
          setPump2(false);
          calibRunning = false;
          currentAppState = START_MENU;
          menuNeedsFullRedraw = true;
          drawStartMenu();
        }
      }
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      currentAppState = START_MENU;
      menuNeedsFullRedraw = true;
      drawStartMenu();
    } else if (currentAppState == FAN_TEST_PICK) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == FAN_TEST_MENU) {
      currentAppState = FAN_TEST_PICK;
      fanTestNeedsFullRedraw = true;
      isFanOn = false;
      isFermFanOn = false;
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      setFanSpeed(0);
      drawFanTestPick();
    } else if (currentAppState == LIGHT_TEST_MENU) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == RELAY_TEST_PICK) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == RELAY_TEST_MENU) {
      for (int i = 0; i < 9; i++) {
        setRelayTestChannel(i, false);
      }
      currentAppState = RELAY_TEST_PICK;
      relayTestPickNeedsFullRedraw = true;
      drawRelayTestPick();
    } else if (currentAppState == MOTOR_TEST_MENU) {
      sendMotorCommand(0, true);
      motorTestSpeed = 0;
      motorTestOn = false;
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == DISPENSER_TEST_MENU) {
      if (dispenserTestEditing) {
        dispenserTestEditing = false;
        drawDispenserTestMenu();
      } else {
        dispenserTestOn = false;
        sendDispenserTestCommand();
        currentAppState = SYSTEM_CHECK_MENU;
        systemCheckNeedsFullRedraw = true;
        drawSystemCheckMenu();
      }
    } else if (currentAppState == PID_CHAMBER_PICK) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == PID_CONFIG_MENU) {
      if (pidConfigEditing) {
        pidConfigEditing = false;
        pidConfigNeedsFullRedraw = true;
        drawPidConfigMenu();
      } else {
        pidTestNeedsFullRedraw = true;
        currentAppState = PID_CHAMBER_PICK;
        drawPidChamberPick();
      }
    } else if (currentAppState == PID_TRACKING_MENU) {
      pidTrackRunning = false;
      currentHeatingPercent = 0;
      digitalWrite(SSR_PREHEAT, LOW);
      digitalWrite(SSR_FERM, LOW);
      digitalWrite(SSR_PAST, LOW);
      setFanSpeed(0);
      isFermFanOn = false;
      isFanOn = false;
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      pidConfigNeedsFullRedraw = true;
      currentAppState = PID_CONFIG_MENU;
      drawPidConfigMenu();
    } else if (currentAppState == STAGE_PARAM_MENU) {
      if (stageParamEditing) {
        if (stageParamSelection == 1)
          stageTargetTemp[stageParamStage] -= 1.0f;
        else if (stageParamStage == 1 && stageParamSelection == 2)
          fermTargetPH -= 0.1f;
        else if (stageParamStage == 1 && stageParamSelection == 3)
          fermTargetGravity -= 0.001f;
        drawStageParamMenu();
      } else {
        stageParamEditing = false;
        currentAppState = DASHBOARD_ACTIVE;
        moduleViewActive = true;
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
      }
    } else if (currentAppState == HEATER_TEST_PICK) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == HEATER_TEST_MENU) {
      if (heaterTestEditing) {
        heaterTestEditing = false;
        drawHeaterTestMenu();
      } else {
        heaterTestRunning = false;
        heaterTestSelection = 0;
        currentHeatingPercent = 0;
        heaterTestNeedsFullRedraw = true;
        currentAppState = HEATER_TEST_PICK;
        drawHeaterTestPick();
      }
    } else if (currentAppState == SD_VERIFY_MENU ||
               currentAppState == UART_MONITOR_MENU) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == RTC_SET_MENU) {
      if (rtcSetField == 0) {
        rtcSetYear = max(2026, rtcSetYear - 1);
        drawRtcSetMenu();
      } else if (rtcSetField == 1) {
        rtcSetMonth = (rtcSetMonth == 1) ? 12 : rtcSetMonth - 1;
        drawRtcSetMenu();
      } else if (rtcSetField == 2) {
        rtcSetDay = (rtcSetDay == 1) ? 31 : rtcSetDay - 1;
        drawRtcSetMenu();
      } else if (rtcSetField == 3) {
        rtcSetHour = (rtcSetHour == 0) ? 23 : rtcSetHour - 1;
        drawRtcSetMenu();
      } else if (rtcSetField == 4) {
        rtcSetMinute = (rtcSetMinute == 0) ? 59 : rtcSetMinute - 1;
        drawRtcSetMenu();
      } else if (rtcSetField == 5) {
        rtcSetField = 6;
        drawRtcSetMenu();
      } else {
        currentAppState = SYSTEM_CHECK_MENU;
        systemCheckNeedsFullRedraw = true;
        drawSystemCheckMenu();
      }
    } else if (currentAppState == SETTINGS_MENU) {
      if (settingsEditing) {
        if (settingsSelection == 0) {
          minVolumeReq = max(1.0f, minVolumeReq - 0.5f);
        } else if (settingsSelection == 1) {
          pidFanPercent = max(0, (pidFanPercent > 0 ? pidFanPercent : 20) - 5);
        }
        drawSettingsMenu();
      } else {
        currentAppState = START_MENU;
        menuNeedsFullRedraw = true;
        drawStartMenu();
      }
    } else if (currentAppState == TRANSFER_TEST_MENU) {
      pumpPreHeatFermOn = false;
      pumpFermPastOn = false;
      setPump1(false);
      setPump2(false);
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == FLOW_CAL_MENU) {
      if (flowCalEditing) {
        flowCalEditing = false;
        drawFlowCalMenu();
      } else {
        transferTestNeedsFullRedraw = true;
        transferTestSelection = (flowCalSensor == 0) ? 1 : 3;
        currentAppState = TRANSFER_TEST_MENU;
        drawTransferTestMenu();
      }
    }
  }

  ljRight = cRight;
  ljLeft = cLeft;
  ljUp = cUp;
  ljDown = cDown;
  ljSelect = cSelect;

  // Relay test auto-advance (1000ms per channel)
  if (currentAppState == RELAY_TEST_MENU && relayTestAuto &&
      millis() - relayTestTimer > 1000) {
    // Turn off current relay
    int idx = relayTestSelection;
    setRelayTestChannel(idx, false);

    delay(50); // Buffer for electrical stabilization

    // Advance to the next relay channel (0..8)
    relayTestSelection = (relayTestSelection + 1) % 9;

    // Turn on the next relay
    idx = relayTestSelection;
    setRelayTestChannel(idx, true);

    relayTestTimer = millis();
    drawRelayTestMenu();
  }

  // SD card health check (1s)
  if (millis() - ls > 1000) {
    ls = millis();
    bool acc = false;
    if (SD.cardType() != CARD_NONE) {
      File r = SD.open("/");
      if (r) {
        acc = true;
        r.close();
      }
    }
    if (sdStatus && !acc) {
      SD.end();
      sdStatus = false;
    } else if (!sdStatus && acc) {
      sdStatus = true;
    } else if (!sdStatus && !acc) {
      static uint32_t rst = 0;
      if (millis() - rst > 5000) {
        rst = millis();
        if (SD.begin(SD_CS_PIN, SPI, 400000))
          sdStatus = true;
      }
    }
  }

  // Update SD status indicator on dashboard
  if (sdStatus != lsd) {
    lsd = sdStatus;
    drawDashboardHeaderInfo();
  }

  // Log every 60s
  if (millis() - ll > 60000) {
    ll = millis();
    logDataToSD();
  }

  // UART receive from Secondary
  while (Serial2.available() > 0) {
    if (Serial2.peek() != 0xEF) {
      Serial2.read();
      continue;
    }
    if (Serial2.available() < (int)sizeof(struct_message))
      break;
    struct_message t;
    Serial2.readBytes((uint8_t *)&t, sizeof(t));
    uint8_t calc_cs = calculateChecksum(t);
    if (t.signature == 0xDEADBEEF && calc_cs == t.checksum) {
      incomingData = t;
      uartPacketCount++;
      incomingData.pillGravity += GRAVITY_OFFSET;
      lastDataReceivedMillis = millis();

      // Check for new RAPT Pill telemetry update
      if (incomingData.pillGravity > 0.1f) {
        static float lastPillTemp = -999.0f;
        static float lastPillGravity = -999.0f;
        static int16_t lastPillRSSI = -999;

        if (incomingData.pillTemp != lastPillTemp ||
            incomingData.pillGravity != lastPillGravity ||
            incomingData.pillRSSI != lastPillRSSI) {

          lastPillTemp = incomingData.pillTemp;
          lastPillGravity = incomingData.pillGravity;
          lastPillRSSI = incomingData.pillRSSI;

          if (currentAppState == RAPT_TEST_MENU && raptLogCount < 10) {
            static uint32_t lastLogTimeMs = 0;
            if (raptLogCount == 0 || (millis() - lastLogTimeMs > 15000UL)) {
              lastLogTimeMs = millis();
              raptLogs[raptLogCount].gravity = incomingData.pillGravity;
              raptLogs[raptLogCount].temp = incomingData.pillTemp;
              raptLogs[raptLogCount].rssi = incomingData.pillRSSI;
              if (rtcStatus) {
                DateTime now = rtc.now();
                sprintf(raptLogs[raptLogCount].timeStr, "%02d:%02d:%02d",
                        now.hour(), now.minute(), now.second());
              } else {
                uint32_t sec = millis() / 1000;
                uint32_t min = sec / 60;
                sec = sec % 60;
                sprintf(raptLogs[raptLogCount].timeStr, "%02d:%02d", min, sec);
              }
              raptLogCount++;

              raptTestNeedsFullRedraw = true;
            }
          }
        }
      }

      if (ogCapturing && incomingData.pillGravity > 0.5f) {
        ogSampleSum += incomingData.pillGravity;
        ogSampleCount++;
        if (ogSampleCount >= OG_SAMPLES) {
          originalGravity = ogSampleSum / ogSampleCount;
          ogCapturing = false;
        }
      }
    } else {
      uartChecksumErrors++;
      Serial.printf("[UART Error] Recv sig=0x%08X (expected 0xDEADBEEF), cs=0x%02X, calc_cs=0x%02X\n",
                    t.signature, t.checksum, calc_cs);
    }
  }

  // HX711 Hybrid EMA Filter (1Hz)
  static uint32_t lastScaleMillis = 0;
  if (hx711Status && scale.is_ready() && (millis() - lastScaleMillis > 1000)) {
    lastScaleMillis = millis();

    // Single read — is_ready() already confirmed data is waiting, so no
    // blocking delay
    float rawW = scale.get_units(1);
    rawHX711 = (long)(rawW * calibrationFactor) + scale.get_offset();

    // 1. Ignore extreme garbage (massive spikes)
    if (rawW < -5.0f || rawW > 70.0f) {
      Serial.printf("raw=%.4f [DISCARDED: Out of Range]\n", rawW);
    } else {
      // 2. Allow negative drift/values through for debugging inverted load
      float inputW = rawW;

      if (!hx711WeightSeeded) {
        currentWeight = inputW;
        hx711WeightSeeded = true;
      } else {
        float diff = abs(inputW - currentWeight);

        if (diff > 0.5f) {
          // Significant change (Object added/removed): Snap instantly
          currentWeight = inputW;
        } else {
          // Small change or drift: Apply EMA smoothing (alpha = 0.2)
          currentWeight = (currentWeight * 0.8f) + (inputW * 0.2f);
        }
      }

      // Floor final currentWeight slightly, but allow negative values for
      // debugging
      if (currentWeight > -0.05f && currentWeight < 0.05f)
        currentWeight = 0.0f;

      Serial.printf("raw=%.4f  ema=%.4f\n", rawW, currentWeight);
    }
  }

  // Wizard weight fast refresh (250ms) — only wizard needs this rate;
  // sensor/cal pages stay on 1s to avoid starving the HX711 EMA with
  // scale.read()
  if (currentAppState == NEW_BREW_WIZARD && millis() - lw > 250) {
    lw = millis();
    drawNewBrewWizard();
  }

  // Auto-mixing scheduler
  if (currentMixerMode == MIXER_AUTO) {
    uint32_t now = millis();
    if (mixerRunning) {
      if (now - mixerOnTimer >= MIXER_ON_MS) {
        mixerRunning = false;
        mixerCycleTimer = now;
        mixerSpeedPercent = 0;
        setMixerSpeed(0);
      }
    } else {
      if (now - mixerCycleTimer >= MIXER_OFF_MS) {
        mixerRunning = true;
        mixerOnTimer = now;
        mixerSpeedPercent = 100;
        setMixerSpeed(100);
      }
    }
  }

  // Main 1s display update
  if (millis() - ld > 1000) {
    ld = millis();
    // ---- PID Thermal Tracking Test Control ----
    if (currentAppState == PID_TRACKING_MENU && pidTrackRunning) {
      static uint32_t pidTrackLastPidCalcMs = 0;

      float curT = -127.0f;
      if (pidTestChoice == 0)
        curT = (liquid1Status || liquid2Status) ? getPreheatTemp() : -127.0f;
      else if (pidTestChoice == 1)
        curT = (incomingData.sensor2Status > 0) ? incomingData.room2Temp : -127.0f;
      else
        curT = liquid1Status ? getPastTemp() : -127.0f;

      if (curT <= -100.0f) {
        // Sensor unavailable — hold output at 0 and skip PID until we have a valid reading
        currentHeatingPercent = 0;
        ledcWrite(pwmChannel, 0);
        return;
      }

      float error = pidTrackTargetTemp - curT;

      // Run PID calculation at fixed 1-second interval
      if (millis() - pidTrackLastPidCalcMs >= 1000) {
        uint32_t nowMs = millis();
        float dt = (pidTrackLastPidCalcMs == 0) ? 1.0f : (nowMs - pidTrackLastPidCalcMs) / 1000.0f;
        pidTrackLastPidCalcMs = nowMs;

        float pidOut = trackingPid.compute(pidTrackTargetTemp, curT, dt);
        currentHeatingPercent = (int)pidOut;

        // Fermentation Chamber Fan Ventilation Control (pidTestChoice == 1)
        if (pidTestChoice == 1) {
          isFermFanOn = true;
          mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_ON);
          mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_ON);

          static uint32_t fermTrackOvershootStartMs = 0;
          if (curT > pidTestCoolTarget) {
            if (fermTrackOvershootStartMs == 0) {
              fermTrackOvershootStartMs = millis();
            }
            uint32_t overSec = (millis() - fermTrackOvershootStartMs) / 1000;
            float overDeg = curT - pidTestCoolTarget;
            int fanSpd = 20 + (int)(overDeg * 20.0f) + (int)(overSec * 2);
            if (fanSpd > 100) fanSpd = 100;
            setFanSpeed(fanSpd);
          } else {
            fermTrackOvershootStartMs = 0;
            setFanSpeed(20); // 20% baseline power during heating / setpoint
          }
        }
      }

      // Dynamic sampling based on pidTrackSampleIntervalSec
      if (millis() - pidTrackLastSampleMs >= (uint32_t)(pidTrackSampleIntervalSec * 1000)) {
        pidTrackLastSampleMs = millis();
        uint32_t elapsedSec = (millis() - pidTrackStartMs) / 1000;

        // Push to graph history array with dynamic time zoom (in-place downsampling when full)
        if (pidTrackHistoryCount < 600) {
          pidTrackHistory[pidTrackHistoryCount++] = curT;
        } else {
          for (int i = 0; i < 300; i++) {
            pidTrackHistory[i] = (pidTrackHistory[2 * i] + pidTrackHistory[2 * i + 1]) / 2.0f;
          }
          pidTrackHistoryCount = 300;
          pidTrackSampleIntervalSec *= 2;
          pidTrackHistory[pidTrackHistoryCount++] = curT;
        }

        // Metrics Calculation
        pidTrackMetrics.steadyStateError = fabs(curT - pidTrackTargetTemp);

        if (curT > pidTrackMetrics.peakTemp) {
          pidTrackMetrics.peakTemp = curT;
        }
        if (pidTrackMetrics.peakTemp > pidTrackTargetTemp) {
          pidTrackMetrics.overshootDeg = pidTrackMetrics.peakTemp - pidTrackTargetTemp;
        }

        // Rise Time: Time to reach 90% of setpoint
        if (pidTrackMetrics.riseTimeSec < 0) {
          float target90 = pidTrackMetrics.startTemp + 0.9f * (pidTrackTargetTemp - pidTrackMetrics.startTemp);
          if (curT >= target90) {
            pidTrackMetrics.riseTimeSec = (int)elapsedSec;
          }
        }

        // Settling Time: Time when temp enters and remains within +-2% band around pidTrackTargetTemp
        static uint32_t inBandStartMs = 0;
        float band = max(1.5f, pidTrackTargetTemp * 0.02f);
        if (curT >= (pidTrackTargetTemp - band) && curT <= (pidTrackTargetTemp + band)) {
          if (inBandStartMs == 0) inBandStartMs = millis();
          if (pidTrackMetrics.settlingTimeSec < 0 && (millis() - inBandStartMs >= 5000)) {
            pidTrackMetrics.settlingTimeSec = (int)((inBandStartMs - pidTrackStartMs) / 1000);
          }
        } else {
          inBandStartMs = 0;
        }

        // Stability Status
        if (curT < (pidTrackTargetTemp - band)) {
          strcpy(pidTrackMetrics.stabilityStr, "TESTING");
        } else if (pidTrackMetrics.steadyStateError <= 0.5f) {
          strcpy(pidTrackMetrics.stabilityStr, "STABLE");
        } else if (pidTrackMetrics.steadyStateError <= 2.0f) {
          strcpy(pidTrackMetrics.stabilityStr, "SETTLING");
        } else {
          strcpy(pidTrackMetrics.stabilityStr, "UNSTABLE");
        }

        // SD Card CSV Logging every 2s to unique run file
        if (sdStatus && pidLogFileName[0] != '\0') {
          File f = SD.open(pidLogFileName, FILE_APPEND);
          if (f) {
            char timeBuf[12] = "--:--:--";
            if (rtcStatus) {
              DateTime now = rtc.now();
              sprintf(timeBuf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            }
            f.printf("%s,%lu,%.2f,%.1f,%d,%d,%.2f\n",
                     timeBuf, (unsigned long)elapsedSec, curT, pidTrackTargetTemp,
                     currentHeatingPercent, currentSpeedPercent, error);
            f.close();
          }
        }
      }
      drawPidTrackingMenu(true);
    }

    // ---- Transfer Completion ----
    if (stageTransferring && millis() - transferStartMs >= 10000UL) {
      stageTransferring = false;
      activeBrewStage = stageTransferTarget;
      stageStartMillis = millis();
      tempHistoryCount = 0;
      if (activeBrewStage == 1) {
        mcp.digitalWrite(LIGHT_R, RELAY_OFF);
        mcp.digitalWrite(LIGHT_Y, RELAY_ON);
        mcp.digitalWrite(LIGHT_G, RELAY_OFF);
      } else if (activeBrewStage == 2) {
        mcp.digitalWrite(LIGHT_R, RELAY_OFF);
        mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
        mcp.digitalWrite(LIGHT_G, RELAY_ON);
      }
      setPump1(false);
      setPump2(false);
      pumpPreHeatFermOn = false;
      pumpFermPastOn = false;
      dashNeedsFullRedraw = true;
      if (currentAppState == DASHBOARD_ACTIVE)
        drawDashboardLayout();
    }

    // ---- Closed-Loop Brew Stage Control ----
    if (activeBrewStage >= 0 && activeBrewStage <= 2 && !stageTransferring) {
      static int lastCtrlStage = -1;
      if (activeBrewStage != lastCtrlStage) {
        preheatPid.reset();
        pastPid.reset();
        lastCtrlStage = activeBrewStage;
      }

      float liquidTemp = -999.0f;
      if (simTempOverride[activeBrewStage] > 0.0f) {
        liquidTemp = simTempOverride[activeBrewStage];
      } else if (activeBrewStage == 0 && liquid2Status) {
        liquidTemp = getPreheatTemp();
      } else if (activeBrewStage == 1 && incomingData.sensor2Status > 0) {
        liquidTemp = incomingData.room2Temp;
      } else if (activeBrewStage == 2 && liquid1Status) {
        liquidTemp = getPastTemp();
      }

      if (activeBrewStage == 0) {
        if (!preHeatSterilized) {
          if (skipPreheatHeater) {
            preHeatSterilized = true;
            preHeatHolding = false;
            currentHeatingPercent = 0;
            preheatPid.reset();
          } else if (liquidTemp > -100.0f) {
            float pidOut = preheatPid.compute(80.0f, liquidTemp, 1.0f);
            currentHeatingPercent = (int)pidOut;
            if (simManual[0])
              currentHeatingPercent = 0;

            if (liquidTemp >= 80.0f) {
              if (!preHeatHolding) {
                preHeatHolding = true;
                preHeatHoldStart = millis();
              }
              if (millis() - preHeatHoldStart >= 15000UL) {
                preHeatSterilized = true;
                preHeatHolding = false;
                currentHeatingPercent = 0;
                preheatPid.reset();
              }
            } else {
              preHeatHolding = false;
            }
          }
        } else {
          currentHeatingPercent = 0;
          if (liquidTemp > -100.0f && !preHeatCooled) {
            if (liquidTemp > 30.0f && !skipPreheatHeater) {
              isFanOn = true;
              mcp.digitalWrite(FAN_RELAY_PIN, RELAY_ON);
              setFanSpeed(100);
            } else {
              preHeatCooled = true;
              isFanOn = false;
              mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
              setFanSpeed(0);
            }
          }
        }

      } else if (activeBrewStage == 1) {
        if (liquidTemp > -100.0f) {
          isFermFanOn = true;
          mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_ON);
          mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_ON);

          static uint32_t fermStageOvershootStartMs = 0;
          float targetTemp = 30.0f;

          if (liquidTemp < 27.0f) {
            currentHeatingPercent = 50; // 50% duty cycle (1.5s ON / 1.5s OFF per 3s cycle) for effective quartz heating
            fermStageOvershootStartMs = 0;
            setFanSpeed(20); // 20% baseline power during active heating
          } else if (liquidTemp > targetTemp) {
            currentHeatingPercent = 0;
            if (fermStageOvershootStartMs == 0) {
              fermStageOvershootStartMs = millis();
            }
            uint32_t overSec = (millis() - fermStageOvershootStartMs) / 1000;
            float overDeg = liquidTemp - targetTemp;
            int fanSpd = 20 + (int)(overDeg * 20.0f) + (int)(overSec * 2);
            if (fanSpd > 100) fanSpd = 100;
            setFanSpeed(fanSpd);
          } else {
            currentHeatingPercent = 0;
            fermStageOvershootStartMs = 0;
            setFanSpeed(20); // 20% baseline power within setpoint range
          }
        }
        if (incomingData.adsStatus == 1 && incomingData.phValue > 0.0f &&
            incomingData.phValue <= fermTargetPH && !phAlertActive) {
          phAlertActive = true;
        }

      } else if (activeBrewStage == 2) {
        if (!pastSterilized) {
          if (liquidTemp > -100.0f) {
            float pidOut = pastPid.compute(80.0f, liquidTemp, 1.0f);
            currentHeatingPercent = (int)pidOut;
            if (simManual[2])
              currentHeatingPercent = 0;

            if (liquidTemp >= 80.0f) {
              if (!pastHolding) {
                pastHolding = true;
                pastHoldStart = millis();
              }
              if (millis() - pastHoldStart >= 15000UL) {
                pastSterilized = true;
                pastHolding = false;
                currentHeatingPercent = 0;
                pastPid.reset();
                mcp.digitalWrite(LIGHT_G, RELAY_ON);
              }
            } else {
              pastHolding = false;
            }
          }
        } else {
          currentHeatingPercent = 0;
        }
      }
    }

    // ---- Dynamic Sim Temperature Update ----
    if (activeBrewStage >= 0 && activeBrewStage <= 2 && !stageTransferring &&
        simTempOverride[activeBrewStage] > 0.0f &&
        simDynamic[activeBrewStage]) {
      const float SIM_RATE = 5.0f;
      float cur = simTempOverride[activeBrewStage];
      bool fansActive = (activeBrewStage == 0) ? isFanOn : isFermFanOn;
      float delta = 0.0f;

      if (activeBrewStage == 1) {
        // Fermentation: bounded random walk 24–33°C, crosses heater (<27) and
        // fan (>30) thresholds
        delta = (float)random(-30, 31) / 10.0f;
        if (cur + delta < 24.0f)
          delta = 3.0f;
        if (cur + delta > 33.0f)
          delta = -3.0f;
      } else {
        // Pre-heat (0) and Pasteurization (2): rise → hold at 80 → fan cool
        bool sterilized =
            (activeBrewStage == 0) ? preHeatSterilized : pastSterilized;
        if (fansActive) {
          // Cooling: noisy downward drift, never positive
          delta = -(SIM_RATE + (float)random(0, 21) / 10.0f);
        } else if (!sterilized && cur < 80.0f) {
          // Heating: noisy upward drift, always at least 0.5°C/s
          delta = SIM_RATE + (float)random(-10, 11) / 10.0f;
          if (delta < 0.5f)
            delta = 0.5f;
        }
        // At 80 holding, or sterilized no fans: delta stays 0 → clean hold
      }

      simTempOverride[activeBrewStage] += delta;
      if (simTempOverride[activeBrewStage] < 0.0f)
        simTempOverride[activeBrewStage] = 0.0f;
      if (simTempOverride[activeBrewStage] > 100.0f)
        simTempOverride[activeBrewStage] = 100.0f;
    }

    // ---- Demo Run: sensor simulation and auto-advance ----
    if (simRunActive && activeBrewStage >= 0) {
      if (activeBrewStage == 1) {
        float prog = (float)(millis() - stageStartMillis) / 80000.0f;
        if (prog > 1.0f)
          prog = 1.0f;
        incomingData.pillGravity = 1.060f - prog * (1.060f - 0.995f);
        incomingData.phValue = 4.2f - prog * (4.2f - 2.5f);
        incomingData.room2LiquidTemp = simTempOverride[1];
        incomingData.room2Temp = 27.5f + (float)random(-5, 6) / 10.0f;
      }
      bool doAdv = false;
      if (activeBrewStage == 0 && preHeatCooled && !stageTransferring)
        doAdv = true;
      else if (activeBrewStage == 1 && millis() - stageStartMillis >= 80000UL &&
               !stageTransferring)
        doAdv = true;
      else if (activeBrewStage == 2 && pastSterilized && !stageTransferring)
        doAdv = true;
      if (doAdv) {
        currentHeatingPercent = 0;
        isFanOn = false;
        mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
        setFanSpeed(0);
        isFermFanOn = false;
        mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
        mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
        stageElapsedMs[activeBrewStage] = millis() - stageStartMillis;
        if (activeBrewStage < 2) {
          stageTransferring = true;
          stageTransferTarget = activeBrewStage + 1;
          transferStartMs = millis();
          transferStartWeight = (hx711Status && currentWeight > 0.0f) ? currentWeight : 10.0f;

          mcp.digitalWrite(LIGHT_R, RELAY_OFF);
          mcp.digitalWrite(LIGHT_Y, RELAY_OFF);
          mcp.digitalWrite(LIGHT_G, RELAY_OFF);
        } else {
          activeBrewStage = -1;
          simRunActive = false;
          mcp.digitalWrite(LIGHT_R, RELAY_ON);
          mcp.digitalWrite(LIGHT_Y, RELAY_ON);
          mcp.digitalWrite(LIGHT_G, RELAY_ON);
        }
        dashNeedsFullRedraw = true;
        if (currentAppState == DASHBOARD_ACTIVE)
          drawDashboardLayout();
      }
    }

    int activeHeaterPin = -1;
    if (currentAppState == PID_TRACKING_MENU && pidTrackRunning) {
      if (pidTestChoice == 0)
        activeHeaterPin = SSR_PREHEAT;
      else if (pidTestChoice == 1)
        activeHeaterPin = SSR_FERM;
      else
        activeHeaterPin = SSR_PAST;
    } else if (currentAppState == HEATER_TEST_MENU && heaterTestRunning) {
      currentHeatingPercent = heaterTestPercent;
      if (heaterTestStage == 0)
        activeHeaterPin = SSR_PREHEAT;
      else if (heaterTestStage == 1)
        activeHeaterPin = SSR_FERM;
      else
        activeHeaterPin = SSR_PAST;
    } else if (activeBrewStage == 0) {
      activeHeaterPin = skipPreheatHeater ? -1 : SSR_PREHEAT;
    } else if (activeBrewStage == 1) {
      activeHeaterPin = SSR_FERM;
    } else if (activeBrewStage == 2) {
      activeHeaterPin = SSR_PAST;
    }

    if (millis() - pidWindowStart >= PID_WINDOW_MS) {
      pidWindowStart += PID_WINDOW_MS;
    }
    uint32_t onTime = (currentHeatingPercent * PID_WINDOW_MS) / 100;

    // Quartz Heater Safety Limit for Fermentation (SSR_FERM): Max 1.5 seconds (1500 ms) continuous ON pulse
    if (activeHeaterPin == SSR_FERM && onTime > 1500) {
      onTime = 1500;
    }

    bool pState = LOW;
    bool fState = LOW;
    bool pastState = LOW;

    if (activeHeaterPin != -1 && (millis() - pidWindowStart < onTime)) {
      if (activeHeaterPin == SSR_PREHEAT)
        pState = HIGH;
      else if (activeHeaterPin == SSR_FERM)
        fState = HIGH;
      else if (activeHeaterPin == SSR_PAST)
        pastState = HIGH;
    }

    // Failsafe: Maximum continuous ON pulse for Fermentation Quartz Heater is 1.5 seconds (1500 ms)
    static uint32_t quartzOnStartMs = 0;
    if (fState == HIGH) {
      if (quartzOnStartMs == 0) quartzOnStartMs = millis();
      if (millis() - quartzOnStartMs >= 1500) {
        fState = LOW; // Cut off pulse after 1.5s continuous burst to protect insulation while maintaining fast ramp
      }
    } else {
      quartzOnStartMs = 0;
    }

    digitalWrite(SSR_PREHEAT, pState);
    digitalWrite(SSR_FERM, fState);
    digitalWrite(SSR_PAST, pastState);

    // ---- Stage Transition Pump Control ----
    if (stageTransferring && !simRunActive) {
      if (stageTransferTarget == 1) {
        setPump1(true);
        pumpPreHeatFermOn = true;
      } else if (stageTransferTarget == 2) {
        setPump2(true);
        pumpFermPastOn = true;
      }
    } else {
      // Turn off pumps if not in manual transfer test menu, relay test menu, or
      // calibration wizard
      if (currentAppState != TRANSFER_TEST_MENU &&
          currentAppState != RELAY_TEST_MENU &&
          currentAppState != CALIB_WIZARD) {
        setPump1(false);
        setPump2(false);
        pumpPreHeatFermOn = false;
        pumpFermPastOn = false;
      }
    }

    tft.setTextPadding(0);

    if (liquid1Status || liquid2Status)
      sharedLiquidSensors.requestTemperatures();

    if (rtcStatus && currentAppState != SENSOR_MONITOR) {
      DateTime n = rtc.now();
      int h12 = n.hour() % 12;
      if (h12 == 0) h12 = 12;
      const char* ampm = (n.hour() >= 12) ? "PM" : "AM";
      sprintf(buf, "%d:%02d", h12, n.minute());
      uint16_t hdrBg;
      switch (currentAppState) {
      case START_MENU:
      case NEW_BREW_WIZARD:
      case DASHBOARD_ACTIVE:
      case MIXER_MENU:
        hdrBg = TFT_NAVY;
        break;
      case LOAD_CELL_PAGE:
        hdrBg = 0x0493;
        break;
      case CALIBRATION_MODE:
        hdrBg = 0x9000;
        break;
      case CALIB_WIZARD:
        hdrBg = 0x4810;
        break;
      case BREW_SUMMARY_MENU:
        hdrBg = 0x0400;
        break;
      case STAGE_PARAM_MENU: {
        const uint16_t sc[] = {TFT_RED, TFT_ORANGE, 0x03E0};
        hdrBg = sc[stageParamStage];
        break;
      }
      default:
        hdrBg = 0x03E0;
        break;
      }
      tft.setTextPadding(0);
      tft.setTextColor(TFT_YELLOW, hdrBg);
      tft.drawRightString(buf, 285, 15, 4);
      tft.drawString(ampm, 290, 24, 1);
    }

    if (currentAppState == SENSOR_MONITOR)
      drawSensorMonitorPage(true);

    if (currentAppState == CALIBRATION_MODE)
      drawCalibrationPage(true);

    if (currentAppState == LOAD_CELL_PAGE)
      drawLoadCellPage(true);

    if (currentAppState == RAPT_TEST_MENU)
      drawRaptTestPage(true);

    if (currentAppState == PH_FERM_MENU)
      drawPhFermMenu(true);

    if (currentAppState == MIXER_MENU)
      drawMixerMenu();

    if (currentAppState == FAN_TEST_MENU)
      drawFanTestMenu();

    if (currentAppState == MOTOR_TEST_MENU)
      drawMotorTestMenu();

    if (currentAppState == DISPENSER_TEST_MENU)
      drawDispenserTestMenu();

    if (currentAppState == STAGE_PARAM_MENU)
      drawStageParamMenu();

    if (currentAppState == UART_MONITOR_MENU)
      drawUartMonitorMenu();

    if (currentAppState == TRANSFER_TEST_MENU)
      drawTransferTestMenu(true);

    if (currentAppState == FLOW_CAL_MENU)
      drawFlowCalMenu(true);

    if (currentAppState == CALIB_WIZARD)
      drawCalibWizard();

    if (currentAppState == DASHBOARD_ACTIVE) {
      updateDashboardValues();

      if (activeBrewStage >= 0 && dashSelection != activeBrewStage) {
        if (millis() - lastPhaseViewNavMs >= 5000UL) {
          dashSelection = activeBrewStage;
          dashNeedsFullRedraw = true;
          drawDashboardLayout();
        }
      }

      if (activeBrewStage >= 0 && !stageTransferring) {
        float gTemp = -999.0f;
        if (simTempOverride[activeBrewStage] > 0.0f)
          gTemp = simTempOverride[activeBrewStage];
        else if (activeBrewStage == 0 && liquid2Status)
          gTemp = getPreheatTemp();
        else if (activeBrewStage == 1 && incomingData.ds18Status == 1)
          gTemp = getFermTemp();
        else if (activeBrewStage == 2 && liquid1Status)
          gTemp = getPastTemp();
        if (gTemp > -100.0f) {
          if (tempHistoryCount < TEMP_GRAPH_W) {
            tempHistory[tempHistoryCount++] = gTemp;
          } else {
            memmove(tempHistory, tempHistory + 1,
                    (TEMP_GRAPH_W - 1) * sizeof(float));
            tempHistory[TEMP_GRAPH_W - 1] = gTemp;
          }
          updateDashboardGraph();
        }
      }
    }

  }
}

float getPreheatTemp() {
  float t = sharedLiquidSensors.getTempCByIndex(1);
  if (t == DEVICE_DISCONNECTED_C)
    return t;
  return t + preheatTempOffset;
}

float getPastTemp() {
  float t = sharedLiquidSensors.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C)
    return t;
  return t + pastTempOffset;
}

float getFermTemp() {
  float t = incomingData.room2LiquidTemp;
  if (t < -100.0f)
    return t;
  return t + fermTempOffset;
}
