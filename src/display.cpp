#include "display.h"

void drawSplashScreen() {
  uint16_t bg = tft.color565(28, 49, 28);
  tft.fillScreen(bg);
  int cx = 160;
  tft.fillCircle(cx, 145, 15, TFT_WHITE);
  tft.fillTriangle(cx - 15, 145, cx + 15, 145, cx, 115, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Automated Wine", cx, 180, 4);
  tft.drawCentreString("Brewing System", cx, 210, 4);
  tft.drawFastHLine(80, 240, 160, TFT_WHITE);
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
  tft.fillRect(0, 0, 320, 50, 0x03E0);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("COOLING CONTROL", CENTER_X, 15, 4);

  const char   *modeTxt[]    = {"OFF", "ON", "AUTO"};
  uint16_t      modeColors[] = {TFT_RED, 0x0400, 0x03E0};

  tft.fillRect(20, 80, 280, 120, modeColors[currentFanMode]);
  tft.drawRect(20, 80, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, modeColors[currentFanMode]);
  tft.drawCentreString("FAN MODE", CENTER_X, 100, 2);
  tft.drawCentreString(modeTxt[currentFanMode], CENTER_X, 130, 4);
  tft.drawCentreString("(SELECT TO CYCLE)", CENTER_X, 175, 2);

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

  bool        canProceed = (currentWeight > 10.0);
  const char *options[]  = {"PROCEED", "BYPASS (TEST)"};
  for (int i = 0; i < 2; i++) {
    int      y        = 250 + (i * 70);
    uint16_t bgColor  = TFT_DARKGREY;
    uint16_t txtColor = TFT_WHITE;
    if (i == 0)
      bgColor = canProceed ? ((wizardSelection == 0) ? 0x03E0 : 0x0400) : TFT_DARKGREY;
    else
      bgColor = (wizardSelection == 1) ? TFT_ORANGE : 0x9000;

    tft.drawRect(18, y - 2, 284, 54, (wizardSelection == i) ? TFT_BLACK : TFT_WHITE);
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
    tft.setTextPadding(0);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    char startBuf[64];
    sprintf(startBuf, "STARTED: %s", brewStartTime);
    tft.drawString(startBuf, 15, 60, 2);
    tft.drawString("SD CARD:", 15, 80, 2);
    uint16_t sdBg = sdStatus ? 0x0400 : TFT_RED;
    tft.setTextColor(TFT_WHITE, sdBg);
    tft.drawString(sdStatus ? " READY " : " ERROR ", 75, 80, 2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("LOG:", 190, 80, 2);
    char lBuf[16];
    sprintf(lBuf, " %s ", lastLogTime);
    tft.drawString(lBuf, 230, 80, 2);
    dashNeedsFullRedraw = false;
  }

  const char *titles[] = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  uint16_t    colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

  if (!moduleViewActive) {
    for (int i = 0; i < 3; i++) {
      int y = 110 + (i * 60);
      tft.drawRect(3, y - 2, 314, 54, (i == dashSelection) ? TFT_BLACK : TFT_WHITE);
      tft.fillRect(5, y, 310, 50, colors[i]);
      tft.drawRect(5, y, 310, 50, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, colors[i]);
      tft.drawCentreString(titles[i], CENTER_X, y + 15, 2);
    }
    tft.fillRect(0, 290, 320, 190, TFT_WHITE);
  } else {
    int y = 110, h = 360;
    tft.fillRect(5, y, 310, h, colors[dashSelection]);
    tft.drawRect(5, y, 310, h, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(titles[dashSelection], CENTER_X, y + 10, 4);
    tft.drawFastHLine(20, y + 45, 280, TFT_WHITE);
    if (dashSelection == 0) {
      tft.drawCentreString("AMBIENT", 80, y + 55, 2);
      tft.drawCentreString("LIQUID",  240, y + 55, 2);
      tft.drawCentreString("COOLING", 80, y + 130, 2);
      tft.drawCentreString("HEATING", 240, y + 130, 2);
      tft.drawCentreString("FAN MODE", 80, y + 210, 2);
      tft.drawCentreString("WEIGHT",  240, y + 210, 2);
    } else if (dashSelection == 1) {
      tft.drawCentreString("AMBIENT",    80,  y + 60, 2);
      tft.drawCentreString("LIQUID",     240, y + 60, 2);
      tft.drawCentreString("S. GRAVITY", 80,  y + 130, 2);
      tft.drawCentreString("PH LEVEL",   240, y + 130, 2);
      tft.drawCentreString("SIGNAL",     80,  y + 210, 2);
      tft.drawCentreString("BATTERY",    240, y + 210, 2);
      tft.drawCentreString("MIXER",      CENTER_X, y + 285, 2);
    } else if (dashSelection == 2) {
      tft.drawCentreString("PAST. TEMP",     CENTER_X, y + 60,  2);
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
  const char *options[] = {"NEW BREW", "CONTINUE BREW", "SYSTEM CHECK", "SENSOR VALUES"};
  for (int i = 0; i < 4; i++) {
    uint16_t color    = (menuSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (menuSelection == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 70), 280, 50, color);
    tft.drawRect(20, 80 + (i * 70), 280, 50, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 95 + (i * 70), 2);
  }
}

void drawSystemCheckMenu() {
  if (systemCheckNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SYSTEM CHECK", CENTER_X, 15, 4);
    systemCheckNeedsFullRedraw = false;
  }
  const char *options[] = {"FAN TEST", "LIGHT INDICATORS"};
  for (int i = 0; i < 2; i++) {
    uint16_t color    = (systemCheckSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (systemCheckSelection == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 100), 280, 80, color);
    tft.drawRect(20, 80 + (i * 100), 280, 80, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 110 + (i * 100), 4);
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO GO BACK", CENTER_X, 450, 2);
}

void drawFanTestPick() {
  if (fanTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SELECT FAN GROUP", CENTER_X, 15, 4);
    fanTestNeedsFullRedraw = false;
  }
  const char *options[] = {"PRE-HEATING FANS", "FERMENTATION FANS"};
  for (int i = 0; i < 2; i++) {
    uint16_t color    = (fanTestFanChoice == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (fanTestFanChoice == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 120), 280, 100, color);
    tft.drawRect(20, 80 + (i * 120), 280, 100, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 120 + (i * 120), 4);
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO GO BACK", CENTER_X, 450, 2);
}

void drawFanTestMenu() {
  if (fanTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("FAN CONTROL", CENTER_X, 15, 4);
    fanTestNeedsFullRedraw = false;
  }

  const char *title = (fanTestFanChoice == 0) ? "PRE-HEATING" : "FERMENTATION";
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString(title, CENTER_X, 70, 4);

  bool activeFanOn = (currentFanMode == FAN_ON);
  uint16_t statusColor = activeFanOn ? 0x0400 : TFT_RED;
  tft.fillRect(20, 120, 280, 160, statusColor);
  tft.drawRect(20, 120, 280, 160, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, statusColor);
  tft.drawCentreString(activeFanOn ? "RUNNING" : "STOPPED", CENTER_X, 170, 4);
  tft.drawCentreString("SELECT TO TOGGLE", CENTER_X, 240, 2);

  tft.fillRect(20, 300, 280, 80, 0xD6BA);
  tft.drawRect(20, 300, 280, 80, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("UP/DOWN: SPEED", CENTER_X, 315, 2);
  char buf[16];
  sprintf(buf, "%d %%", fanTestSpeed);
  tft.drawCentreString(buf, CENTER_X, 340, 4);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO CHANGE GROUP", CENTER_X, 450, 2);
}

void drawLightTestMenu() {
  if (lightTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("LIGHT INDICATORS", CENTER_X, 15, 4);
    lightTestNeedsFullRedraw = false;
  }

  const char *lightLabels[] = {"INDICATOR RED", "INDICATOR YELLOW", "INDICATOR GREEN"};
  bool        lightStates[] = {isLight1On, isLight2On, isLight3On};
  uint16_t    ledColors[]   = {TFT_RED, TFT_YELLOW, TFT_GREEN};

  for (int i = 0; i < 3; i++) {
    uint16_t color    = (lightTestSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (lightTestSelection == i) ? TFT_WHITE : TFT_BLACK;
    uint16_t ledColor = lightStates[i] ? ledColors[i] : TFT_DARKGREY;
    tft.fillRect(20, 80 + (i * 100), 280, 80, color);
    tft.drawRect(20, 80 + (i * 100), 280, 80, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawString(lightLabels[i], 40, 110 + (i * 100), 4);
    tft.fillCircle(260, 120 + (i * 100), 15, ledColor);
    tft.drawCircle(260, 120 + (i * 100), 15, TFT_BLACK);
  }

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("SELECT TO TOGGLE", CENTER_X, 420, 2);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 450, 2);
}

void drawValueTile(int x, int y, const char *label, String value, bool isError) {
  uint16_t bgColor = isError ? TFT_RED : 0x3566;
  tft.fillRect(5, y, 310, 45, bgColor);
  tft.drawRect(5, y, 310, 45, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(label, 20, y + 13, 2);
  tft.drawRightString(value, 300, y + 13, 2);
}

void drawCalibrationValueTile(int y, const char *label, String value, bool isSelected) {
  uint16_t bgColor  = isSelected ? 0x3566 : 0xD6BA;
  uint16_t txtColor = isSelected ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, y, 300, 60, bgColor);
  tft.drawRect(10, y, 300, 60, TFT_DARKGREY);
  tft.setTextColor(txtColor);
  tft.drawString(label, 25, y + 22, 2);
  tft.drawRightString(value, 285, y + 22, 2);
}

void drawCalibrationPage(bool valuesOnly) {
  char b[32];
  if (!valuesOnly) {
    if (calNeedsFullRedraw) {
      tft.fillScreen(TFT_WHITE);
      tft.fillRect(0, 0, 320, 50, 0x9000);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("SCALE CALIBRATION", CENTER_X, 15, 4);
      calNeedsFullRedraw = false;
    }
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("1. EMPTY VAT & TARE", CENTER_X, 60, 2);
    tft.drawCentreString("2. ADD KNOWN WEIGHT", CENTER_X, 80, 2);
    tft.drawCentreString("3. ADJUST FACTOR",    CENTER_X, 100, 2);
    drawCalibrationValueTile(130, "TARE SCALE", "SELECT", calSelection == 0);
    sprintf(b, "%.1f", calibrationFactor);
    drawCalibrationValueTile(200, "CAL. FACTOR", b, calSelection == 1);
    tft.fillRect(10, 280, 300, 80, 0xCE79);
    tft.drawRect(10, 280, 300, 80, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("CURRENT READING", CENTER_X, 295, 2);
    drawCalibrationValueTile(380, "SAVE & EXIT", "GO BACK", calSelection == 2);
  }
  // Always update the live reading value in place
  sprintf(b, "%.2f L", currentWeight);
  tft.setTextColor(TFT_BLACK, 0xCE79);
  tft.setTextPadding(200);
  tft.drawCentreString(b, CENTER_X, 320, 4);
  tft.setTextPadding(0);
}

void drawSensorMonitorPage(bool valuesOnly) {
  if (!valuesOnly) {
    if (monitorNeedsFullRedraw) {
      tft.fillScreen(TFT_WHITE);
      tft.fillRect(0, 0, 320, 50, TFT_NAVY);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("KEY VALUES", CENTER_X, 15, 4);
      tft.setTextColor(TFT_BLACK);
      tft.drawCentreString("SELECT: CALIBRATE", CENTER_X, 420, 2);
      tft.drawCentreString("RETURN: GO BACK",   CENTER_X, 450, 2);
      monitorNeedsFullRedraw = false;
    }
    int yStart = 60, yGap = 47;
    char b[32];
    if (bme1Status) { sprintf(b, "%.1f C", bme1.readTemperature());                    drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)",   b,        false); }
    else              drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)",   "FAILED", true);
    if (liquid1Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(0)); drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)",    b,        false); }
    else                drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)",    "FAILED", true);
    if (incomingData.sensor2Status > 0) { sprintf(b, "%.1f C", incomingData.room2Temp);      drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", b,        false); }
    else                                  drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", "FAILED", true);
    if (incomingData.ds18Status == 1) { sprintf(b, "%.1f C", incomingData.room2LiquidTemp); drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)",  b,        false); }
    else                                drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)",  "FAILED", true);
    if (liquid2Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(1)); drawValueTile(5, yStart + yGap * 4, "LIQUID (PST)",   b,        false); }
    else                drawValueTile(5, yStart + yGap * 4, "LIQUID (PST)",   "FAILED", true);
    if (incomingData.pillGravity > 0.1) { sprintf(b, "%.4f SG", incomingData.pillGravity); drawValueTile(5, yStart + yGap * 5, "S. GRAVITY",     b,        false); }
    else                                  drawValueTile(5, yStart + yGap * 5, "S. GRAVITY",     "FAILED", true);
    if (incomingData.adsStatus == 1) { sprintf(b, "%.2f pH", incomingData.phValue); drawValueTile(5, yStart + yGap * 6, "PH LEVEL",       b,        false); }
    else                               drawValueTile(5, yStart + yGap * 6, "PH LEVEL",       "FAILED", true);
    if (hx711Status) { sprintf(b, "%.1f L", currentWeight); drawValueTile(5, yStart + yGap * 7, "EST. VOLUME",    b,        false); }
    else               drawValueTile(5, yStart + yGap * 7, "EST. VOLUME",    "FAILED", true);
    sprintf(b, "R:%d L:%d U:%d D:%d S:%d HX:%d", ljRight, ljLeft, ljUp, ljDown, ljSelect, (int)hx711Status);
    drawValueTile(5, yStart + yGap * 8, "RAW DEBUG", b, false);
    return;
  }

  // Values-only: overwrite just the right-side text of each tile, no border/label redraws
  int  yStart = 60, yGap = 47;
  char b[32];
  tft.setTextPadding(160);

  auto rv = [&](int row, const char *val, bool isErr) {
    int      y  = yStart + yGap * row;
    uint16_t bg = isErr ? TFT_RED : 0x3566;
    tft.setTextColor(TFT_WHITE, bg);
    tft.drawRightString(val, 298, y + 13, 2);
  };

  if (bme1Status) { sprintf(b, "%.1f C", bme1.readTemperature());                    rv(0, b, false); } else rv(0, "FAILED", true);
  if (liquid1Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(0)); rv(1, b, false); } else rv(1, "FAILED", true);
  if (incomingData.sensor2Status > 0) { sprintf(b, "%.1f C", incomingData.room2Temp);      rv(2, b, false); } else rv(2, "FAILED", true);
  if (incomingData.ds18Status == 1) { sprintf(b, "%.1f C", incomingData.room2LiquidTemp); rv(3, b, false); } else rv(3, "FAILED", true);
  if (liquid2Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(1)); rv(4, b, false); } else rv(4, "FAILED", true);
  if (incomingData.pillGravity > 0.1) { sprintf(b, "%.4f SG", incomingData.pillGravity); rv(5, b, false); } else rv(5, "FAILED", true);
  if (incomingData.adsStatus == 1) { sprintf(b, "%.2f pH", incomingData.phValue); rv(6, b, false); } else rv(6, "FAILED", true);
  if (hx711Status) { sprintf(b, "%.1f L", currentWeight); rv(7, b, false); } else rv(7, "FAILED", true);
  sprintf(b, "R:%d L:%d U:%d D:%d S:%d HX:%d", ljRight, ljLeft, ljUp, ljDown, ljSelect, (int)hx711Status);
  rv(8, b, false);

  tft.setTextPadding(0);
}

void drawMixerMenu() {
  tft.fillScreen(TFT_WHITE);
  tft.fillRect(0, 0, 320, 50, TFT_NAVY);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("MIXER CONTROL", CENTER_X, 15, 4);

  const char   *modeTxt[]    = {"OFF", "MANUAL", "AUTO"};
  uint16_t      modeColors[] = {TFT_RED, 0x0400, 0x001F};

  tft.fillRect(20, 80, 280, 120, modeColors[currentMixerMode]);
  tft.drawRect(20, 80, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, modeColors[currentMixerMode]);
  tft.drawCentreString("MIXER MODE", CENTER_X, 100, 2);
  tft.drawCentreString(modeTxt[currentMixerMode], CENTER_X, 127, 4);
  tft.drawCentreString("(SELECT TO CYCLE)", CENTER_X, 178, 2);

  tft.fillRect(20, 220, 280, 120, 0xD6BA);
  tft.drawRect(20, 220, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("SPEED", CENTER_X, 240, 2);
  char buf[32];
  sprintf(buf, "%d%%", mixerSpeedPercent);
  tft.drawCentreString(buf, CENTER_X, 262, 4);
  if (currentMixerMode == MIXER_MANUAL)
    tft.drawCentreString("(UP/DOWN TO ADJ)", CENTER_X, 318, 2);
  else
    tft.drawCentreString("AUTO CONTROLLED", CENTER_X, 318, 2);

  if (currentMixerMode == MIXER_AUTO) {
    uint16_t statusColor = mixerRunning ? 0x0400 : 0x3566;
    tft.fillRect(20, 360, 280, 50, statusColor);
    tft.drawRect(20, 360, 280, 50, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusColor);
    tft.drawCentreString(mixerRunning ? "RUNNING" : "STANDBY", CENTER_X, 377, 2);
  }

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 440, 2);
}

void drawInitTile(int x, int y, const char *label, int status) {
  uint16_t    bgColor   = 0xD6BA;
  uint16_t    textColor = TFT_BLACK;
  const char *stTxt     = "PENDING";
  if (status == 1) { bgColor = 0x3566; textColor = TFT_WHITE; stTxt = "GOOD"; }
  else if (status == 2) { bgColor = TFT_RED; textColor = TFT_WHITE; stTxt = "NOT FOUND"; }
  tft.fillRect(5, y, 310, 45, bgColor);
  tft.drawRect(5, y, 310, 45, TFT_DARKGREY);
  tft.setTextColor(textColor);
  tft.drawString(label, 20, y + 13, 2);
  tft.drawRightString(stTxt, 300, y + 13, 2);
}
