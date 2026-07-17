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
const uint32_t PID_WINDOW_MS = 2000;
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
bool isSystemHalted = false;
int estopState = 0;
uint32_t estopTimer = 0;
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
bool dashNeedsFullRedraw = true;
int dashSelection = 0;
bool moduleViewActive = false;
int lastDashSelection = -1;
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
// ---- PID Test State ----
bool pidTestNeedsFullRedraw = true;
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

// ---- Mixer Speed Helper ----
void setMixerSpeed(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  mixerSpeedPercent = percent;
  ledcWrite(MOTOR_PWM_CHANNEL, map(percent, 0, 100, 0, 255));
}

// ---- Motor Command Sender ----
void sendMotorCommand(int speed, bool cw) {
  motor_cmd_t cmd;
  cmd.signature = 0xC0DEBABE;
  cmd.motorSpeed = (uint8_t)speed;
  cmd.motorCW = cw ? 1 : 0;
  cmd.checksum = 0;
  const uint8_t *p = (const uint8_t *)&cmd;
  for (size_t i = 0; i < sizeof(motor_cmd_t) - 1; i++)
    cmd.checksum ^= p[i];
  Serial2.write((uint8_t *)&cmd, sizeof(cmd));
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
    if (rtc.lostPower())
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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
    mcp.pinMode(ESTOP_BUTTON_PIN, INPUT_PULLUP);
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

  ArduinoOTA.setHostname("winebrew-main");
  ArduinoOTA.begin();

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
  ArduinoOTA.handle();
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
  static bool lsd = !sdStatus, les = HIGH;
  char buf[64];

  // Emergency stop
  static uint32_t haltHoldStart = 0;
  static bool haltHoldActive = false;
  static uint8_t lastHoldPct = 255;
  static bool prePauseFanOn = false;
  static bool prePauseFermFanOn = false;
  static int prePauseSpeed = 0;
  static FanMode prePauseFanMode = FAN_OFF;
  bool ces = mcp.digitalRead(ESTOP_BUTTON_PIN);

  if (isSystemHalted) {
    // Long-press e-stop for 3s to reset
    if (ces == LOW) {
      if (!haltHoldActive) {
        haltHoldActive = true;
        haltHoldStart = millis();
        lastHoldPct = 255;
      }
      uint32_t held = millis() - haltHoldStart;
      uint8_t pct = (uint8_t)min(held * 100 / 3000UL, 100UL);
      if (pct != lastHoldPct) {
        lastHoldPct = pct;
        tft.fillRect(40, 300, 240, 20, TFT_DARKGREY);
        tft.fillRect(40, 300, (int)(240 * pct / 100), 20, TFT_WHITE);
        tft.drawRect(40, 300, 240, 20, TFT_WHITE);
      }
      if (held >= 3000) {
        isSystemHalted = false;
        haltHoldActive = false;
        pidTestRunning = false;
        activeBrewStage = -1;
        currentHeatingPercent = 0;
        currentAppState = START_MENU;
        menuNeedsFullRedraw = true;
        drawStartMenu();
      }
    } else {
      if (haltHoldActive) {
        haltHoldActive = false;
        // Reset progress bar
        tft.fillRect(40, 300, 240, 20, TFT_RED);
        tft.drawRect(40, 300, 240, 20, TFT_WHITE);
      }
    }
    les = ces;
    return;
  }

  // Detect ESTOP Button
  if (ces == LOW && les == HIGH) {
    if (estopState == 0) {
      prePauseFanOn = isFanOn;
      prePauseFermFanOn = isFermFanOn;
      prePauseSpeed = currentSpeedPercent;
      prePauseFanMode = currentFanMode;
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      setFanSpeed(0);
      digitalWrite(SSR_PREHEAT, LOW);
      digitalWrite(SSR_FERM, LOW);
      digitalWrite(SSR_PAST, LOW);
      estopState = 1;
      drawEstopPage();
    } else if (estopState == 1) {
      isSystemHalted = true;
      estopState = 0;
      haltHoldActive = false;
      for (int i = 0; i < 8; i++)
        mcp.digitalWrite(RELAY_PINS[i], RELAY_OFF);
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      setFanSpeed(0);
      setMixerSpeed(0);
      currentMixerMode = MIXER_OFF;
      mixerRunning = false;
      isFanOn = false;
      isFermFanOn = false;
      pidTestRunning = false;
      activeBrewStage = -1;
      currentHeatingPercent = 0;
      digitalWrite(SSR_PREHEAT, LOW);
      digitalWrite(SSR_FERM, LOW);
      digitalWrite(SSR_PAST, LOW);
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("SYSTEM HALTED", 160, 80, 4);
      tft.drawCentreString("HOLD ESTOP 3s TO RESET", 160, 200, 2);
      tft.fillRect(40, 300, 240, 20, TFT_RED);
      tft.drawRect(40, 300, 240, 20, TFT_WHITE);
    }
  }
  les = ces;

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

  // E-stop confirm state
  if (estopState == 1) {
    if (cLeft && !ljLeft) {
      estopState = 0;
      isFanOn = prePauseFanOn;
      isFermFanOn = prePauseFermFanOn;
      currentFanMode = prePauseFanMode;
      mcp.digitalWrite(FAN_RELAY_PIN, isFanOn ? RELAY_ON : RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, isFermFanOn ? RELAY_ON : RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, isFermFanOn ? RELAY_ON : RELAY_OFF);
      setFanSpeed(prePauseSpeed);
      switch (currentAppState) {
      case START_MENU:
        menuNeedsFullRedraw = true;
        drawStartMenu();
        break;
      case DASHBOARD_ACTIVE:
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
        break;
      case COOLING_MENU:
        drawCoolingMenu();
        break;
      case MIXER_MENU:
        drawMixerMenu();
        break;
      case SENSOR_MONITOR:
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
        break;
      case CALIBRATION_MODE:
        calNeedsFullRedraw = true;
        drawCalibrationPage();
        break;
      case LOAD_CELL_PAGE:
        loadCellNeedsFullRedraw = true;
        drawLoadCellPage();
        break;
      case SYSTEM_CHECK_MENU:
        systemCheckNeedsFullRedraw = true;
        drawSystemCheckMenu();
        break;
      case PID_TEST_PICK:
        pidTestNeedsFullRedraw = true;
        drawPidTestPick();
        break;
      case PID_TEST_MENU:
        pidTestNeedsFullRedraw = true;
        drawPidTestMenu();
        break;
      case TRANSFER_TEST_MENU:
        transferTestNeedsFullRedraw = true;
        drawTransferTestMenu();
        break;
      case FLOW_CAL_MENU:
        flowCalNeedsFullRedraw = true;
        drawFlowCalMenu();
        break;
      case HEATER_TEST_PICK:
        heaterTestNeedsFullRedraw = true;
        drawHeaterTestPick();
        break;
      case HEATER_TEST_MENU:
        heaterTestRunning = false;
        heaterTestEditing = false;
        heaterTestPercent = 0;
        heaterTestSelection = 0;
        currentHeatingPercent = 0;
        heaterTestNeedsFullRedraw = true;
        currentAppState = HEATER_TEST_PICK;
        drawHeaterTestPick();
        break;
      case FAN_TEST_PICK:
        fanTestNeedsFullRedraw = true;
        drawFanTestPick();
        break;
      case FAN_TEST_MENU:
        fanTestNeedsFullRedraw = true;
        drawFanTestMenu();
        break;
      case LIGHT_TEST_MENU:
        lightTestNeedsFullRedraw = true;
        drawLightTestMenu();
        break;
      case RELAY_TEST_MENU:
        relayTestNeedsFullRedraw = true;
        drawRelayTestMenu();
        break;
      case MOTOR_TEST_MENU:
        motorTestNeedsFullRedraw = true;
        drawMotorTestMenu();
        break;
      case STAGE_PARAM_MENU:
        stageParamNeedsFullRedraw = true;
        drawStageParamMenu();
        break;
      default:
        currentHeatingPercent = 0;
        menuNeedsFullRedraw = true;
        drawStartMenu();
        break;
      }
    }
    ljLeft = cLeft;
    return;
  }

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
  if ((cRight && !ljRight) || (cDown && !ljDown)) {
    if (currentAppState == START_MENU) {
      menuSelection = (menuSelection + 1) % 4;
      drawStartMenu();
    } else if (currentAppState == NEW_BREW_WIZARD) {
      if (cDown && !ljDown && !wizardEditing) {
        wizardSelection = (wizardSelection + 1) % 4;
        drawNewBrewWizard();
      }
    } else if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive) {
      if (cDown && !ljDown && activeBrewStage >= 0 &&
          simManual[activeBrewStage]) {
        simTempOverride[activeBrewStage] -= 5.0f;
        if (simTempOverride[activeBrewStage] < 0.0f)
          simTempOverride[activeBrewStage] = 0.0f;
      } else {
        dashSelection = (dashSelection + 1) % 3;
      }
      drawDashboardLayout();
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
      systemCheckSelection = (systemCheckSelection + 1) % 12;
      drawSystemCheckMenu();
    } else if (currentAppState == PID_TEST_PICK) {
      pidTestChoice = (pidTestChoice + 1) % 3;
      drawPidTestPick();
    } else if (currentAppState == PID_TEST_MENU && !pidTestRunning) {
      if (cDown && !ljDown) {
        if (pidTestTargetSelection == 0) {
          pidTestHeatTarget -= 1.0f;
          if (pidTestHeatTarget < 0.0f)
            pidTestHeatTarget = 0.0f;
        } else if (pidTestTargetSelection == 1) {
          pidTestCoolTarget -= 1.0f;
          if (pidTestCoolTarget < 0.0f)
            pidTestCoolTarget = 0.0f;
        } else {
          if (pidTestChoice == 0)
            pidPreHeatSensor ^= 1;
          else
            pidFermSensor ^= 1;
        }
        drawPidTestMenu();
      }
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
      sendMotorCommand(motorTestSpeed, motorTestCW);
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
        rtcSetField = (rtcSetField + 1) % 2;
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
        wizardSelection = (wizardSelection + 3) % 4;
        drawNewBrewWizard();
      }
    } else if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive) {
      if (activeBrewStage >= 0 && simManual[activeBrewStage]) {
        simTempOverride[activeBrewStage] += 5.0f;
        if (simTempOverride[activeBrewStage] > 100.0f)
          simTempOverride[activeBrewStage] = 100.0f;
      } else {
        dashSelection = (dashSelection + 2) % 3;
      }
      drawDashboardLayout();
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
      systemCheckSelection = (systemCheckSelection + 11) % 12;
      drawSystemCheckMenu();
    } else if (currentAppState == PID_TEST_PICK) {
      pidTestChoice = (pidTestChoice + 2) % 3;
      drawPidTestPick();
    } else if (currentAppState == PID_TEST_MENU && !pidTestRunning) {
      if (pidTestTargetSelection == 0) {
        pidTestHeatTarget += 1.0f;
        if (pidTestHeatTarget > 100.0f)
          pidTestHeatTarget = 100.0f;
      } else if (pidTestTargetSelection == 1) {
        pidTestCoolTarget += 1.0f;
        if (pidTestCoolTarget > 100.0f)
          pidTestCoolTarget = 100.0f;
      } else {
        if (pidTestChoice == 0)
          pidPreHeatSensor ^= 1;
        else
          pidFermSensor ^= 1;
      }
      drawPidTestMenu();
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
      sendMotorCommand(motorTestSpeed, motorTestCW);
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
      rtcSetField = (rtcSetField + 1) % 2;
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
        currentAppState = NEW_BREW_WIZARD;
        wizardNeedsFullRedraw = true;
        wizardSelection = 0;
        wizardEditing = false;
        drawNewBrewWizard();
      } else if (menuSelection == 1) {
        currentAppState = DASHBOARD_ACTIVE;
        dashNeedsFullRedraw = true;
        moduleViewActive = false;
        drawDashboardLayout();
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
        wizardEditing = !wizardEditing;
        drawNewBrewWizard();
      } else if (wizardSelection == 1) {
        skipPreheatHeater = !skipPreheatHeater;
        drawNewBrewWizard();
      } else if ((wizardSelection == 2 && currentWeight >= minVolumeReq) ||
                 wizardSelection == 3) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          sprintf(brewStartTime, "%02d/%02d %02d:%02d", now.day(), now.month(),
                  now.hour(), now.minute());
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
    } else if (currentAppState == BREW_SUMMARY_MENU) {
      currentAppState = DASHBOARD_ACTIVE;
      dashNeedsFullRedraw = true;
      moduleViewActive = false;
      drawDashboardLayout();
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      if (!moduleViewActive && activeBrewStage == -1) {
        currentAppState = BREW_SUMMARY_MENU;
        brewSummaryNeedsFullRedraw = true;
        drawBrewSummaryMenu();
      } else if (!moduleViewActive) {
        moduleViewActive = true;
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
      } else if (dashSelection == 0) {
        currentAppState = COOLING_MENU;
        drawCoolingMenu();
      } else if (dashSelection == 1) {
        currentAppState = MIXER_MENU;
        drawMixerMenu();
      }
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
        testRelayStates[i] = false;
        if (i == 1)
          setPump1(false);
        else if (i == 2)
          setPump2(false);
        else if (i < 8)
          mcp.digitalWrite(RELAY_PINS[i], RELAY_OFF);
        else
          mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      }
      relayTestSelection = 0;
      relayTestTimer = millis();
      if (relayTestAuto) {
        testRelayStates[0] = true;
        mcp.digitalWrite(RELAY_PINS[0], RELAY_ON);
      }
      currentAppState = RELAY_TEST_MENU;
      relayTestNeedsFullRedraw = true;
      drawRelayTestMenu();
    } else if (currentAppState == RELAY_TEST_MENU) {
      if (!relayTestAuto) {
        int idx = relayTestSelection;
        testRelayStates[idx] = !testRelayStates[idx];
        bool state = testRelayStates[idx];
        if (idx == 1) {
          setPump1(state);
        } else if (idx == 2) {
          setPump2(state);
        } else if (idx < 8) {
          mcp.digitalWrite(RELAY_PINS[idx], state ? RELAY_ON : RELAY_OFF);
        } else {
          mcp.digitalWrite(FAN_RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
        }
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
        motorTestNeedsFullRedraw = true;
        sendMotorCommand(0, true);
        currentAppState = MOTOR_TEST_MENU;
        drawMotorTestMenu();
      } else if (systemCheckSelection == 4) {
        currentAppState = PID_TEST_PICK;
        pidTestNeedsFullRedraw = true;
        pidTestChoice = 0;
        drawPidTestPick();
      } else if (systemCheckSelection == 5) {
        heaterTestStage = 0;
        heaterTestNeedsFullRedraw = true;
        currentAppState = HEATER_TEST_PICK;
        drawHeaterTestPick();
      } else if (systemCheckSelection == 6) {
        sdVerifyResult = -1;
        sdVerifyNeedsFullRedraw = true;
        currentAppState = SD_VERIFY_MENU;
        drawSdVerifyMenu();
      } else if (systemCheckSelection == 7) {
        uartMonitorNeedsFullRedraw = true;
        currentAppState = UART_MONITOR_MENU;
        drawUartMonitorMenu();
      } else if (systemCheckSelection == 8) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          rtcSetHour = now.hour();
          rtcSetMinute = now.minute();
        } else {
          rtcSetHour = 0;
          rtcSetMinute = 0;
        }
        rtcSetField = 0;
        rtcSetNeedsFullRedraw = true;
        currentAppState = RTC_SET_MENU;
        drawRtcSetMenu();
      } else if (systemCheckSelection == 9) {
        transferTestNeedsFullRedraw = true;
        currentAppState = TRANSFER_TEST_MENU;
        drawTransferTestMenu();
      } else if (systemCheckSelection == 10) {
        currentAppState = LOAD_CELL_PAGE;
        loadCellNeedsFullRedraw = true;
        loadCellSelection = 0;
        wizardEditing = false;
        drawLoadCellPage();
      } else if (systemCheckSelection == 11) {
        currentAppState = RAPT_TEST_MENU;
        raptTestNeedsFullRedraw = true;
        raptLogCount = 0;
        memset(raptLogs, 0, sizeof(raptLogs));
        drawRaptTestPage();
      }
    } else if (currentAppState == PID_TEST_PICK) {
      currentAppState = PID_TEST_MENU;
      pidTestNeedsFullRedraw = true;
      if (pidTestChoice == 0) {
        pidTestHeatTarget = 35.0f;
        pidTestCoolTarget = 30.0f;
        pidPreHeatSensor = 0;
      } else if (pidTestChoice == 1) {
        pidTestHeatTarget = 27.0f;
        pidTestCoolTarget = 30.0f;
        pidFermSensor = 1;
      } else {
        pidTestHeatTarget = 35.0f;
        pidTestCoolTarget = 30.0f;
      }
      pidTestRunning = false;
      pidTestSuccess = false;
      drawPidTestMenu();
    } else if (currentAppState == PID_TEST_MENU) {
      if (!pidTestRunning) {
        pidTestRunning = true;
        pidTestSuccess = false;
        pidTestStableStart = 0;
        pidTestStartMs = millis();
      } else {
        pidTestRunning = false;
        currentHeatingPercent = 0;
        pidFanPercent = 0;
        isFermFanOn = false;
        isFanOn = false;
        mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
        mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
        mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      }
      pidTestNeedsFullRedraw = true;
      drawPidTestMenu();
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
      if (rtcStatus) {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), rtcSetHour,
                            rtcSetMinute, 0));
      }
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
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
      motorTestSpeed = 0;
      sendMotorCommand(0, motorTestCW);
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

  if (cRight && !ljRight && currentAppState == MOTOR_TEST_MENU) {
    motorTestCW = !motorTestCW;
    sendMotorCommand(motorTestSpeed, motorTestCW);
    drawMotorTestMenu();
  }

  if (cRight && !ljRight && currentAppState == PID_TEST_MENU &&
      !pidTestRunning) {
    pidTestTargetSelection =
        (pidTestTargetSelection + 1) % (pidTestChoice <= 1 ? 3 : 2);
    drawPidTestMenu();
  }

  if (cRight && !ljRight && currentAppState == DASHBOARD_ACTIVE &&
      moduleViewActive) {
    stageParamStage = dashSelection;
    stageParamSelection = 0;
    stageParamEditing = false;
    stageParamNeedsFullRedraw = true;
    currentAppState = STAGE_PARAM_MENU;
    drawStageParamMenu();
  }

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
      rtcSetHour = (rtcSetHour + 1) % 24;
    } else {
      rtcSetMinute = (rtcSetMinute + 1) % 60;
    }
    drawRtcSetMenu();
  }
  // Navigation: Left / Return
  if (cLeft && !ljLeft) {
    if (currentAppState == NEW_BREW_WIZARD) {
      if (wizardSelection == 0 && wizardEditing) {
        minVolumeReq -= 1.0f;
        if (minVolumeReq < 1.0f)
          minVolumeReq = 1.0f;
        drawNewBrewWizard();
      } else {
        currentAppState = START_MENU;
        menuNeedsFullRedraw = true;
        drawStartMenu();
      }
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
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == RAPT_TEST_MENU) {
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
        testRelayStates[i] = false;
        if (i == 1)
          setPump1(false);
        else if (i == 2)
          setPump2(false);
        else if (i < 8)
          mcp.digitalWrite(RELAY_PINS[i], RELAY_OFF);
        else
          mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      }
      currentAppState = RELAY_TEST_PICK;
      relayTestPickNeedsFullRedraw = true;
      drawRelayTestPick();
    } else if (currentAppState == MOTOR_TEST_MENU) {
      sendMotorCommand(0, motorTestCW);
      motorTestSpeed = 0;
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == PID_TEST_PICK) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == PID_TEST_MENU) {
      pidTestRunning = false;
      currentHeatingPercent = 0;
      isFermFanOn = false;
      isFanOn = false;
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
      mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
      currentAppState = PID_TEST_PICK;
      pidTestNeedsFullRedraw = true;
      drawPidTestPick();
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
               currentAppState == UART_MONITOR_MENU ||
               currentAppState == RTC_SET_MENU) {
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
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
    testRelayStates[idx] = false;
    if (idx == 1) {
      setPump1(false);
    } else if (idx == 2) {
      setPump2(false);
    } else if (idx < 8) {
      mcp.digitalWrite(RELAY_PINS[idx], RELAY_OFF);
    } else {
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
    }

    delay(50); // Buffer for electrical stabilization

    // Advance to the next relay channel (0..8)
    relayTestSelection = (relayTestSelection + 1) % 9;

    // Turn on the next relay
    idx = relayTestSelection;
    testRelayStates[idx] = true;
    if (idx == 1) {
      setPump1(true);
    } else if (idx == 2) {
      setPump2(true);
    } else if (idx < 8) {
      mcp.digitalWrite(RELAY_PINS[idx], RELAY_ON);
    } else {
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_ON);
    }

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
    if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive) {
      tft.setTextPadding(0);
      uint16_t bg = sdStatus ? 0x0400 : TFT_RED;
      tft.setTextColor(TFT_WHITE, bg);
      tft.drawString(sdStatus ? " READY " : " ERROR ", 75, 80, 2);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.drawString("LOG:", 190, 80, 2);
      sprintf(buf, " %s ", lastLogTime);
      tft.drawString(buf, 230, 80, 2);
    }
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
    if (t.signature == 0xDEADBEEF && calculateChecksum(t) == t.checksum) {
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
    // ---- PID Test Control ----
    if (currentAppState == PID_TEST_MENU && pidTestRunning) {
      static float pidTestIntegral = 0.0f;
      static float pidTestPrevError = 0.0f;
      static float pidFanPrevTemp = -999.0f;
      const float TEST_KP = 20.0f;
      const float TEST_KI = 0.05f;
      const float TEST_KD = 2.0f;
      const float FAN_KD = 10.0f;

      float liquidTemp = -999.0f;
      if (pidTestChoice == 0) {
        if (pidPreHeatSensor == 0) {
          if (liquid2Status)
            liquidTemp = getPreheatTemp();
          else if (bme1Status)
            liquidTemp = bme1.readTemperature();
        } else {
          if (bme1Status)
            liquidTemp = bme1.readTemperature();
          else if (liquid2Status)
            liquidTemp = getPreheatTemp();
        }
      } else if (pidTestChoice == 1) {
        if (pidFermSensor == 0) {
          if (incomingData.ds18Status == 1 &&
              incomingData.room2LiquidTemp > -100.0f)
            liquidTemp = getFermTemp();
          else if (incomingData.sensor2Status == 1)
            liquidTemp = incomingData.room2Temp;
        } else {
          if (incomingData.sensor2Status == 1)
            liquidTemp = incomingData.room2Temp;
          else if (incomingData.ds18Status == 1 &&
                   incomingData.room2LiquidTemp > -100.0f)
            liquidTemp = getFermTemp();
        }
      } else if (pidTestChoice == 2) {
        if (liquid1Status)
          liquidTemp = getPastTemp();
      }

      if (liquidTemp > -100.0f) {
        float error = 0.0f;
        if (liquidTemp < pidTestHeatTarget) {
          error = pidTestHeatTarget - liquidTemp;
          if (liquidTemp >= pidTestHeatTarget - 5.0f) {
            pidTestIntegral += error;
            if (pidTestIntegral > 100.0f)
              pidTestIntegral = 100.0f;
            if (pidTestIntegral < -100.0f)
              pidTestIntegral = -100.0f;
          } else {
            pidTestIntegral = 0.0f;
          }
        } else if (liquidTemp > pidTestCoolTarget) {
          error = pidTestCoolTarget - liquidTemp; // negative error for cooling
          if (liquidTemp <= pidTestCoolTarget + 5.0f) {
            pidTestIntegral += error;
            if (pidTestIntegral > 100.0f)
              pidTestIntegral = 100.0f;
            if (pidTestIntegral < -100.0f)
              pidTestIntegral = -100.0f;
          } else {
            pidTestIntegral = 0.0f;
          }
        } else {
          error = 0.0f;
          pidTestIntegral = 0.0f;
        }

        float pidOut = (TEST_KP * error) + (TEST_KI * pidTestIntegral) +
                       (TEST_KD * (error - pidTestPrevError));
        pidTestPrevError = error;
        if (pidOut < -100.0f)
          pidOut = -100.0f;
        if (pidOut > 100.0f)
          pidOut = 100.0f;

        if (pidOut > 0.0f) {
          int heat = (int)pidOut;
          if (pidTestChoice == 1 && heat > 50)
            heat = 50;
          currentHeatingPercent = heat;
          isFermFanOn = false;
          isFanOn = false;
        } else if (pidOut < 0.0f) {
          currentHeatingPercent = 0;
          if (pidTestChoice == 1) {
            isFermFanOn = true;
            isFanOn = false;
          } else {
            isFermFanOn = false;
            isFanOn = true;
          }
        } else {
          currentHeatingPercent = 0;
          isFermFanOn = false;
          isFanOn = false;
        }

        if (pidTestChoice == 1) {
          mcp.digitalWrite(FERM_FAN_RELAY_PIN,
                           isFermFanOn ? RELAY_ON : RELAY_OFF);
          mcp.digitalWrite(FERM_FAN2_RELAY_PIN,
                           isFermFanOn ? RELAY_ON : RELAY_OFF);
          if (isFermFanOn) {
            float dTemp = (pidFanPrevTemp > -100.0f)
                              ? (liquidTemp - pidFanPrevTemp)
                              : 0.0f;
            pidFanPrevTemp = liquidTemp;
            // P: how far above cool target; D: back off when temp is already
            // falling fast
            float eNeg = -error;
            float baseFan = (eNeg >= 1.0f) ? 100.0f : (30.0f + eNeg * 70.0f);
            int fanPct = (int)(baseFan + FAN_KD * dTemp);
            if (fanPct > 100)
              fanPct = 100;
            if (fanPct < 30)
              fanPct = 30;
            pidFanPercent = fanPct;
          } else {
            pidFanPrevTemp = -999.0f;
            pidFanPercent = 0;
          }
          setFanSpeed(pidFanPercent);
        } else {
          mcp.digitalWrite(FAN_RELAY_PIN, isFanOn ? RELAY_ON : RELAY_OFF);
          pidFanPercent = isFanOn ? 100 : 0;
          setFanSpeed(pidFanPercent);
        }

        if (abs(error) <= 0.5f) {
          if (pidTestStableStart == 0)
            pidTestStableStart = millis();
          else if (millis() - pidTestStableStart > 15000UL) {
            pidTestSuccess = true;
          }
        } else {
          pidTestStableStart = 0;
          pidTestSuccess = false;
        }
      } else {
        currentHeatingPercent = 0;
        isFanOn = false;
        isFermFanOn = false;
        pidFanPercent = 0;
      }
      drawPidTestMenu();
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
      static float pidIntegral = 0.0f;
      static float pidPrevError = 0.0f;
      static int lastCtrlStage = -1;
      if (activeBrewStage != lastCtrlStage) {
        pidIntegral = 0.0f;
        pidPrevError = 0.0f;
        lastCtrlStage = activeBrewStage;
      }

      float liquidTemp = -999.0f;
      if (simTempOverride[activeBrewStage] > 0.0f) {
        liquidTemp = simTempOverride[activeBrewStage];
      } else if (activeBrewStage == 0 && liquid2Status) {
        liquidTemp = getPreheatTemp();
      } else if (activeBrewStage == 1 && incomingData.ds18Status == 1) {
        liquidTemp = getFermTemp();
      } else if (activeBrewStage == 2 && liquid1Status) {
        liquidTemp = getPastTemp();
      }

      if (activeBrewStage == 0) {
        if (!preHeatSterilized) {
          if (skipPreheatHeater) {
            preHeatSterilized = true;
            preHeatHolding = false;
            currentHeatingPercent = 0;
            pidIntegral = 0.0f;
          } else if (liquidTemp > -100.0f) {
            float error = 80.0f - liquidTemp;
            if (liquidTemp >= PID_THROTTLE_TEMP) {
              pidIntegral += error;
              if (pidIntegral > 100.0f)
                pidIntegral = 100.0f;
              if (pidIntegral < -100.0f)
                pidIntegral = -100.0f;
            } else {
              pidIntegral = 0.0f;
            }
            float pidOut = (PID_KP * error) + (PID_KI * pidIntegral) +
                           (PID_KD * (error - pidPrevError));
            pidPrevError = error;
            if (pidOut < 0.0f)
              pidOut = 0.0f;
            if (pidOut > 100.0f)
              pidOut = 100.0f;
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
                pidIntegral = 0.0f;
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
          if (liquidTemp < 27.0f) {
            currentHeatingPercent = 100;
            isFermFanOn = false;
            mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_OFF);
            mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_OFF);
          } else if (liquidTemp > 30.0f) {
            currentHeatingPercent = 0;
            isFermFanOn = true;
            mcp.digitalWrite(FERM_FAN_RELAY_PIN, RELAY_ON);
            mcp.digitalWrite(FERM_FAN2_RELAY_PIN, RELAY_ON);
          } else {
            currentHeatingPercent = 0;
          }
        }
        if (incomingData.adsStatus == 1 && incomingData.phValue > 0.0f &&
            incomingData.phValue <= fermTargetPH && !phAlertActive) {
          phAlertActive = true;
        }

      } else if (activeBrewStage == 2) {
        if (!pastSterilized) {
          if (liquidTemp > -100.0f) {
            float error = 80.0f - liquidTemp;
            if (liquidTemp >= PID_THROTTLE_TEMP) {
              pidIntegral += error;
              if (pidIntegral > 100.0f)
                pidIntegral = 100.0f;
              if (pidIntegral < -100.0f)
                pidIntegral = -100.0f;
            } else {
              pidIntegral = 0.0f;
            }
            float pidOut = (PID_KP * error) + (PID_KI * pidIntegral) +
                           (PID_KD * (error - pidPrevError));
            pidPrevError = error;
            if (pidOut < 0.0f)
              pidOut = 0.0f;
            if (pidOut > 100.0f)
              pidOut = 100.0f;
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
                pidIntegral = 0.0f;
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
    if (currentAppState == PID_TEST_MENU && pidTestRunning) {
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
      sprintf(buf, "%02d:%02d", n.hour(), n.minute());
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
      tft.setTextColor(TFT_YELLOW, hdrBg);
      tft.drawRightString(buf, 310, 15, 4);
    }

    if (currentAppState == SENSOR_MONITOR)
      drawSensorMonitorPage(true);

    if (currentAppState == CALIBRATION_MODE)
      drawCalibrationPage(true);

    if (currentAppState == LOAD_CELL_PAGE)
      drawLoadCellPage(true);

    if (currentAppState == RAPT_TEST_MENU)
      drawRaptTestPage(true);

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

    if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive &&
        activeBrewStage >= 0) {
      updateDashboardTimers();
      if (!stageTransferring) {
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

    if (currentAppState == DASHBOARD_ACTIVE && moduleViewActive) {
      int y = 110;
      uint16_t colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

      if (dashSelection == 0) {
        tft.setTextColor(TFT_WHITE, colors[0]);
        tft.setTextPadding(140);
        String pa = bme1Status ? String(bme1.readTemperature(), 1) : "--";
        String pl = (simTempOverride[0] > 0.0f)
                        ? String(simTempOverride[0], 1)
                        : (liquid2Status ? String(getPreheatTemp(), 1) : "--");
        tft.drawCentreString(pa + "C", 80, y + 80, 4);
        tft.drawCentreString(pl + "C", 240, y + 80, 4);
        tft.drawCentreString(String(currentSpeedPercent) + "%", 80, y + 160, 4);
        tft.drawCentreString(String(currentHeatingPercent) + "%", 240, y + 160,
                             4);
        const char *mTxt[] = {"OFF", "ON", "AUTO"};
        tft.drawCentreString(mTxt[currentFanMode], 80, y + 230, 2);
        String wStr = hx711Status ? String(currentWeight, 1) + " L" : "--";
        tft.drawCentreString(wStr, 240, y + 230, 4);

      } else if (dashSelection == 1) {
        tft.setTextColor(TFT_WHITE, colors[1]);
        tft.setTextPadding(140);
        String fa = (incomingData.sensor2Status > 0)
                        ? String(incomingData.room2Temp, 1)
                        : "--";
        String fl =
            (simTempOverride[1] > 0.0f)
                ? String(simTempOverride[1], 1)
                : (incomingData.ds18Status == 1 ? String(getFermTemp(), 1)
                                                : "--");
        tft.drawCentreString(fa + "C", 80, y + 85, 4);
        tft.drawCentreString(fl + "C", 240, y + 85, 4);
        if (incomingData.pillGravity != 0 && incomingData.pillGravity < 10.0) {
          dtostrf(incomingData.pillGravity, 5, 3, buf);
          tft.drawCentreString(buf, 80, y + 155, 4);
        } else {
          tft.drawCentreString("--", 80, y + 155, 4);
        }
        if (incomingData.adsStatus == 1) {
          dtostrf(incomingData.phValue, 4, 2, buf);
          tft.drawCentreString(buf, 240, y + 155, 4);
        } else {
          tft.drawCentreString("--", 240, y + 155, 4);
        }
        sprintf(buf, "%d dBm", incomingData.pillRSSI);
        tft.drawCentreString(buf, 80, y + 235, 4);
        sprintf(buf, "%d%%", incomingData.pillBattery);
        tft.drawCentreString(buf, 240, y + 235, 4);
        tft.setTextPadding(280);
        const char *mxTxt[] = {"OFF", "MANUAL", "AUTO"};
        if (currentMixerMode == MIXER_AUTO) {
          tft.drawCentreString(mixerRunning ? "AUTO: RUNNING" : "AUTO: STANDBY",
                               CENTER_X, y + 310, 2);
        } else {
          sprintf(buf, "%s: %d%%", mxTxt[currentMixerMode], mixerSpeedPercent);
          tft.drawCentreString(buf, CENTER_X, y + 310, 2);
        }
        if (originalGravity > 0 && incomingData.pillGravity > 0 &&
            incomingData.pillGravity < 10.0) {
          float abv = (originalGravity - incomingData.pillGravity) * 131.25f;
          if (abv < 0.0f)
            abv = 0.0f;
          dtostrf(abv, 4, 2, buf);
          strcat(buf, "%");
        } else {
          strcpy(buf, "--");
        }
        tft.drawCentreString(buf, CENTER_X, y + 344, 2);

      } else if (dashSelection == 2) {
        tft.setTextColor(TFT_WHITE, colors[2]);
        tft.setTextPadding(280);
        String pt = (simTempOverride[2] > 0.0f)
                        ? String(simTempOverride[2], 1)
                        : (liquid1Status ? String(getPastTemp(), 1) : "--");
        tft.drawCentreString(pt + "C", CENTER_X, y + 85, 4);
        tft.drawCentreString("READY", CENTER_X, y + 155, 4);
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
