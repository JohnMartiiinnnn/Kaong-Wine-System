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

// --- Hardware Setup ---
Adafruit_MCP23X17 mcp;
RTC_DS3231 rtc;

// --- AC Dimmer Pins ---
const int DIM1_CH1 = 14;
const int DIM1_CH2 = 12;
const int DIM2_SHARED = 13;
const int AC_ZC_PIN = 32;

// --- OneWire Pins ---
const int ONE_WIRE_BUS = 26;

// --- MCP23017 Pins ---
const int RELAY_PINS[] = {0, 1, 2, 3, 4, 5, 6, 7}; // GPA0-GPA7
const int FAN_RELAY_PIN = 15;                      // GPB7
const int BTN_RIGHT_PIN = 8;                       // GPB0
const int BTN_LEFT_PIN = 9;                        // GPB1
const int BTN_UP_PIN = 10;                         // GPB2
const int BTN_DOWN_PIN = 11;                       // GPB3
const int BTN_SELECT_PIN = 12;                     // GPB4
const int ESTOP_BUTTON_PIN = 14;                   // GPB6

#define RELAY_ON LOW
#define RELAY_OFF HIGH

const int PWM_PIN = 25;
const int SD_CS_PIN = 5;

// --- PWM Settings ---
const int pwmFreq = 25000;
const int pwmChannel = 0;
const int pwmResolution = 8;

// --- HX711 Settings ---
const int HX711_DT_PIN = 36;
const int HX711_SCK_PIN = 27;
HX711 scale;
bool hx711Status = false;
float currentWeight = 0.0;
long loadCellOffset = 471598;        // Constant tare offset
float calibrationFactor = 22742.666; // Updated from loadcell.cpp

// --- Variables ---
int currentSpeedPercent = 0;
int currentHeatingPercent = 0;
bool isFanOn = false;
enum FanMode { FAN_OFF, FAN_ON, FAN_AUTO };
FanMode currentFanMode = FAN_OFF;

bool isSystemHalted = false;
int estopState = 0; // 0: Normal, 1: Full-Screen Confirm
uint32_t estopTimer = 0;

int returnConfirmState = 0; // 0: Normal, 1: Orange Confirm Page
uint32_t returnConfirmTimer = 0;

char lastLogTime[10] = "--:--";
char brewStartTime[32] = "NOT STARTED";
String currentLogFile = "/data_log.csv";

// Button state for debouncing
bool ljRight = false, ljLeft = false, ljUp = false, ljDown = false,
     ljSelect = false;
bool lastLjRight = false, lastLjLeft = false, lastLjUp = false,
     lastLjDown = false, lastLjSelect = false;

// Application State Machine
enum AppState {
  SYSTEM_INIT,
  START_MENU,
  NEW_BREW_WIZARD,
  DASHBOARD_ACTIVE,
  COOLING_MENU,
  SENSOR_MONITOR,
  CALIBRATION_MODE
};
AppState currentAppState = SYSTEM_INIT;
bool menuNeedsFullRedraw = true;
int menuSelection =
    0; // 0: New Brew, 1: Continue, 2: System Check, 3: Sensor Values
bool wizardNeedsFullRedraw = true;
int wizardSelection = 0; // 0: Proceed, 1: Bypass
bool dashNeedsFullRedraw = true;
int dashSelection = 0; // 0: Pre-Heating, 1: Fermentation, 2: Pasteurization
bool moduleViewActive = false;
int lastDashSelection = -1;
bool monitorNeedsFullRedraw = true;
bool calNeedsFullRedraw = true;
int calSelection = 0; // 0: Tare, 1: Set Factor, 2: Save & Exit

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

struct_message incomingData;
uint32_t lastDataReceivedMillis = 0;

uint8_t calculateChecksum(const struct_message &msg) {
  uint8_t checksum = 0;
  const uint8_t *ptr = (const uint8_t *)&msg;
  for (size_t i = 0; i < sizeof(struct_message) - 1; i++)
    checksum ^= ptr[i];
  return checksum;
}

TFT_eSPI tft = TFT_eSPI();
Adafruit_BME280 bme1;
OneWire sharedOneWire(ONE_WIRE_BUS);
DallasTemperature sharedLiquidSensors(&sharedOneWire);

bool bme1Status = false;
bool liquid1Status = false;
bool liquid2Status = false;
bool remoteStatusReceived = false;
bool rtcStatus = false;
bool sdStatus = false;

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WineBrew System Monitor</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f172a;color:#e2e8f0;font-family:system-ui,-apple-system,sans-serif;padding:1rem;max-width:600px;margin:0 auto}
.card{background:#1e293b;border-radius:12px;padding:1.25rem;margin-bottom:1rem;border:1px solid #334155}
h1{font-size:1.2rem;margin-bottom:1.5rem;text-align:center;color:#38bdf8}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:1rem}
.label{font-size:.7rem;color:#94a3b8;text-transform:uppercase;letter-spacing:.05em}
.val{font-size:1.5rem;font-weight:700;margin-top:.25rem}
.unit{font-size:.9rem;color:#64748b;margin-left:.2rem}
.status{display:inline-block;padding:.2rem .5rem;border-radius:4px;font-size:.7rem;font-weight:700}
.good{background:#064e3b;color:#34d399}
.warn{background:#78350f;color:#fbbf24}
</style>
</head>
<body>
<h1>SYSTEM MONITOR</h1>
<div class="card" style="text-align:center">
  <div class="label">Sap Volume</div>
  <div class="val" id="vol">--</div><span class="unit">Liters</span>
</div>
<div class="grid">
  <div class="card"><div class="label">Local Ambient</div><div class="val" id="la">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Local Liquid</div><div class="val" id="ll">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Ferm Ambient</div><div class="val" id="fa">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">Ferm Liquid</div><div class="val" id="fl">--</div><span class="unit">C</span></div>
  <div class="card"><div class="label">pH Level</div><div class="val" id="ph">--</div></div>
  <div class="card"><div class="label">Gravity</div><div class="val" id="sg">--</div></div>
</div>
<script>
async function update(){
  try{
    const r=await fetch('/data');
    const d=await r.json();
    document.getElementById('vol').innerText=d.vol.toFixed(2);
    document.getElementById('la').innerText=d.la.toFixed(1);
    document.getElementById('ll').innerText=d.ll.toFixed(1);
    document.getElementById('fa').innerText=d.fa.toFixed(1);
    document.getElementById('fl').innerText=d.fl.toFixed(1);
    document.getElementById('ph').innerText=d.ph.toFixed(2);
    document.getElementById('sg').innerText=d.sg.toFixed(4);
  }catch(e){}
}
setInterval(update,1000);
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }
void handleData() {
  String json = "{";
  json += "\"vol\":" + String(currentWeight) + ",";
  json += "\"la\":" + String(bme1Status ? bme1.readTemperature() : 0) + ",";
  json += "\"ll\":" +
          String(liquid1Status ? sharedLiquidSensors.getTempCByIndex(0) : 0) +
          ",";
  json += "\"fa\":" + String(incomingData.room2Temp) + ",";
  json += "\"fl\":" + String(incomingData.room2LiquidTemp) + ",";
  json += "\"ph\":" + String(incomingData.phValue) + ",";
  json += "\"sg\":" + String(incomingData.pillGravity);
  json += "}";
  server.send(200, "application/json", json);
}

const int CENTER_X = 160;

void setFanSpeed(int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  currentSpeedPercent = percent;
  int pwmValue = map(percent, 0, 100, 255, 0);
  ledcWrite(pwmChannel, pwmValue);
}

void logDataToSD() {
  if (!sdStatus || !rtcStatus)
    return;
  DateTime now = rtc.now();
  bool fileExists = SD.exists(currentLogFile);
  File dataFile = SD.open(currentLogFile, FILE_APPEND);
  if (dataFile) {
    if (!fileExists)
      dataFile.println("Date,Time,LocalTemp,LocalLiquid1,LocalLiquid2,"
                       "RemoteTemp,RemoteLiquid,Gravity,pH");
    sprintf(lastLogTime, "%02d:%02d", now.hour(), now.minute());
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(',');
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(',');
    dataFile.print(bme1Status ? bme1.readTemperature() : 0.0);
    dataFile.print(',');
    dataFile.print(liquid1Status ? sharedLiquidSensors.getTempCByIndex(0)
                                 : 0.0);
    dataFile.print(',');
    dataFile.print(liquid2Status ? sharedLiquidSensors.getTempCByIndex(1)
                                 : 0.0);
    dataFile.print(',');
    dataFile.print(incomingData.room2Temp);
    dataFile.print(',');
    dataFile.print(incomingData.ds18Status == 1 ? incomingData.room2LiquidTemp
                                                : 0.0);
    dataFile.print(',');
    dataFile.print(incomingData.pillGravity, 4);
    dataFile.print(',');
    dataFile.print(incomingData.phValue, 2);
    dataFile.println();
    dataFile.close();
  }
}

// --- UI Layouts ---

void drawSplashScreen() {
  // Colors translated from YAML hex
  uint16_t bg = tft.color565(28, 49, 28); // 0x1C311C

  tft.fillScreen(bg);

  int cx = 160;

  // Drop Icon (lv_icon_1)
  tft.fillCircle(cx, 145, 15, TFT_WHITE);
  tft.fillTriangle(cx - 15, 145, cx + 15, 145, cx, 115, TFT_WHITE);

  // Title (lv_label_11)
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Automated Wine", cx, 180, 4);
  tft.drawCentreString("Brewing System", cx, 210, 4);

  // Line (lv_line_30)
  tft.drawFastHLine(80, 240, 160, TFT_WHITE);

  // Static Initializing text
  tft.drawCentreString("Initializing", cx, 360, 2);
}

void drawReturnConfirmation() {
  tft.fillScreen(TFT_ORANGE);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("RETURN TO MAIN MENU?", 160, 150, 4);
  tft.drawCentreString("PRESS RETURN AGAIN", 160, 220, 2);
  tft.drawCentreString("TO CONFIRM", 160, 250, 2);
}

void drawEstopPage() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(10, 10, 300, 460, TFT_RED);
  tft.setTextColor(TFT_RED);
  tft.drawCentreString("EMERGENCY STOP", 160, 100, 4);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("ARE YOU SURE?", 160, 180, 2);
  tft.drawCentreString("CLICK AGAIN TO HALT", 160, 220, 2);
  tft.drawCentreString("RETURN TO CANCEL", 160, 260, 2);
}

void drawCoolingMenu() {
  tft.fillScreen(TFT_WHITE);
  tft.fillRect(0, 0, 320, 50, 0x03E0); // Dark Green Header
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("COOLING CONTROL", CENTER_X, 15, 4);

  const char *modeTxt[] = {"OFF", "ON", "AUTO"};
  uint16_t modeColors[] = {TFT_RED, 0x0400, 0x03E0};

  // Mode Tile
  tft.fillRect(20, 80, 280, 120, modeColors[currentFanMode]);
  tft.drawRect(20, 80, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, modeColors[currentFanMode]);
  tft.drawCentreString("FAN MODE", CENTER_X, 100, 2);
  tft.drawCentreString(modeTxt[currentFanMode], CENTER_X, 130, 4);
  tft.drawCentreString("(SELECT TO CYCLE)", CENTER_X, 175, 2);

  // Speed Tile
  tft.fillRect(20, 220, 280, 120, 0xD6BA);
  tft.drawRect(20, 220, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("FAN SPEED", CENTER_X, 240, 2);
  char buf[32];
  sprintf(buf, "%d%%", currentSpeedPercent);
  tft.drawCentreString(buf, CENTER_X, 270, 4);
  tft.drawCentreString("(NEXT TO ADJ)", CENTER_X, 315, 2);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 420, 2);
}

void drawNewBrewWizard() {
  if (wizardNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("NEW BREW SETUP", CENTER_X, 15, 4);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("ADD LIQUID TO VAT", CENTER_X, 70, 2);
    tft.drawCentreString("MINIMUM 10.0 L REQUIRED", CENTER_X, 90, 2);
    tft.drawCentreString("RETURN TO CANCEL", CENTER_X, 450, 2);
    wizardNeedsFullRedraw = false;
  }

  // Draw Current Volume
  tft.fillRect(20, 130, 280, 80, 0xD6BA);
  tft.drawRect(20, 130, 280, 80, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("CURRENT VOLUME", CENTER_X, 145, 2);
  char buf[32];
  if (hx711Status)
    sprintf(buf, "%.1f L", currentWeight);
  else
    strcpy(buf, "FAILED");
  tft.drawCentreString(buf, CENTER_X, 175, 4);

  // Draw Buttons
  bool canProceed = (currentWeight > 10.0);
  const char *options[] = {"PROCEED", "BYPASS (TEST)"};
  for (int i = 0; i < 2; i++) {
    int y = 250 + (i * 70);
    uint16_t bgColor = TFT_DARKGREY; // Disabled by default
    uint16_t txtColor = TFT_WHITE;

    if (i == 0) { // Proceed Button
      if (canProceed)
        bgColor =
            (wizardSelection == 0) ? 0x03E0 : 0x0400; // Green / Dark Green
      else
        bgColor = TFT_DARKGREY; // Keep grey if can't proceed
    } else {                    // Bypass Button
      bgColor = (wizardSelection == 1) ? TFT_ORANGE : 0x9000; // Orange / Red
    }

    if (wizardSelection == i) {
      tft.drawRect(18, y - 2, 284, 54, TFT_BLACK);
    } else {
      tft.drawRect(18, y - 2, 284, 54, TFT_WHITE); // erase old selection border
    }

    tft.fillRect(20, y, 280, 50, bgColor);
    tft.drawRect(20, y, 280, 50, TFT_DARKGREY);
    tft.setTextColor(txtColor, bgColor);
    tft.drawCentreString(options[i], CENTER_X, y + 15, 2);
  }
}

void drawDashboardLayout() {
  if (dashNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("DASHBOARD", 10, 15, 4);
    // Brew Info
    tft.setTextPadding(0);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    char startBuf[64];
    sprintf(startBuf, "STARTED: %s", brewStartTime);
    tft.drawString(startBuf, 15, 60, 2);

    // SD Status
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("SD CARD:", 15, 80, 2);
    uint16_t sdBg = sdStatus ? 0x0400 : TFT_RED;
    tft.setTextColor(TFT_WHITE, sdBg);
    tft.drawString(sdStatus ? " READY " : " ERROR ", 75, 80, 2);

    // Log Status
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("LOG:", 190, 80, 2);
    char lBuf[16];
    sprintf(lBuf, " %s ", lastLogTime);
    tft.drawString(lBuf, 230, 80, 2);

    dashNeedsFullRedraw = false;
  }

  const char *titles[] = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  uint16_t colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

  if (!moduleViewActive) {
    for (int i = 0; i < 3; i++) {
      int y = 110 + (i * 60);

      if (i == dashSelection) {
        tft.drawRect(3, y - 2, 314, 54, TFT_BLACK);
      } else {
        tft.drawRect(3, y - 2, 314, 54,
                     TFT_WHITE); // erase old selection border
      }

      tft.fillRect(5, y, 310, 50, colors[i]);
      tft.drawRect(5, y, 310, 50, TFT_DARKGREY);

      uint16_t txtColor = TFT_WHITE;
      tft.setTextColor(txtColor, colors[i]);
      tft.drawCentreString(titles[i], CENTER_X, y + 15, 2);
    }
    tft.fillRect(0, 290, 320, 190, TFT_WHITE);
  } else {
    int y = 110;
    int h = 360;
    tft.fillRect(5, y, 310, h, colors[dashSelection]);
    tft.drawRect(5, y, 310, h, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(titles[dashSelection], CENTER_X, y + 10, 4);
    tft.drawFastHLine(20, y + 45, 280, TFT_WHITE);

    if (dashSelection == 0) {
      tft.drawCentreString("AMBIENT", 80, y + 55, 2);
      tft.drawCentreString("LIQUID", 240, y + 55, 2);
      tft.drawCentreString("COOLING", 80, y + 130, 2);
      tft.drawCentreString("HEATING", 240, y + 130, 2);
      tft.drawCentreString("FAN MODE", 80, y + 210, 2);
      tft.drawCentreString("WEIGHT", 240, y + 210, 2);
    } else if (dashSelection == 1) {
      tft.drawCentreString("AMBIENT", 80, y + 60, 2);
      tft.drawCentreString("LIQUID", 240, y + 60, 2);
      tft.drawCentreString("S. GRAVITY", 80, y + 130, 2);
      tft.drawCentreString("PH LEVEL", 240, y + 130, 2);
      tft.drawCentreString("SIGNAL", 80, y + 210, 2);
      tft.drawCentreString("BATTERY", 240, y + 210, 2);
    } else if (dashSelection == 2) {
      tft.drawCentreString("PAST. TEMP", CENTER_X, y + 60, 2);
      tft.drawCentreString("PROCESS STATUS", CENTER_X, y + 130, 2);
    }
  }
  lastDashSelection = dashSelection;
}

void drawStartMenu() {
  if (menuNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("MAIN MENU", CENTER_X, 15, 4);
    menuNeedsFullRedraw = false;
  }
  const char *options[] = {"NEW BREW", "CONTINUE BREW", "SYSTEM CHECK",
                           "SENSOR VALUES"};
  for (int i = 0; i < 4; i++) {
    uint16_t color = (menuSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (menuSelection == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 70), 280, 50, color);
    tft.drawRect(20, 80 + (i * 70), 280, 50, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 95 + (i * 70), 2);
  }
}

void drawValueTile(int x, int y, const char *label, String value,
                   bool isError) {
  uint16_t bgColor = isError ? TFT_RED : 0x3566;
  uint16_t textColor = TFT_WHITE;
  tft.fillRect(5, y, 310, 45, bgColor);
  tft.drawRect(5, y, 310, 45, TFT_DARKGREY);
  tft.setTextColor(textColor);
  tft.drawString(label, 20, y + 13, 2);
  tft.drawRightString(value, 300, y + 13, 2);
}

void drawCalibrationValueTile(int y, const char *label, String value,
                              bool isSelected) {
  uint16_t bgColor = isSelected ? 0x3566 : 0xD6BA;
  uint16_t textColor = isSelected ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, y, 300, 60, bgColor);
  tft.drawRect(10, y, 300, 60, TFT_DARKGREY);
  tft.setTextColor(textColor);
  tft.drawString(label, 25, y + 22, 2);
  tft.drawRightString(value, 285, y + 22, 2);
}

void drawCalibrationPage() {
  if (calNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x9000); // Dark Red Header
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SCALE CALIBRATION", CENTER_X, 15, 4);
    calNeedsFullRedraw = false;
  }

  char b[32];
  // Instructions
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("1. EMPTY VAT & TARE", CENTER_X, 60, 2);
  tft.drawCentreString("2. ADD KNOWN WEIGHT", CENTER_X, 80, 2);
  tft.drawCentreString("3. ADJUST FACTOR", CENTER_X, 100, 2);

  // Tare Option
  drawCalibrationValueTile(130, "TARE SCALE", "SELECT", calSelection == 0);

  // Calibration Factor Option
  sprintf(b, "%.1f", calibrationFactor);
  drawCalibrationValueTile(200, "CAL. FACTOR", b, calSelection == 1);

  // Current Reading (Live)
  tft.fillRect(10, 280, 300, 80, 0xCE79);
  tft.drawRect(10, 280, 300, 80, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("CURRENT READING", CENTER_X, 295, 2);
  sprintf(b, "%.2f L", currentWeight);
  tft.drawCentreString(b, CENTER_X, 320, 4);

  // Save & Exit
  drawCalibrationValueTile(380, "SAVE & EXIT", "GO BACK", calSelection == 2);
}

void drawSensorMonitorPage() {
  if (monitorNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("KEY VALUES", CENTER_X, 15, 4);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("SELECT: CALIBRATE", CENTER_X, 420, 2);
    tft.drawCentreString("RETURN: GO BACK", CENTER_X, 450, 2);
    monitorNeedsFullRedraw = false;
  }

  int yStart = 60;
  int yGap = 47;
  char b[32];

  // Ambient PH (BME1)
  if (bme1Status) {
    sprintf(b, "%.1f C", bme1.readTemperature());
    drawValueTile(5, yStart + (yGap * 0), "AMBIENT (PH)", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 0), "AMBIENT (PH)", "FAILED", true);

  // Liquid PH (DS1)
  if (liquid1Status) {
    sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(0));
    drawValueTile(5, yStart + (yGap * 1), "LIQUID (PH)", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 1), "LIQUID (PH)", "FAILED", true);

  // Ambient Ferm (Remote BME2)
  if (incomingData.sensor2Status > 0) {
    sprintf(b, "%.1f C", incomingData.room2Temp);
    drawValueTile(5, yStart + (yGap * 2), "AMBIENT (FERM)", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 2), "AMBIENT (FERM)", "FAILED", true);

  // Liquid Ferm (Remote DS)
  if (incomingData.ds18Status == 1) {
    sprintf(b, "%.1f C", incomingData.room2LiquidTemp);
    drawValueTile(5, yStart + (yGap * 3), "LIQUID (FERM)", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 3), "LIQUID (FERM)", "FAILED", true);

  // Pasteurization Temp (Local DS2)
  if (liquid2Status) {
    sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(1));
    drawValueTile(5, yStart + (yGap * 4), "LIQUID (PST)", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 4), "LIQUID (PST)", "FAILED", true);

  // Gravity (Pill)
  if (incomingData.pillGravity > 0.1) {
    sprintf(b, "%.4f SG", incomingData.pillGravity);
    drawValueTile(5, yStart + (yGap * 5), "S. GRAVITY", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 5), "S. GRAVITY", "FAILED", true);

  // pH Level
  if (incomingData.adsStatus == 1) {
    sprintf(b, "%.2f pH", incomingData.phValue);
    drawValueTile(5, yStart + (yGap * 6), "PH LEVEL", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 6), "PH LEVEL", "FAILED", true);

  // Weight/Volume
  if (hx711Status) {
    sprintf(b, "%.1f L", currentWeight);
    drawValueTile(5, yStart + (yGap * 7), "EST. VOLUME", b, false);
  } else
    drawValueTile(5, yStart + (yGap * 7), "EST. VOLUME", "FAILED", true);

  // Button Debug & Scale Raw
  long sRaw = hx711Status ? scale.read() : 0;
  sprintf(b, "R:%d L:%d U:%d D:%d S:%d W:%ld", ljRight, ljLeft, ljUp, ljDown,
          ljSelect, sRaw);
  drawValueTile(5, yStart + (yGap * 8), "RAW DEBUG", b, false);
}

void drawInitTile(int x, int y, const char *label, int status) {
  uint16_t bgColor = 0xD6BA;
  uint16_t textColor = TFT_BLACK;
  const char *stTxt = "PENDING";
  if (status == 1) {
    bgColor = 0x3566;
    textColor = TFT_WHITE;
    stTxt = "GOOD";
  } else if (status == 2) {
    bgColor = TFT_RED;
    textColor = TFT_WHITE;
    stTxt = "NOT FOUND";
  }
  tft.fillRect(5, y, 310, 45, bgColor);
  tft.drawRect(5, y, 310, 45, TFT_DARKGREY);
  tft.setTextColor(textColor);
  tft.drawString(label, 20, y + 13, 2);
  tft.drawRightString(stTxt, 300, y + 13, 2);
}

void setup() {
  // 1. IMMEDIATE FAN SILENCING
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

  if (bme1.begin(0x76)) {
    bme1Status = true;
  }

  sharedLiquidSensors.begin();
  sharedLiquidSensors.setWaitForConversion(false);

  int deviceCount = sharedLiquidSensors.getDeviceCount();
  if (deviceCount > 0) {
    liquid1Status = true;
  }
  if (deviceCount > 1) {
    liquid2Status = true;
  }

  // Initial request
  sharedLiquidSensors.requestTemperatures();

  // --- Initialize HX711 ---
  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  delay(500); // Give the HX711 ADC time to stabilize before taring
  if (scale.is_ready()) {
    hx711Status = true;
    scale.set_scale(calibrationFactor);
    scale.tare(10); // Tare to actual zero at startup (vat must be empty)
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

  // --- Local Monitoring Setup ---
  WiFi.softAP("WineBrew_System", "12345678");
  if (MDNS.begin("winebrew")) {
    MDNS.addService("http", "tcp", 80);
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();

  delay(2000); // Hold splash for a moment
  currentAppState = START_MENU;
  drawStartMenu();
}

void loop() {
  server.handleClient();
  static uint32_t ld = 0, ll = 0, ls = 0, lw = 0;
  static bool lsd = !sdStatus, les = HIGH;
  char buf[64];

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

  // Right
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

  // Left
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

  // Up
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

  // Down
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

  // Select
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
      } else if (menuSelection == 2)
        ESP.restart();
      else if (menuSelection == 3) {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    } else if (currentAppState == NEW_BREW_WIZARD) {
      if (wizardSelection == 0 && currentWeight > 10.0) {
        if (rtcStatus) {
          DateTime now = rtc.now();
          sprintf(brewStartTime, "%02d/%02d %02d:%02d", now.day(), now.month(),
                  now.hour(), now.minute());
        }
        currentAppState = DASHBOARD_ACTIVE;
        dashNeedsFullRedraw = true;
        moduleViewActive = false;
        drawDashboardLayout();
      } else if (wizardSelection == 1) {
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
      if (calSelection == 0) // Tare
      {
        if (hx711Status)
          scale.tare();
        drawCalibrationPage();
      } else if (calSelection == 2) // Exit
      {
        currentAppState = SENSOR_MONITOR;
        monitorNeedsFullRedraw = true;
        drawSensorMonitorPage();
      }
    }
  }
  if (cRight && !ljRight && currentAppState == CALIBRATION_MODE &&
      calSelection == 1) {
    calibrationFactor += 10.0;
    scale.set_scale(calibrationFactor);
    drawCalibrationPage();
  }
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
    } else if (!sdStatus && acc)
      sdStatus = true;
    else if (!sdStatus && !acc) {
      static uint32_t rst = 0;
      if (millis() - rst > 5000) {
        rst = millis();
        if (SD.begin(SD_CS_PIN, SPI, 400000))
          sdStatus = true;
      }
    }
  }

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

  if (millis() - ll > 60000) {
    ll = millis();
    logDataToSD();
  }

  while (Serial2.available() > 0) {
    if (Serial2.peek() != 0xEF) {
      Serial2.read();
      continue;
    }
    if (Serial2.available() < sizeof(struct_message))
      break;
    struct_message t;
    Serial2.readBytes((uint8_t *)&t, sizeof(t));
    if (t.signature == 0xDEADBEEF && calculateChecksum(t) == t.checksum) {
      incomingData = t;
      lastDataReceivedMillis = millis();
    }
  }

  if (hx711Status && scale.is_ready()) {
    float rawW = scale.get_units(1);
    // Apply deadband to raw reading before EMA so it can't trap the filter at
    // zero
    if (rawW > -0.15 && rawW < 0.15)
      rawW = 0.0;
    static bool weightSeeded = false;
    if (!weightSeeded) {
      currentWeight = rawW; // Seed EMA with first real reading
      weightSeeded = true;
    } else {
      currentWeight = (currentWeight * 0.9) + (rawW * 0.1);
    }
  }

  // Wizard weight tile refreshes at 250ms so pouring feels responsive
  if (currentAppState == NEW_BREW_WIZARD && millis() - lw > 250) {
    lw = millis();
    drawNewBrewWizard();
  }

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
    if (currentAppState == SENSOR_MONITOR) {
      drawSensorMonitorPage();
    }
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
        } else
          tft.drawCentreString("--", 80, y + 155, 4);
        if (incomingData.adsStatus == 1) {
          dtostrf(incomingData.phValue, 4, 2, buf);
          tft.drawCentreString(buf, 240, y + 155, 4);
        } else
          tft.drawCentreString("--", 240, y + 155, 4);
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
