/*
 * WIRING FOR MAIN ESP32:
 * SD Card: CS->5, MISO->19 (LCD MISO Disconnected), MOSI->23, SCLK->18
 * OneWire 1 (Liquid Temp): Data->27
 * OneWire 2 (Secondary Liquid Temp): Data->26
 * RTC (DS3231): SDA->21, SCL->22
 * TFT LCD: CS->15, DC->2, RST->4, MOSI->23, SCLK->18
 * TOUCHSCREEN: CS->33, CLK->18, DIN->23, DO->19
 * AC DIMMER 1: CH1->14, CH2->12
 * AC DIMMER 2: SHARED->13
 * AC ZERO-CROSS: 32
 * MCP23017 GPA0-7: 8-Channel Relay Module
 * FAN RELAY: GPB7 (MCP)
 * KEYPAD: RIGHT->GPB0, LEFT->GPB1, UP->GPB2, DOWN->GPB3, SELECT->GPB4 (MCP)
 * EMERGENCY STOP: GPB6 (MCP)
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

// ---- Sensor & Brew Data ----
struct_message incomingData = {};
uint32_t lastDataReceivedMillis = 0;
float currentWeight = 0.0;
float calibrationFactor = 22742.666;
String currentLogFile = "/data_log.csv";
char lastLogTime[10] = "--:--";
char brewStartTime[32] = "NOT STARTED";

// ---- Actuator State ----
int currentSpeedPercent = 0;
int currentHeatingPercent = 0;
bool isFanOn = false;
FanMode currentFanMode = FAN_OFF;
bool isSystemHalted = false;
int estopState = 0;
uint32_t estopTimer = 0;
int returnConfirmState = 0;
uint32_t returnConfirmTimer = 0;

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

// ---- Button Latch State ----
bool ljRight = false, ljLeft = false, ljUp = false, ljDown = false,
     ljSelect = false;
bool lastLjRight = false, lastLjLeft = false, lastLjUp = false,
     lastLjDown = false, lastLjSelect = false;

// ---- Fan Speed Helper ----
void setFanSpeed(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  currentSpeedPercent = percent;
  ledcWrite(pwmChannel, map(percent, 0, 100, 255, 0));
}

// ---- Setup ----
void setup() {
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, HIGH);

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
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  WiFi.softAP("WineBrew_System", "12345678");
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
  static uint32_t ld = 0, ll = 0, ls = 0, lw = 0;
  static bool lsd = !sdStatus, les = HIGH;
  char buf[64];

  // Emergency stop
  bool ces = mcp.digitalRead(ESTOP_BUTTON_PIN);
  if (ces == LOW && les == HIGH) {
    if (estopState == 0 && !isSystemHalted) {
      estopState = 1;
      drawEstopPage();
    } else if (estopState == 1) {
      isSystemHalted = true;
      estopState = 0;
      for (int i = 0; i < 8; i++)
        mcp.digitalWrite(RELAY_PINS[i], RELAY_OFF);
      mcp.digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
      setFanSpeed(0);
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("SYSTEM HALTED", 160, 100, 4);
      tft.drawCentreString("REBOOT TO RESET", 160, 200, 2);
    }
  }
  les = ces;
  if (isSystemHalted)
    return;

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
      if (currentAppState == START_MENU) {
        menuNeedsFullRedraw = true;
        drawStartMenu();
      } else {
        dashNeedsFullRedraw = true;
        drawDashboardLayout();
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
    } else if (currentAppState == CALIBRATION_MODE) {
      calSelection = (calSelection + 1) % 3;
      drawCalibrationPage();
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
    } else if (currentAppState == CALIBRATION_MODE) {
      calSelection = (calSelection + 2) % 3;
      drawCalibrationPage();
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
        ESP.restart();
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
      }
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
    } else if (currentAppState == SENSOR_MONITOR) {
      currentAppState = CALIBRATION_MODE;
      calNeedsFullRedraw = true;
      calSelection = 0;
      drawCalibrationPage();
    } else if (currentAppState == CALIBRATION_MODE) {
      if (calSelection == 0) {
        if (hx711Status)
          scale.tare();
        drawCalibrationPage();
      } else if (calSelection == 2) {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
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
    }
  }

  ljRight = cRight;
  ljLeft = cLeft;
  ljUp = cUp;
  ljDown = cDown;
  ljSelect = cSelect;

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
      lastDataReceivedMillis = millis();
    }
  }

  // HX711 EMA with spike rejection
  if (hx711Status && scale.is_ready()) {
    float rawW = scale.get_units(1);
    static bool weightSeeded = false;
    if (!weightSeeded) {
      currentWeight = rawW;
      weightSeeded = true;
    } else if (abs(rawW - currentWeight) < 3.0) {
      // reject spikes >3L from current — only smooth plausible readings
      currentWeight = (currentWeight * 0.95f) + (rawW * 0.05f);
    }
    // snap to zero when the vat is effectively empty
    if (currentWeight < 0.3f && currentWeight > -0.3f)
      currentWeight = 0.0f;
  }

  // Wizard weight fast refresh (250ms) — only wizard needs this rate;
  // sensor/cal pages stay on 1s to avoid starving the HX711 EMA with
  // scale.read()
  if (currentAppState == NEW_BREW_WIZARD && millis() - lw > 250) {
    lw = millis();
    drawNewBrewWizard();
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
