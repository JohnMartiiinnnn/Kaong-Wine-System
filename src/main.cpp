/*
 * WIRING FOR MAIN ESP32:
 * SD Card: CS->5, MISO->19 (LCD MISO Disconnected), MOSI->23, SCLK->18
 * OneWire (Liquid Temps, shared bus): Data->26
 * RTC (DS3231): SDA->21, SCL->22
 * TFT LCD: CS->15, DC->2, RST->4, MOSI->23, SCLK->18
 * TOUCHSCREEN: CS->33, CLK->18, DIN->23, DO->19
 * AC DIMMER 1: CH1->14, CH2->12
 * AC DIMMER 2: SHARED->13
 * AC ZERO-CROSS: 32
 * MCP23017 GPB0-7: 8-Channel Relay Module
 * FAN RELAY: GPA7 (MCP)
 * KEYPAD: RIGHT->GPA0, LEFT->GPA1, UP->GPA2, DOWN->GPA3, SELECT->GPA4 (MCP)
 * EMERGENCY STOP: GPA6 (MCP)
 */

#include "config.h"
#include "display.h"
#include "logging.h"
#include "server.h"

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
bool relayTestNeedsFullRedraw = true;
int relayTestChannel = 0;
uint32_t relayTestTimer = 0;
bool motorTestNeedsFullRedraw = true;
int motorTestSpeed = 0;
bool motorTestCW = true;

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
  // Pre-Heating logic: 0% = 255 PWM, 100% = 0 PWM
  // Fermentation logic: 0% = 0 PWM, 100% = 255 PWM
  if (currentAppState == FAN_TEST_MENU && fanTestFanChoice == 1) {
    ledcWrite(pwmChannel, map(percent, 0, 100, 0, 255));
  } else {
    ledcWrite(pwmChannel, map(percent, 0, 100, 255, 0));
  }
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

// ---- Setup ----
void setup() {
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
  pinMode(AC_ZC_PIN, INPUT_PULLUP);
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
        case COOLING_MENU:     drawCoolingMenu();   break;
        case MIXER_MENU:       drawMixerMenu();     break;
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
        default:
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
      wizardSelection = (wizardSelection + 1) % 2;
      drawNewBrewWizard();
    } else if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive) {
      dashSelection = (dashSelection + 1) % 3;
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
      calSelection = (calSelection + 1) % 3;
      drawCalibrationPage();
    } else if (currentAppState == LOAD_CELL_PAGE) {
      loadCellSelection = (loadCellSelection + 1) % 2;
      drawLoadCellPage();
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      systemCheckSelection = (systemCheckSelection + 1) % 4;
      drawSystemCheckMenu();
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
    }
  }

  // Navigation: Up
  if (cUp && !ljUp) {
    if (currentAppState == START_MENU) {
      menuSelection = (menuSelection + 3) % 4;
      drawStartMenu();
    } else if (currentAppState == NEW_BREW_WIZARD) {
      wizardSelection = (wizardSelection + 1) % 2;
      drawNewBrewWizard();
    } else if (currentAppState == DASHBOARD_ACTIVE && !moduleViewActive) {
      dashSelection = (dashSelection + 2) % 3;
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
      calSelection = (calSelection + 2) % 3;
      drawCalibrationPage();
    } else if (currentAppState == LOAD_CELL_PAGE) {
      loadCellSelection = (loadCellSelection + 1) % 2;
      drawLoadCellPage();
    } else if (currentAppState == SYSTEM_CHECK_MENU) {
      systemCheckSelection = (systemCheckSelection + 3) % 4;
      drawSystemCheckMenu();
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
    }
  }

  // Navigation: Select
  if (cSelect && !ljSelect) {
    if (currentAppState == START_MENU) {
      if (menuSelection == 0) {
        currentAppState = NEW_BREW_WIZARD;
        wizardNeedsFullRedraw = true;
        wizardSelection = 0;
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
      if ((wizardSelection == 0 && currentWeight > 10.0) ||
          wizardSelection == 1) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          sprintf(brewStartTime, "%02d/%02d %02d:%02d", now.day(), now.month(),
                  now.hour(), now.minute());
        }
        originalGravity = 0.0f;
        ogSampleSum = 0.0f;
        ogSampleCount = 0;
        ogCapturing = true;
        currentAppState = DASHBOARD_ACTIVE;
        dashNeedsFullRedraw = true;
        moduleViewActive = false;
        drawDashboardLayout();
      }
    } else if (currentAppState == DASHBOARD_ACTIVE) {
      if (!moduleViewActive) {
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
        relayTestChannel = 0;
        relayTestNeedsFullRedraw = true;
        relayTestTimer = millis();
        mcp.digitalWrite(RELAY_PINS[0], RELAY_ON);
        currentAppState = RELAY_TEST_MENU;
        drawRelayTestMenu();
      } else {
        motorTestSpeed = 0;
        motorTestCW = true;
        motorTestNeedsFullRedraw = true;
        sendMotorCommand(0, true);
        currentAppState = MOTOR_TEST_MENU;
        drawMotorTestMenu();
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
      motorTestSpeed = 0;
      sendMotorCommand(0, motorTestCW);
      drawMotorTestMenu();
    } else if (currentAppState == SENSOR_MONITOR) {
      currentAppState = LOAD_CELL_PAGE;
      loadCellNeedsFullRedraw = true;
      loadCellSelection = 0;
      drawLoadCellPage();
    } else if (currentAppState == CALIBRATION_MODE) {
      if (calSelection == 0) {
        if (hx711Status) {
          scale.tare();
          currentWeight = 0.0f;
          hx711WeightSeeded = false;
        }
        drawCalibrationPage();
      } else if (calSelection == 2) {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      if (loadCellSelection == 0 && hx711Status) {
        scale.tare();
        currentWeight = 0.0f;
        hx711WeightSeeded = false;
        drawLoadCellPage();
      }
    }
  }

  // Calibration factor adjust via Right/Left while on factor row
  if (cRight && !ljRight && currentAppState == CALIBRATION_MODE &&
      calSelection == 1) {
    calibrationFactor += 10.0;
    scale.set_scale(calibrationFactor);
    drawCalibrationPage();
  }
  if (cRight && !ljRight && currentAppState == LOAD_CELL_PAGE &&
      loadCellSelection == 1) {
    calibrationFactor += 10.0;
    scale.set_scale(calibrationFactor);
    drawLoadCellPage();
  }

  if (cRight && !ljRight && currentAppState == MOTOR_TEST_MENU) {
    motorTestCW = !motorTestCW;
    sendMotorCommand(motorTestSpeed, motorTestCW);
    drawMotorTestMenu();
  }

  // Navigation: Left / Return
  if (cLeft && !ljLeft) {
    if (currentAppState == NEW_BREW_WIZARD) {
      currentAppState = START_MENU;
      menuNeedsFullRedraw = true;
      drawStartMenu();
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
      if (calSelection == 1) {
        calibrationFactor -= 10.0;
        scale.set_scale(calibrationFactor);
        drawCalibrationPage();
      } else {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == LOAD_CELL_PAGE) {
      if (loadCellSelection == 1) {
        calibrationFactor -= 10.0;
        scale.set_scale(calibrationFactor);
        drawLoadCellPage();
      } else {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
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
    } else if (currentAppState == RELAY_TEST_MENU) {
      if (relayTestChannel < 8)
        mcp.digitalWrite(RELAY_PINS[relayTestChannel], RELAY_OFF);
      else
        mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    } else if (currentAppState == MOTOR_TEST_MENU) {
      sendMotorCommand(0, motorTestCW);
      motorTestSpeed = 0;
      currentAppState = SYSTEM_CHECK_MENU;
      systemCheckNeedsFullRedraw = true;
      drawSystemCheckMenu();
    }
  }

  ljRight = cRight;
  ljLeft = cLeft;
  ljUp = cUp;
  ljDown = cDown;
  ljSelect = cSelect;

  // Relay test auto-advance (500ms per channel)
  if (currentAppState == RELAY_TEST_MENU && millis() - relayTestTimer > 500) {
    if (relayTestChannel < 8)
      mcp.digitalWrite(RELAY_PINS[relayTestChannel], RELAY_OFF);
    else
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
    relayTestChannel = (relayTestChannel + 1) % 9;
    if (relayTestChannel < 8)
      mcp.digitalWrite(RELAY_PINS[relayTestChannel], RELAY_ON);
    else
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_ON);
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
      incomingData.pillGravity += GRAVITY_OFFSET;
      lastDataReceivedMillis = millis();
      if (ogCapturing && incomingData.pillGravity > 0.5f) {
        ogSampleSum += incomingData.pillGravity;
        ogSampleCount++;
        if (ogSampleCount >= OG_SAMPLES) {
          originalGravity = ogSampleSum / ogSampleCount;
          ogCapturing = false;
        }
      }
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
      // cells
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
    tft.setTextPadding(0);

    if (liquid1Status || liquid2Status)
      sharedLiquidSensors.requestTemperatures();

    if (rtcStatus && currentAppState != SENSOR_MONITOR) {
      DateTime n = rtc.now();
      sprintf(buf, "%02d:%02d", n.hour(), n.minute());
      tft.setTextColor(TFT_YELLOW, TFT_NAVY);
      tft.drawRightString(buf, 310, 15, 4);
    }

    if (currentAppState == SENSOR_MONITOR)
      drawSensorMonitorPage(true);

    if (currentAppState == CALIBRATION_MODE)
      drawCalibrationPage(true);

    if (currentAppState == LOAD_CELL_PAGE)
      drawLoadCellPage(true);

    if (currentAppState == DASHBOARD_ACTIVE && moduleViewActive) {
      int y = 110;
      uint16_t colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

      if (dashSelection == 0) {
        tft.setTextColor(TFT_WHITE, colors[0]);
        tft.setTextPadding(140);
        String pa = bme1Status ? String(bme1.readTemperature(), 1) : "--";
        String pl = liquid1Status
                        ? String(sharedLiquidSensors.getTempCByIndex(0), 1)
                        : "--";
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
        String fl = (incomingData.ds18Status == 1)
                        ? String(incomingData.room2LiquidTemp, 1)
                        : "--";
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
        String pt = liquid2Status
                        ? String(sharedLiquidSensors.getTempCByIndex(1), 1)
                        : "--";
        tft.drawCentreString(pt + "C", CENTER_X, y + 85, 4);
        tft.drawCentreString("READY", CENTER_X, y + 155, 4);
      }
    }
  }
}
