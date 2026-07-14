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
  tft.fillRect(0, 0, 320, 50, 0x03E0);
  tft.fillRect(0, 50, 320, 430, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("COOLING CONTROL", 10, 15, 4);

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
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("NEW BREW SETUP", 10, 15, 4);
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

static void formatStageTimer(uint32_t ms, char *out) {
  uint32_t totalSec = ms / 1000;
  uint32_t hours    = totalSec / 3600;
  uint32_t days     = hours / 24;
  if (days > 0) {
    sprintf(out, "%lud %02luh", (unsigned long)days, (unsigned long)(hours % 24));
  } else {
    sprintf(out, "%02lu:%02lu:%02lu",
            (unsigned long)hours,
            (unsigned long)((totalSec % 3600) / 60),
            (unsigned long)(totalSec % 60));
  }
}

void drawDashboardLayout() {
  if (dashNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
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
    tft.fillRect(0, 106, 320, 184, TFT_WHITE);
    for (int i = 0; i < 3; i++) {
      int y = 110 + (i * 60);
      bool sel = (i == dashSelection);
      tft.fillRect(5, y, 310, 50, colors[i]);
      tft.drawRect(5, y, 310, 50, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, colors[i]);
      tft.drawCentreString(titles[i], CENTER_X, y + 15, 2);
      if (sel) {
        tft.drawString(">", 278, y + 15, 2);
      }
      if (i == activeBrewStage) {
        tft.fillCircle(22, y + 25, 5, TFT_WHITE);
      }
      {
        char timerBuf[16] = "";
        if (i == activeBrewStage) {
          formatStageTimer(millis() - stageStartMillis, timerBuf);
        } else if (stageElapsedMs[i] > 0) {
          formatStageTimer(stageElapsedMs[i], timerBuf);
        }
        tft.setTextColor(TFT_WHITE, colors[i]);
        tft.setTextPadding(130);
        tft.drawString(timerBuf, 35, y + 35, 1);
        tft.setTextPadding(0);
      }
      if (simTempOverride[i] > 0.0f) {
        char simBuf[16];
        const char *tag = simManual[i] ? "MAN" : (simDynamic[i] ? "DYN" : "SIM");
        sprintf(simBuf, "[%s %.0fC]", tag, simTempOverride[i]);
        tft.drawRightString(simBuf, 305, y + 35, 1);
      } else if ((i == 0 && preHeatSterilized && isFanOn) || (i == 2 && pastSterilized && isFanOn)) {
        tft.drawRightString("[COOLING]", 305, y + 35, 1);
      } else if (i == 0 && preHeatHolding) {
        tft.drawRightString("[HOLDING]", 305, y + 35, 1);
      } else if (i == 2 && pastHolding) {
        tft.drawRightString("[HOLDING]", 305, y + 35, 1);
      }
    }
    if (activeBrewStage == -1) {
      tft.fillRect(0, 290, 320, 190, TFT_WHITE);
      tft.fillRect(0, 292, 320, 1, TFT_DARKGREY);
      tft.setTextColor(0x0400, TFT_WHITE);
      tft.drawCentreString("BREW COMPLETE", CENTER_X, 330, 4);
      tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
      tft.drawCentreString("SELECT: View Brew Results", CENTER_X, 390, 2);
      tft.drawCentreString("LEFT: Return to Menu", CENTER_X, 412, 2);
    } else if (stageTransferring) {
      tft.fillRect(0, 290, 320, 190, TFT_WHITE);
      tft.drawFastHLine(0, 292, 320, TFT_DARKGREY);
      const char *toNames[] = {"", "FERMENTATION", "PASTEURIZATION"};
      tft.setTextColor(TFT_NAVY, TFT_WHITE);
      tft.drawCentreString("TRANSFERRING LIQUID TO", CENTER_X, 315, 2);
      tft.drawCentreString(toNames[stageTransferTarget], CENTER_X, 340, 2);
      int rem = 10 - (int)((millis() - transferStartMs) / 1000);
      if (rem < 0) rem = 0;
      char cbuf[8];
      sprintf(cbuf, "%d", rem);
      tft.setTextColor(TFT_NAVY, TFT_WHITE);
      tft.setTextPadding(160);
      tft.drawCentreString(cbuf, CENTER_X, 368, 6);
      tft.setTextPadding(0);
      tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
      tft.drawCentreString("PLEASE WAIT", CENTER_X, 458, 1);
    } else {
      tft.fillRect(0, 290, 320, 2, TFT_WHITE);
      updateDashboardGraph();
    }
  } else {
    int y = 110, h = 340;
    tft.fillRect(5, y, 310, h, colors[dashSelection]);
    tft.drawRect(5, y, 310, h, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(titles[dashSelection], CENTER_X, y + 10, 4);
    {
      char timerBuf[16] = "";
      if (dashSelection == activeBrewStage) {
        formatStageTimer(millis() - stageStartMillis, timerBuf);
      } else if (stageElapsedMs[dashSelection] > 0) {
        formatStageTimer(stageElapsedMs[dashSelection], timerBuf);
      }
      tft.setTextPadding(130);
      tft.drawString(timerBuf, 25, y + 38, 1);
      tft.setTextPadding(0);
    }
    if (simTempOverride[dashSelection] > 0.0f) {
      char simBuf[22];
      sprintf(simBuf, "[%s: %.1f C]",
              simDynamic[dashSelection] ? "DYN" : "SIM",
              simTempOverride[dashSelection]);
      tft.drawRightString(simBuf, 295, y + 38, 1);
    }
    tft.drawFastHLine(20, y + 48, 280, TFT_WHITE);
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
      tft.drawCentreString("ABV",        CENTER_X, y + 326, 2);
    } else if (dashSelection == 2) {
      tft.drawCentreString("PAST. TEMP",     CENTER_X, y + 60,  2);
      tft.drawCentreString("PROCESS STATUS", CENTER_X, y + 130, 2);
    }
    // Module view button hints
    tft.fillRect(0, y + h, 320, 30, TFT_WHITE);
    tft.fillRect(0, y + h, 320, 1, TFT_DARKGREY);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("LEFT: BACK    RIGHT: STAGE PARAMS", CENTER_X, y + h + 10, 1);
  }
  lastDashSelection = dashSelection;
}

void updateDashboardTimers() {
  if (moduleViewActive || activeBrewStage < 0) return;
  const uint16_t colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  int i = activeBrewStage;
  int y = 110 + (i * 60);
  tft.setTextColor(TFT_WHITE, colors[i]);
  char timerBuf[16] = "";
  if (stageTransferring) {
    formatStageTimer(stageElapsedMs[i], timerBuf);
  } else {
    formatStageTimer(millis() - stageStartMillis, timerBuf);
  }
  tft.setTextPadding(130);
  tft.drawString(timerBuf, 35, y + 35, 1);
  if (!stageTransferring && simTempOverride[i] > 0.0f) {
    char simBuf[20];
    const char *tag = simManual[i] ? "MAN" : (simDynamic[i] ? "DYN" : "SIM");
    sprintf(simBuf, "[%s %.0fC]", tag, simTempOverride[i]);
    tft.setTextPadding(120);
    tft.drawRightString(simBuf, 305, y + 35, 1);
  }
  tft.setTextPadding(0);
  if (stageTransferring) {
    int rem = 10 - (int)((millis() - transferStartMs) / 1000);
    if (rem < 0) rem = 0;
    char cbuf[8];
    sprintf(cbuf, "%d", rem);
    tft.setTextColor(TFT_NAVY, TFT_WHITE);
    tft.setTextPadding(160);
    tft.drawCentreString(cbuf, CENTER_X, 368, 6);
    tft.setTextPadding(0);
  }
}

void drawStartMenu() {
  if (menuNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("MAIN MENU", 10, 15, 4);
    menuNeedsFullRedraw = false;
  }
  const char *options[] = {"NEW BREW", "CONTINUE BREW", "SYSTEM CHECK", "SENSOR VALUES", "DEMO RUN"};
  for (int i = 0; i < 5; i++) {
    uint16_t color    = (menuSelection == i) ? 0x3566 : (i == 4 ? 0x0400 : 0xD6BA);
    uint16_t txtColor = (menuSelection == i) ? TFT_WHITE : (i == 4 ? TFT_WHITE : TFT_BLACK);
    tft.fillRect(20, 80 + (i * 70), 280, 50, color);
    tft.drawRect(20, 80 + (i * 70), 280, 50, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 95 + (i * 70), 2);
  }
}

void drawLoadCellPage(bool valuesOnly) {
  char b[32];

  if (!valuesOnly) {
    if (loadCellNeedsFullRedraw) {
      // Header
      tft.fillRect(0, 0, 320, 50, 0x0493);
      tft.fillRect(0, 50, 320, 430, TFT_WHITE);
      tft.setTextColor(TFT_WHITE);
      tft.drawString("LOAD CELL", 10, 15, 4);
      loadCellNeedsFullRedraw = false;
    }

    // Weight display box (static frame)
    tft.fillRect(10, 58, 300, 72, 0xCE79);
    tft.drawRect(10, 58, 300, 72, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, 0xCE79);
    tft.drawCentreString("LIVE WEIGHT", CENTER_X, 63, 2);

    // TARE row
    uint16_t tareBg  = (loadCellSelection == 0) ? 0x3566 : 0xD6BA;
    uint16_t tareTxt = (loadCellSelection == 0) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 142, 300, 65, tareBg);
    tft.drawRect(10, 142, 300, 65, TFT_DARKGREY);
    tft.setTextColor(tareTxt, tareBg);
    tft.drawString("TARE", 25, 161, 4);
    tft.drawRightString("SELECT TO ZERO", 290, 165, 2);

    // CAL FACTOR row
    uint16_t calBg  = (loadCellSelection == 1) ? 0x3566 : 0xD6BA;
    uint16_t calTxt = (loadCellSelection == 1) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 217, 300, 65, calBg);
    tft.drawRect(10, 217, 300, 65, TFT_DARKGREY);
    tft.setTextColor(calTxt, calBg);
    tft.drawString("CAL FACTOR", 25, 236, 4);
    sprintf(b, "%.2f", calibrationFactor);
    tft.drawRightString(b, 290, 240, 2);

    // HX711 status bar
    uint16_t statusBg  = hx711Status ? 0x0400 : TFT_RED;
    tft.fillRect(10, 292, 300, 45, statusBg);
    tft.drawRect(10, 292, 300, 45, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusBg);
    tft.drawCentreString(hx711Status ? "HX711  OK" : "HX711  FAILED", CENTER_X, 308, 2);

    // Navigation hints
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP / DOWN : NAVIGATE", CENTER_X, 352, 2);
    tft.drawCentreString("LEFT / RIGHT : ADJUST FACTOR  +-10", CENTER_X, 375, 2);
    tft.drawCentreString("RETURN : GO BACK", CENTER_X, 450, 2);
  }

  // Live weight value — always update
  sprintf(b, "%.2f L", currentWeight);
  tft.setTextColor(TFT_BLACK, 0xCE79);
  tft.setTextPadding(200);
  tft.drawCentreString(b, CENTER_X, 82, 4);
  
  // Show raw ADC value below the live weight for debugging
  sprintf(b, "RAW: %ld", rawHX711);
  tft.setTextPadding(150);
  tft.drawCentreString(b, CENTER_X, 108, 2);
  
  tft.setTextPadding(0);

  // Always refresh cal factor value in case it changed
  if (!valuesOnly) return;
  uint16_t calBg  = (loadCellSelection == 1) ? 0x3566 : 0xD6BA;
  uint16_t calTxt = (loadCellSelection == 1) ? TFT_WHITE : TFT_BLACK;
  sprintf(b, "%.2f", calibrationFactor);
  tft.setTextColor(calTxt, calBg);
  tft.setTextPadding(120);
  tft.drawRightString(b, 290, 240, 2);
  tft.setTextPadding(0);
}

void drawSystemCheckMenu() {
  static int prevSel = -1;

  const char *options[] = {
    "FAN TEST", "LIGHT INDICATORS", "RELAY TEST",
    "MOTOR TEST", "PID CONTROL",
    "HEATER OUTPUT", "SD CARD VERIFY", "UART MONITOR",
    "SET RTC TIME", "TRANSFER TEST"
  };

  auto drawTile = [&](int i, bool sel) {
    uint16_t color    = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 52 + (i * 34), 300, 30, color);
    tft.drawRect(10, 52 + (i * 34), 300, 30, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 59 + (i * 34), 2);
  };

  if (systemCheckNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SYSTEM CHECK", 10, 15, 4);
    for (int i = 0; i < 10; i++) drawTile(i, systemCheckSelection == i);
    tft.fillRect(0, 432, 320, 48, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: ENTER   RETURN: BACK", CENTER_X, 458, 1);
    systemCheckNeedsFullRedraw = false;
    prevSel = systemCheckSelection;
  } else if (prevSel != systemCheckSelection) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(systemCheckSelection, true);
    prevSel = systemCheckSelection;
  }
}

void drawFanTestPick() {
  static int prevSel = -1;

  const char *options[] = {"PRE-HEATING FANS", "FERMENTATION FANS"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color    = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 120), 280, 100, color);
    tft.drawRect(20, 80 + (i * 120), 280, 100, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 120 + (i * 120), 4);
  };

  if (fanTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SELECT FAN GROUP", 10, 15, 4);
    for (int i = 0; i < 2; i++) drawTile(i, fanTestFanChoice == i);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK", CENTER_X, 458, 1);
    fanTestNeedsFullRedraw = false;
    prevSel = fanTestFanChoice;
  } else if (prevSel != fanTestFanChoice) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(fanTestFanChoice, true);
    prevSel = fanTestFanChoice;
  }
}

void drawFanTestMenu() {
  if (fanTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("FAN CONTROL", 10, 15, 4);
    fanTestNeedsFullRedraw = false;
  }

  const char *title = (fanTestFanChoice == 0) ? "PRE-HEATING" : "FERMENTATION";
  tft.fillRect(0, 52, 320, 28, 0x4208);
  tft.setTextColor(TFT_WHITE, 0x4208);
  tft.drawCentreString(title, CENTER_X, 60, 2);

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

  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: SPEED   SELECT: TOGGLE FAN   RETURN: BACK", CENTER_X, 458, 1);
}

void drawLightTestMenu() {
  if (lightTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("LIGHT INDICATORS", 10, 15, 4);
    lightTestNeedsFullRedraw = false;
  }

  const char *lightLabels[] = {"RED", "YELLOW", "GREEN"};
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

  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("SELECT: TOGGLE   RETURN: BACK", CENTER_X, 458, 1);
}

void drawRelayTestMenu() {
  if (relayTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("RELAY TEST", 10, 15, 4);
    tft.fillRect(0, 52, 320, 28, 0xFFE0);
    tft.setTextColor(TFT_BLACK, 0xFFE0);
    tft.drawCentreString("! PUMPS + FANS WILL ENERGIZE !", CENTER_X, 60, 2);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("AUTO-SEQUENCING  500ms/CH", CENTER_X, 420, 2);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("RETURN: BACK", CENTER_X, 458, 1);
    relayTestNeedsFullRedraw = false;
  }

  // Channel labels and circle centers: 3 rows of 3, x=55,160,265
  const int cx[3]    = {55, 160, 265};
  const int rowY[3]  = {120, 230, 340};
  const int r        = 35;
  const char *labels[9] = {"FERM FAN","PUMP 1","PUMP 2","CH4","CH5","LIGHT G","LIGHT Y","LIGHT R","PRE FAN"};

  for (int i = 0; i < 9; i++) {
    int row = i / 3;
    int col = i % 3;
    bool active      = (relayTestChannel == i);
    uint16_t fill    = active ? TFT_GREEN : 0xC618;
    uint16_t outline = active ? 0x0300 : TFT_DARKGREY;
    tft.fillCircle(cx[col], rowY[row], r, fill);
    tft.drawCircle(cx[col], rowY[row], r, outline);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.fillRect(cx[col] - 20, rowY[row] + r + 4, 40, 14, TFT_WHITE);
    tft.drawCentreString(labels[i], cx[col], rowY[row] + r + 5, 1);
  }

  // Active channel banner
  char buf[20];
  if (relayTestChannel < 8)
    sprintf(buf, "ACTIVE: CH%d", relayTestChannel + 1);
  else
    sprintf(buf, "ACTIVE: FAN");
  tft.fillRect(0, 55, 320, 28, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString(buf, CENTER_X, 62, 2);
}

void drawMotorTestMenu() {
  if (motorTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("MOTOR TEST", 10, 15, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SPEED   L/R: DIRECTION", CENTER_X, 447, 1);
    tft.drawCentreString("SELECT: STOP   RETURN: BACK", CENTER_X, 462, 1);
    motorTestNeedsFullRedraw = false;
  }

  // Speed tile
  uint16_t speedColor = (motorTestSpeed == 0) ? TFT_DARKGREY : 0x03E0;
  tft.fillRect(20, 70, 280, 130, speedColor);
  tft.drawRect(20, 70, 280, 130, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, speedColor);
  tft.drawCentreString("SPEED", CENTER_X, 85, 2);
  char buf[16];
  sprintf(buf, "%d%%", motorTestSpeed);
  tft.drawCentreString(buf, CENTER_X, 115, 7);

  // Direction tile
  uint16_t dirColor = motorTestCW ? 0x001F : 0xF800;
  tft.fillRect(20, 220, 280, 130, dirColor);
  tft.drawRect(20, 220, 280, 130, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, dirColor);
  tft.drawCentreString("DIRECTION", CENTER_X, 235, 2);
  tft.drawCentreString(motorTestCW ? "CLOCKWISE" : "C-CLOCKWISE", CENTER_X, 270, 4);
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
      tft.fillRect(0, 0, 320, 50, 0x9000);
      tft.fillRect(0, 50, 320, 430, TFT_WHITE);
      tft.setTextColor(TFT_WHITE);
      tft.drawString("SCALE CALIBRATION", 10, 15, 4);
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
      tft.fillRect(0, 0, 320, 50, TFT_NAVY);
      tft.fillRect(0, 50, 320, 430, TFT_WHITE);
      tft.setTextColor(TFT_WHITE);
      tft.drawString("KEY VALUES", 10, 15, 4);
      tft.setTextColor(TFT_BLACK);
      tft.drawCentreString("SELECT: LOAD CELL", CENTER_X, 420, 2);
      tft.drawCentreString("RETURN: GO BACK",   CENTER_X, 450, 2);
      monitorNeedsFullRedraw = false;
    }
    int yStart = 60, yGap = 47;
    char b[32];
    if (bme1Status) { sprintf(b, "%.1f C", bme1.readTemperature());                    drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)",   b,        false); }
    else              drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)",   "FAILED", true);
    if (liquid2Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(1)); drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)",    b,        false); }
    else                drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)",    "FAILED", true);
    if (incomingData.sensor2Status > 0) { sprintf(b, "%.1f C", incomingData.room2Temp);      drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", b,        false); }
    else                                  drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", "FAILED", true);
    if (incomingData.ds18Status == 1) { sprintf(b, "%.1f C", incomingData.room2LiquidTemp); drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)",  b,        false); }
    else                                drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)",  "FAILED", true);
    if (liquid1Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(0)); drawValueTile(5, yStart + yGap * 4, "LIQUID (PST)",   b,        false); }
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
  if (liquid2Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(1)); rv(1, b, false); } else rv(1, "FAILED", true);
  if (incomingData.sensor2Status > 0) { sprintf(b, "%.1f C", incomingData.room2Temp);      rv(2, b, false); } else rv(2, "FAILED", true);
  if (incomingData.ds18Status == 1) { sprintf(b, "%.1f C", incomingData.room2LiquidTemp); rv(3, b, false); } else rv(3, "FAILED", true);
  if (liquid1Status) { sprintf(b, "%.1f C", sharedLiquidSensors.getTempCByIndex(0)); rv(4, b, false); } else rv(4, "FAILED", true);
  if (incomingData.pillGravity > 0.1) { sprintf(b, "%.4f SG", incomingData.pillGravity); rv(5, b, false); } else rv(5, "FAILED", true);
  if (incomingData.adsStatus == 1) { sprintf(b, "%.2f pH", incomingData.phValue); rv(6, b, false); } else rv(6, "FAILED", true);
  if (hx711Status) { sprintf(b, "%.1f L", currentWeight); rv(7, b, false); } else rv(7, "FAILED", true);
  sprintf(b, "R:%d L:%d U:%d D:%d S:%d HX:%d", ljRight, ljLeft, ljUp, ljDown, ljSelect, (int)hx711Status);
  rv(8, b, false);

  tft.setTextPadding(0);
}

void drawMixerMenu() {
  tft.fillRect(0, 0, 320, 50, TFT_NAVY);
  tft.fillRect(0, 50, 320, 430, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("MIXER CONTROL", 10, 15, 4);

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

void drawStageParamMenu() {
  const char    *stageNames[]  = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  char buf[40];
  bool simActive  = (simTempOverride[stageParamStage] > 0.0f);
  bool simIsDyn   = simActive && simDynamic[stageParamStage];
  bool simIsMan   = simActive && simManual[stageParamStage];

  if (stageParamNeedsFullRedraw) {
    stageParamNeedsFullRedraw = false;
  }

  // Header
  tft.fillRect(0, 0, 320, 50, stageColors[stageParamStage]);
  tft.setTextColor(TFT_WHITE, stageColors[stageParamStage]);
  tft.drawCentreString(stageNames[stageParamStage], CENTER_X, 8, 2);
  if (simIsDyn) {
    tft.drawCentreString("[SIM DYNAMIC RUNNING]", CENTER_X, 28, 2);
  } else if (simIsMan) {
    tft.drawCentreString("[SIM MANUAL - UP/DOWN = TEMP]", CENTER_X, 28, 2);
  } else if (simActive) {
    tft.drawCentreString("[SIM STATIC ACTIVE]", CENTER_X, 28, 2);
  } else if (activeBrewStage == stageParamStage && stageStartMillis > 0) {
    uint32_t elapsedSec = (millis() - stageStartMillis) / 1000;
    uint32_t d = elapsedSec / 86400;
    uint32_t h = (elapsedSec % 86400) / 3600;
    sprintf(buf, "%ld d  %ld h elapsed", (long)d, (long)h);
    tft.drawCentreString(buf, CENTER_X, 30, 1);
  } else {
    tft.drawCentreString("PARAMETERS", CENTER_X, 28, 4);
  }

  int lastRow = (stageParamStage == 1) ? 4 : 2;

  // Row 0: SIM TEMP — L/R adjusts value, SELECT toggles DYN mode
  {
    bool sel = (stageParamSelection == 0);
    uint16_t bg  = sel ? 0x3566 : (simIsDyn ? 0xD7FF : (simIsMan ? 0xCFFD : (simActive ? 0xFBE0 : 0xD6BA)));
    uint16_t bdr = simIsDyn ? 0x001F : (simIsMan ? 0x07FF : (simActive ? TFT_ORANGE : TFT_DARKGREY));
    uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 52, 300, 42, bg);
    tft.drawRect(10, 52, 300, 42, bdr);
    tft.setTextColor(fg, bg);
    tft.drawString("SIM TEMP", 20, 65, 2);
    if (simIsDyn)
      sprintf(buf, "%.1f C [DYN]", simTempOverride[stageParamStage]);
    else if (simIsMan)
      sprintf(buf, "%.1f C [MANUAL]", simTempOverride[stageParamStage]);
    else if (simActive)
      sprintf(buf, "%.1f C [STATIC]", simTempOverride[stageParamStage]);
    else
      strcpy(buf, "-- [OFF]");
    tft.drawRightString(buf, 300, 65, 2);
  }

  // Row 1: Target Temp (all stages)
  {
    bool sel = (stageParamSelection == 1);
    bool editing = sel && stageParamEditing;
    uint16_t bg  = sel ? 0x3566 : 0xD6BA;
    uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
    uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
    tft.fillRect(10, 96, 300, 42, bg);
    tft.drawRect(10, 96, 300, 42, bdr);
    tft.setTextColor(fg, bg);
    tft.drawString(editing ? "TARGET TEMP  [L/R=+-1  DOWN=DONE]" : "TARGET TEMP", 20, 109, 2);
    sprintf(buf, "%.1f C", stageTargetTemp[stageParamStage]);
    tft.drawRightString(buf, 300, 109, 2);
  }

  if (stageParamStage == 1) {
    // Row 2: Target pH
    {
      bool sel = (stageParamSelection == 2);
      bool editing = sel && stageParamEditing;
      uint16_t bg  = sel ? 0x3566 : 0xD6BA;
      uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
      uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
      tft.fillRect(10, 140, 300, 42, bg);
      tft.drawRect(10, 140, 300, 42, bdr);
      tft.setTextColor(fg, bg);
      tft.drawString(editing ? "TARGET PH  [L/R=+-0.1  DOWN=DONE]" : "TARGET PH", 20, 153, 2);
      sprintf(buf, "%.2f", fermTargetPH);
      tft.drawRightString(buf, 300, 153, 2);
    }
    // Row 3: Target Gravity
    {
      bool sel = (stageParamSelection == 3);
      bool editing = sel && stageParamEditing;
      uint16_t bg  = sel ? 0x3566 : 0xD6BA;
      uint16_t fg  = sel ? TFT_WHITE : TFT_BLACK;
      uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
      tft.fillRect(10, 184, 300, 42, bg);
      tft.drawRect(10, 184, 300, 42, bdr);
      tft.setTextColor(fg, bg);
      tft.drawString(editing ? "TARGET GRAVITY  [L/R=+-0.001  DOWN=DONE]" : "TARGET GRAVITY", 20, 197, 2);
      sprintf(buf, "%.3f", fermTargetGravity);
      tft.drawRightString(buf, 300, 197, 2);
    }
  }

  // --- Live Status section ---
  int statusY = (stageParamStage == 1) ? 228 : 140;
  tft.fillRect(0, statusY, 320, 18, 0x2124);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.drawCentreString("LIVE STATUS", CENTER_X, statusY + 3, 2);

  if (stageParamStage == 0) {
    float ambT = bme1Status ? bme1.readTemperature() : -999.0f;
    float liqT = simActive ? simTempOverride[0]
               : (liquid2Status ? sharedLiquidSensors.getTempCByIndex(1) : -999.0f);

    uint16_t ambBg = (ambT > -999 && ambT >= stageTargetTemp[0]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, ambBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, ambBg);
    tft.drawString("AMBIENT", 20, statusY + 30, 2);
    if (ambT > -999) sprintf(buf, "%.1fC  %s", ambT, ambT >= stageTargetTemp[0] ? "AT TARGET" : "BELOW");
    else strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 30, 2);

    uint16_t liqBg = (liqT > -999 && liqT >= stageTargetTemp[0]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 64, 300, 40, liqBg);
    tft.drawRect(10, statusY + 64, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, liqBg);
    tft.drawString(simActive ? "LIQUID [SIM]" : "LIQUID", 20, statusY + 74, 2);
    if (liqT > -999) sprintf(buf, "%.1fC  %s", liqT, liqT >= stageTargetTemp[0] ? "AT TARGET" : "BELOW");
    else strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 74, 2);

  } else if (stageParamStage == 1) {
    float ctrlT = simActive ? simTempOverride[1]
                : ((incomingData.ds18Status == 1) ? incomingData.room2LiquidTemp : -999.0f);
    float ph    = (incomingData.adsStatus == 1) ? incomingData.phValue : -999.0f;
    float grav  = (incomingData.pillGravity > 0.1f && incomingData.pillGravity < 10.0f)
                    ? incomingData.pillGravity : -999.0f;

    uint16_t tBg = (ctrlT > -999 && ctrlT >= stageTargetTemp[1]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, tBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, tBg);
    tft.drawString(simActive ? "LIQUID [SIM]" : "LIQUID TEMP", 20, statusY + 30, 2);
    if (ctrlT > -999) sprintf(buf, "%.1fC  %s", ctrlT, ctrlT >= stageTargetTemp[1] ? "OK" : "BELOW");
    else strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 30, 2);

    uint16_t phBg = (ph > -999 && ph <= fermTargetPH) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 64, 300, 40, phBg);
    tft.drawRect(10, statusY + 64, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, phBg);
    tft.drawString("PH", 20, statusY + 74, 2);
    if (ph > -999) sprintf(buf, "%.2f  %s", ph, ph <= fermTargetPH ? "OK" : "ABOVE TARGET");
    else strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 74, 2);

    uint16_t gBg = (grav > -999 && grav <= fermTargetGravity) ? 0x0400 : 0x001F;
    tft.fillRect(10, statusY + 108, 300, 40, gBg);
    tft.drawRect(10, statusY + 108, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, gBg);
    tft.drawString("GRAVITY", 20, statusY + 118, 2);
    if (grav > -999) sprintf(buf, "%.3f  %s", grav, grav <= fermTargetGravity ? "AT TARGET" : "FERMENTING");
    else strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 118, 2);

  } else {
    float pastT = simActive ? simTempOverride[2]
                : (liquid1Status ? sharedLiquidSensors.getTempCByIndex(0) : -999.0f);
    uint16_t pBg = (pastT > -999 && pastT >= stageTargetTemp[2]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, pBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, pBg);
    tft.drawString(simActive ? "PAST. [SIM]" : "PAST. TEMP", 20, statusY + 30, 2);
    if (pastT > -999) sprintf(buf, "%.1fC  %s", pastT, pastT >= stageTargetTemp[2] ? "AT TARGET" : "BELOW");
    else strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 30, 2);
  }

  // --- Action button ---
  const char *btnLabels[] = {
    "ADVANCE TO FERMENTATION",
    "ADVANCE TO PASTEURIZATION",
    "MARK BREW COMPLETE"
  };
  int btnY = (stageParamStage == 1) ? 382 : (stageParamStage == 2 ? 212 : 250);
  bool btnSel  = (stageParamSelection == lastRow);
  bool isActive = (activeBrewStage == stageParamStage);
  uint16_t btnBg = btnSel ? 0x3566 : (isActive ? TFT_NAVY : TFT_DARKGREY);
  tft.fillRect(10, btnY, 300, 48, btnBg);
  tft.drawRect(10, btnY, 300, 48, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnBg);
  tft.drawCentreString(btnLabels[stageParamStage], CENTER_X, btnY + 10, 2);
  tft.drawCentreString(isActive ? "SELECT TO CONFIRM" : "NOT CURRENT STAGE", CENTER_X, btnY + 30, 1);

  // --- Hints ---
  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString(stageParamEditing ? "EDITING: L/R=ADJUST   DOWN/SELECT=DONE" : "UP/DOWN: NAVIGATE   SELECT=EDIT   LEFT=BACK", CENTER_X, 447, 1);
  if (simIsMan)
    tft.drawCentreString("MANUAL: UP/DOWN ON DASHBOARD ADJUSTS TEMP", CENTER_X, 462, 1);
  else
    tft.drawCentreString("SIM: L/R=VALUE  SELECT=CYCLE(OFF/SIM/DYN/MAN)", CENTER_X, 462, 1);
}

void drawPidTestPick() {
  static int prevSel = -1;

  const char *options[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color    = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 110), 280, 80, color);
    tft.drawRect(20, 80 + (i * 110), 280, 80, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 105 + (i * 110), 4);
  };

  if (pidTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SELECT CHAMBER", 10, 15, 4);
    for (int i = 0; i < 3; i++) drawTile(i, pidTestChoice == i);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK", CENTER_X, 458, 1);
    pidTestNeedsFullRedraw = false;
    prevSel = pidTestChoice;
  } else if (prevSel != pidTestChoice) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(pidTestChoice, true);
    prevSel = pidTestChoice;
  }
}

void drawPidTestMenu() {
  if (pidTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("PID CONTROL TEST", 10, 15, 4);
    pidTestNeedsFullRedraw = false;
  }

  const char *chamberTitle;
  if (pidTestChoice == 0) chamberTitle = "PRE-HEAT";
  else if (pidTestChoice == 1) chamberTitle = "FERMENTATION";
  else chamberTitle = "PASTEURIZATION";
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString(chamberTitle, CENTER_X, 60, 4);

  // Target Heat Setting
  uint16_t heatBg = (!pidTestRunning && pidTestTargetSelection == 0) ? TFT_YELLOW : 0xD6BA;
  tft.fillRect(20, 100, 135, 62, heatBg);
  tft.drawRect(20, 100, 135, 62, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, heatBg);
  tft.drawCentreString("HEAT TARGET", 87, 108, 1);
  char buf[32];
  sprintf(buf, "%.1f C", pidTestHeatTarget);
  tft.drawCentreString(buf, 87, 118, 4);

  // Target Cool Setting
  uint16_t coolBg = (!pidTestRunning && pidTestTargetSelection == 1) ? TFT_YELLOW : 0xD6BA;
  tft.fillRect(165, 100, 135, 62, coolBg);
  tft.drawRect(165, 100, 135, 62, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, coolBg);
  tft.drawCentreString("COOL TARGET", 232, 108, 1);
  sprintf(buf, "%.1f C", pidTestCoolTarget);
  tft.drawCentreString(buf, 232, 118, 4);

  // Sensor selector (pre-heat + ferm only, y=165-177)
  if (pidTestChoice == 0 || pidTestChoice == 1) {
    uint16_t sensBg = (!pidTestRunning && pidTestTargetSelection == 2) ? TFT_YELLOW : 0xD6BA;
    tft.fillRect(20, 165, 280, 13, sensBg);
    tft.drawRect(20, 165, 280, 13, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, sensBg);
    int sel = (pidTestChoice == 0) ? pidPreHeatSensor : pidFermSensor;
    tft.drawCentreString(sel == 0 ? "SENSOR: LIQUID (DS18B20)" : "SENSOR: AMBIENT (BME280)", CENTER_X, 167, 1);
  } else {
    tft.fillRect(20, 165, 280, 13, TFT_WHITE);
  }

  // Start / Stop Button
  uint16_t btnColor = pidTestRunning ? 0x0400 : 0xF800;
  tft.fillRect(20, 180, 280, 50, btnColor);
  tft.drawRect(20, 180, 280, 50, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnColor);
  tft.drawCentreString(pidTestRunning ? "STOP TEST" : "START TEST", CENTER_X, 195, 4);

  // Live Data Dashboard (drawn only if running)
  if (pidTestRunning) {
    float liquidTemp = -999.0f;
    float ambientTemp = -999.0f;
    if (pidTestChoice == 0) {
      if (liquid2Status) liquidTemp = sharedLiquidSensors.getTempCByIndex(1);
      if (bme1Status) ambientTemp = bme1.readTemperature();
    } else if (pidTestChoice == 1) {
      if (incomingData.ds18Status == 1) liquidTemp = incomingData.room2LiquidTemp;
      if (incomingData.sensor2Status == 1) ambientTemp = incomingData.room2Temp;
    } else if (pidTestChoice == 2) {
      if (liquid1Status) liquidTemp = sharedLiquidSensors.getTempCByIndex(0);
    }

    // Row 1: temp panels (y=240, h=55)
    if (pidTestChoice == 2) {
      // Pasteurization: single full-width liquid temp panel
      tft.fillRect(10, 240, 300, 55, 0x2124);
      tft.drawRect(10, 240, 300, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("LIQUID TEMP", CENTER_X, 250, 1);
      if (liquidTemp > -100.0f) sprintf(buf, "%.1f C", liquidTemp);
      else strcpy(buf, "---");
      tft.setTextPadding(280);
      tft.drawCentreString(buf, CENTER_X, 264, 2);
      tft.setTextPadding(0);
    } else {
      // Pre-heat and ferm: LIQUID TEMP | AMBIENT TEMP
      tft.fillRect(10, 240, 145, 55, 0x2124);
      tft.drawRect(10, 240, 145, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("LIQUID TEMP", 82, 250, 1);
      if (liquidTemp > -100.0f) sprintf(buf, "%.1f C", liquidTemp);
      else strcpy(buf, "---");
      tft.setTextPadding(130);
      tft.drawCentreString(buf, 82, 264, 2);

      tft.fillRect(165, 240, 145, 55, 0x2124);
      tft.drawRect(165, 240, 145, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("AMBIENT TEMP", 237, 250, 1);
      if (ambientTemp > -100.0f) sprintf(buf, "%.1f C", ambientTemp);
      else strcpy(buf, "---");
      tft.setTextPadding(130);
      tft.drawCentreString(buf, 237, 264, 2);
      tft.setTextPadding(0);
    }

    // Row 2: HEATER % | FAN % (y=302, h=50)
    bool heaterOn = currentHeatingPercent > 0;
    bool fanOn    = isFanOn || isFermFanOn;
    uint16_t htrBg = heaterOn ? 0x0400 : 0xD6BA;
    uint16_t htrFg = heaterOn ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 302, 145, 50, htrBg);
    tft.drawRect(10, 302, 145, 50, TFT_DARKGREY);
    tft.setTextColor(htrFg, htrBg);
    tft.drawCentreString("HEATER", 82, 312, 1);
    if (heaterOn) sprintf(buf, "%d%%", currentHeatingPercent);
    else strcpy(buf, "OFF");
    tft.setTextPadding(130);
    tft.drawCentreString(buf, 82, 322, 2);

    uint16_t fanBg = fanOn ? 0x001F : 0xD6BA;
    uint16_t fanFg = fanOn ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(165, 302, 145, 50, fanBg);
    tft.drawRect(165, 302, 145, 50, TFT_DARKGREY);
    tft.setTextColor(fanFg, fanBg);
    tft.drawCentreString("FAN", 237, 312, 1);
    if (fanOn) sprintf(buf, "%d%%", pidFanPercent);
    else strcpy(buf, "OFF");
    tft.setTextPadding(130);
    tft.drawCentreString(buf, 237, 322, 2);
    tft.setTextPadding(0);

    // Row 3: Status banner (y=360, h=48)
    uint16_t statusBg;
    const char *statusText;
    if (pidTestSuccess)    { statusBg = 0x0400; statusText = "STABLE!"; }
    else if (heaterOn)     { statusBg = 0xF800; statusText = "HEATING"; }
    else if (fanOn)        { statusBg = 0x001F; statusText = "COOLING"; }
    else                   { statusBg = TFT_ORANGE; statusText = "AUTOMATING..."; }
    tft.fillRect(10, 360, 300, 48, statusBg);
    tft.drawRect(10, 360, 300, 48, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusBg);
    tft.drawCentreString(statusText, CENTER_X, 376, 4);

    // Elapsed time
    char timeStr[32];
    uint32_t elapsedSecs = (millis() - pidTestStartMs) / 1000;
    sprintf(timeStr, "TIME: %02d:%02d", elapsedSecs / 60, elapsedSecs % 60);
    tft.fillRect(10, 414, 300, 16, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString(timeStr, CENTER_X, 416, 1);
  } else {
    tft.fillRect(10, 240, 300, 196, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("SELECT START TO BEGIN", CENTER_X, 320, 2);
  }

  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DN: ADJ TEMP  |  SEL: START/STOP", CENTER_X, 447, 1);
  tft.drawCentreString("RETURN: BACK", CENTER_X, 462, 1);
}

void drawHeaterTestPick() {
  static int prevSel = -1;
  const char *options[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color    = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 82 + (i * 105), 280, 88, color);
    tft.drawRect(20, 82 + (i * 105), 280, 88, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 112 + (i * 105), 4);
  };

  if (heaterTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SELECT CHAMBER", CENTER_X, 15, 4);
    for (int i = 0; i < 3; i++) drawTile(i, heaterTestStage == i);
    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK", CENTER_X, 458, 1);
    heaterTestNeedsFullRedraw = false;
    prevSel = heaterTestStage;
  } else if (prevSel != heaterTestStage) {
    if (prevSel >= 0) drawTile(prevSel, false);
    drawTile(heaterTestStage, true);
    prevSel = heaterTestStage;
  }
}

void drawHeaterTestMenu() {
  const char *stageNames[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION"};
  const char *ssrNames[]   = {"SSR PREHEAT", "SSR FERM", "SSR PAST"};
  char buf[40];

  bool dutySel  = (heaterTestSelection == 0);
  bool startSel = (heaterTestSelection == 1);

  if (heaterTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("HEATER OUTPUT TEST", CENTER_X, 15, 4);
    tft.fillRect(0, 52, 320, 28, 0x4208);
    tft.setTextColor(TFT_WHITE, 0x4208);
    char sbuf[48];
    sprintf(sbuf, "%s  (%s)", stageNames[heaterTestStage], ssrNames[heaterTestStage]);
    tft.drawCentreString(sbuf, CENTER_X, 60, 2);
    heaterTestNeedsFullRedraw = false;
  }

  // Duty cycle tile
  uint16_t dutyBg = dutySel ? 0x3566 : 0xD6BA;
  uint16_t dutyFg = dutySel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 86, 300, 106, dutyBg);
  if (dutySel && heaterTestEditing) {
    tft.drawRect(10, 86, 300, 106, TFT_WHITE);
    tft.drawRect(11, 87, 298, 104, TFT_WHITE);
  } else {
    tft.drawRect(10, 86, 300, 106, TFT_DARKGREY);
  }
  tft.setTextColor(dutyFg, dutyBg);
  tft.drawString("DUTY CYCLE", 20, 96, 2);
  sprintf(buf, "%d%%", heaterTestPercent);
  tft.drawCentreString(buf, CENTER_X, 114, 4);
  tft.fillRect(20, 164, 260, 14, dutyBg);
  if (dutySel) {
    tft.setTextColor(TFT_WHITE, dutyBg);
    tft.drawCentreString(heaterTestEditing ? "UP/DN TO ADJUST   SELECT: DONE" : "SELECT TO EDIT", CENTER_X, 166, 1);
  }

  // START/STOP tile
  uint16_t statusBg = heaterTestRunning ? 0x0400 : (startSel ? 0xF800 : 0x4208);
  tft.fillRect(10, 198, 300, 106, statusBg);
  if (startSel) {
    tft.drawRect(10, 198, 300, 106, TFT_WHITE);
    tft.drawRect(11, 199, 298, 104, TFT_WHITE);
  } else {
    tft.drawRect(10, 198, 300, 106, TFT_DARKGREY);
  }
  tft.setTextColor(TFT_WHITE, statusBg);
  tft.drawCentreString(heaterTestRunning ? "RUNNING" : "STOPPED", CENTER_X, 222, 4);
  tft.drawCentreString(heaterTestRunning ? "SELECT TO STOP" : "SELECT TO START", CENTER_X, 268, 2);

  // Safety note
  tft.fillRect(0, 310, 320, 26, 0xFFE0);
  tft.setTextColor(TFT_BLACK, 0xFFE0);
  tft.drawCentreString("! MONITOR TEMPERATURE DURING TEST !", CENTER_X, 320, 1);

  // Footer
  tft.fillRect(0, 434, 320, 46, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (heaterTestEditing)
    tft.drawCentreString("UP/DN: ADJUST DUTY %   SELECT: DONE", CENTER_X, 447, 1);
  else
    tft.drawCentreString("UP/DN: NAVIGATE   SELECT: EDIT / START", CENTER_X, 447, 1);
  tft.drawCentreString("RETURN: BACK", CENTER_X, 462, 1);
}

void drawSdVerifyMenu() {
  char buf[40];

  if (sdVerifyNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SD CARD VERIFY", 10, 15, 4);
    sdVerifyNeedsFullRedraw = false;
  }

  // Mount status
  uint16_t mBg = sdStatus ? 0x0400 : 0xF800;
  tft.fillRect(10, 60, 300, 55, mBg);
  tft.drawRect(10, 60, 300, 55, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, mBg);
  tft.drawString("MOUNT STATUS", 20, 70, 2);
  tft.drawRightString(sdStatus ? "OK" : "FAILED", 300, 70, 2);
  tft.drawCentreString(sdStatus ? "Card detected and mounted" : "No card or mount error", CENTER_X, 94, 1);

  // Write/read result
  uint16_t vrBg = (sdVerifyResult == 1) ? 0x0400 : (sdVerifyResult == 0 ? 0xF800 : 0xD6BA);
  const char *vrTxt = (sdVerifyResult == 1) ? "WRITE / READ: PASS"
                    : (sdVerifyResult == 0)  ? "WRITE / READ: FAIL"
                                             : "WRITE / READ: NOT TESTED";
  tft.fillRect(10, 123, 300, 55, vrBg);
  tft.drawRect(10, 123, 300, 55, TFT_DARKGREY);
  tft.setTextColor((sdVerifyResult < 0) ? TFT_BLACK : TFT_WHITE, vrBg);
  tft.drawCentreString(vrTxt, CENTER_X, 143, 2);

  // Action button
  uint16_t btnBg = sdStatus ? 0x3566 : TFT_DARKGREY;
  tft.fillRect(10, 196, 300, 60, btnBg);
  tft.drawRect(10, 196, 300, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnBg);
  tft.drawCentreString("RUN VERIFY TEST", CENTER_X, 212, 4);
  tft.drawCentreString(sdStatus ? "SELECT TO RUN" : "SD NOT AVAILABLE", CENTER_X, 242, 1);

  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("SELECT: RUN TEST   RETURN: BACK", CENTER_X, 455, 2);
}

void drawUartMonitorMenu() {
  char buf[52];

  if (uartMonitorNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("UART MONITOR", 10, 15, 4);
    uartMonitorNeedsFullRedraw = false;
  }

  uint32_t age = (lastDataReceivedMillis > 0) ? (millis() - lastDataReceivedMillis) / 1000 : 9999;
  bool linkOk  = (age < 5);

  // Link status
  uint16_t lBg = linkOk ? 0x0400 : 0xF800;
  tft.fillRect(10, 58, 300, 55, lBg);
  tft.drawRect(10, 58, 300, 55, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, lBg);
  tft.drawString("LINK", 20, 68, 2);
  tft.drawRightString(linkOk ? "OK" : "TIMEOUT", 300, 68, 2);
  sprintf(buf, "Last packet: %lus ago", (unsigned long)min(age, (uint32_t)9999));
  tft.drawCentreString(buf, CENTER_X, 94, 1);

  // Packet count
  tft.fillRect(10, 121, 300, 45, 0xD6BA);
  tft.drawRect(10, 121, 300, 45, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawString("PACKETS RX", 20, 131, 2);
  sprintf(buf, "%lu", (unsigned long)uartPacketCount);
  tft.drawRightString(buf, 300, 131, 2);

  // Errors
  uint16_t eBg = (uartChecksumErrors > 0) ? 0xFBE0 : 0xD6BA;
  tft.fillRect(10, 174, 300, 45, eBg);
  tft.drawRect(10, 174, 300, 45, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, eBg);
  tft.drawString("CHECKSUM ERRORS", 20, 184, 2);
  sprintf(buf, "%lu", (unsigned long)uartChecksumErrors);
  tft.drawRightString(buf, 300, 184, 2);

  // Live data panel
  tft.fillRect(10, 227, 300, 180, 0x2124);
  tft.drawRect(10, 227, 300, 180, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.drawCentreString("LIVE DATA", CENTER_X, 235, 2);
  sprintf(buf, "Liquid: %.1fC   pH: %.2f", incomingData.room2LiquidTemp, incomingData.phValue);
  tft.drawCentreString(buf, CENTER_X, 258, 1);
  sprintf(buf, "Ambient: %.1fC  %.0fhPa", incomingData.room2Temp, incomingData.room2Pres);
  tft.drawCentreString(buf, CENTER_X, 274, 1);
  sprintf(buf, "Gravity: %.3f  Pill: %.1fC", incomingData.pillGravity, incomingData.pillTemp);
  tft.drawCentreString(buf, CENTER_X, 290, 1);
  sprintf(buf, "BLE: %s   ADS: %s   DS18: %s",
    incomingData.bleStatus  ? "OK" : "--",
    incomingData.adsStatus  ? "OK" : "--",
    incomingData.ds18Status ? "OK" : "--");
  tft.drawCentreString(buf, CENTER_X, 306, 1);
  sprintf(buf, "Battery: %d%%  RSSI: %d", incomingData.pillBattery, incomingData.pillRSSI);
  tft.drawCentreString(buf, CENTER_X, 322, 1);

  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("LIVE - UPDATES EVERY 1s   RETURN: BACK", CENTER_X, 455, 1);
}

void drawRtcSetMenu() {
  char buf[32];

  if (rtcSetNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SET RTC TIME", 10, 15, 4);
    rtcSetNeedsFullRedraw = false;
  }

  // Current RTC reading
  tft.fillRect(0, 54, 320, 24, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (rtcStatus) {
    DateTime now = rtc.now();
    sprintf(buf, "CURRENT: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
  } else {
    strcpy(buf, "RTC: NOT DETECTED");
  }
  tft.drawCentreString(buf, CENTER_X, 60, 2);

  // Hour field
  uint16_t hBg = (rtcSetField == 0) ? 0x3566 : 0xD6BA;
  uint16_t hFg = (rtcSetField == 0) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(15, 88, 130, 110, hBg);
  tft.drawRect(15, 88, 130, 110, TFT_DARKGREY);
  tft.setTextColor(hFg, hBg);
  tft.drawCentreString("HOUR", 80, 98, 2);
  sprintf(buf, "%02d", rtcSetHour);
  tft.drawCentreString(buf, 80, 118, 7);

  // Colon separator
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillRect(147, 120, 26, 60, TFT_WHITE);
  tft.drawCentreString(":", CENTER_X, 122, 7);

  // Minute field
  uint16_t mBg = (rtcSetField == 1) ? 0x3566 : 0xD6BA;
  uint16_t mFg = (rtcSetField == 1) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(175, 88, 130, 110, mBg);
  tft.drawRect(175, 88, 130, 110, TFT_DARKGREY);
  tft.setTextColor(mFg, mBg);
  tft.drawCentreString("MIN", 240, 98, 2);
  sprintf(buf, "%02d", rtcSetMinute);
  tft.drawCentreString(buf, 240, 118, 7);

  // Save button
  uint16_t saveBg = rtcStatus ? 0x3566 : TFT_DARKGREY;
  tft.fillRect(10, 216, 300, 60, saveBg);
  tft.drawRect(10, 216, 300, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, saveBg);
  tft.drawCentreString("SELECT TO SAVE", CENTER_X, 232, 4);
  tft.drawCentreString(rtcStatus ? "Writes to DS3231 chip" : "RTC not available", CENTER_X, 262, 1);

  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: FIELD   L/R: ADJUST VALUE", CENTER_X, 447, 1);
  tft.drawCentreString("SELECT: SAVE   RETURN: CANCEL", CENTER_X, 462, 1);
}

void updateDashboardGraph() {
  if (currentAppState != DASHBOARD_ACTIVE || moduleViewActive || activeBrewStage < 0 || stageTransferring) return;

  const int GX  = 28;
  const int GPY = 308;
  const int GW  = TEMP_GRAPH_W;
  const int GH  = 152;

  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  uint16_t lineColor = stageColors[activeBrewStage];

  float yMin, yMax;
  float tickTemps[5];
  if (activeBrewStage == 1) {
    yMin = 20.0f; yMax = 40.0f;
    tickTemps[0] = 20; tickTemps[1] = 25; tickTemps[2] = 30;
    tickTemps[3] = 35; tickTemps[4] = 40;
  } else {
    yMin = 0.0f; yMax = 100.0f;
    tickTemps[0] = 0;  tickTemps[1] = 25; tickTemps[2] = 50;
    tickTemps[3] = 75; tickTemps[4] = 100;
  }

  auto tempToY = [&](float t) -> int {
    int py = GPY + GH - 1 - (int)(((t) - yMin) / (yMax - yMin) * (GH - 1) + 0.5f);
    if (py < GPY)       py = GPY;
    if (py > GPY+GH-1)  py = GPY + GH - 1;
    return py;
  };

  // Separator line
  tft.drawFastHLine(0, 292, 320, TFT_DARKGREY);

  // Header row
  tft.fillRect(0, 293, 320, 15, TFT_WHITE);
  const char *stageName[] = {"PRE-HEAT", "FERMENT", "PASTEUR."};
  tft.setTextColor(lineColor, TFT_WHITE);
  tft.drawString(stageName[activeBrewStage], 31, 295, 1);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawRightString("TEMP (C)", 315, 295, 1);

  // Y-axis label strip
  tft.fillRect(0, GPY, GX, GH, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.setTextPadding(0);
  char lbuf[8];
  for (int i = 0; i < 5; i++) {
    int ly = tempToY(tickTemps[i]);
    sprintf(lbuf, "%3.0f", tickTemps[i]);
    tft.drawRightString(lbuf, GX - 2, ly - 4, 1);
  }

  // Plot area background
  tft.fillRect(GX, GPY, GW, GH, TFT_WHITE);

  // Gridlines
  for (int i = 0; i < 5; i++) {
    int ly = tempToY(tickTemps[i]);
    tft.drawFastHLine(GX, ly, GW, TFT_LIGHTGREY);
  }

  // Target dashed lines
  if (activeBrewStage == 0 || activeBrewStage == 2) {
    int ty = tempToY(80.0f);
    for (int x = GX; x < GX + GW - 1; x += 4)
      tft.drawFastHLine(x, ty, 2, TFT_YELLOW);
  } else {
    int hy = tempToY(27.0f);
    int fy = tempToY(30.0f);
    for (int x = GX; x < GX + GW - 1; x += 4) {
      tft.drawFastHLine(x, hy, 2, TFT_YELLOW);
      tft.drawFastHLine(x, fy, 2, TFT_CYAN);
    }
  }

  // Border
  tft.drawRect(GX, GPY, GW, GH, TFT_DARKGREY);

  // Data line
  int n = tempHistoryCount;
  if (n >= 2) {
    int displayCount = (n < GW) ? n : GW;
    int startDataIdx = (n > GW) ? n - GW : 0;
    int startPixelX  = GX;
    for (int i = 1; i < displayCount; i++) {
      int x0 = startPixelX + i - 1;
      int x1 = startPixelX + i;
      int y0 = tempToY(tempHistory[startDataIdx + i - 1]);
      int y1 = tempToY(tempHistory[startDataIdx + i]);
      tft.drawLine(x0, y0, x1, y1, lineColor);
    }
  }

  // Footer: current temp + sample count
  tft.fillRect(0, GPY + GH, 320, 480 - (GPY + GH), TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (n > 0) {
    char fbuf[40];
    sprintf(fbuf, "%.1f C  |  %ds", tempHistory[n - 1], n);
    tft.drawCentreString(fbuf, CENTER_X, GPY + GH + 8, 1);
  } else {
    tft.drawCentreString("Collecting data...", CENTER_X, GPY + GH + 8, 1);
  }
}

void drawBrewSummaryMenu() {
  tft.fillRect(0, 0, 320, 50, 0x0400);
  tft.fillRect(0, 50, 320, 430, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, 0x0400);
  tft.drawString("BREW RESULTS", 10, 15, 4);

  char buf[32];

  // Total time
  uint32_t totalMs = stageElapsedMs[0] + stageElapsedMs[1] + stageElapsedMs[2];
  char totBuf[20];
  formatStageTimer(totalMs, totBuf);
  tft.fillRect(10, 60, 300, 36, 0xD6BA);
  tft.drawRect(10, 60, 300, 36, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawString("TOTAL TIME:", 18, 70, 2);
  tft.drawRightString(totBuf, 302, 70, 2);

  // Per-stage breakdown
  char s0[16], s1[16], s2[16];
  formatStageTimer(stageElapsedMs[0], s0);
  formatStageTimer(stageElapsedMs[1], s1);
  formatStageTimer(stageElapsedMs[2], s2);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawString("Pre-heat:",       22, 103, 2);  tft.drawRightString(s0, 302, 103, 2);
  tft.drawString("Fermentation:",   22, 123, 2);  tft.drawRightString(s1, 302, 123, 2);
  tft.drawString("Pasteurization:", 22, 143, 2);  tft.drawRightString(s2, 302, 143, 2);

  tft.drawFastHLine(10, 168, 300, TFT_DARKGREY);

  // SG + ABV
  char sgBuf[16], abvBuf[16];
  if (remoteStatusReceived && incomingData.bleStatus) {
    sprintf(sgBuf, "%.3f", incomingData.pillGravity);
    if (originalGravity > 0.0f) {
      float abv = (originalGravity - incomingData.pillGravity) * 131.25f;
      if (abv < 0.0f) abv = 0.0f;
      sprintf(abvBuf, "%.1f%%", abv);
    } else {
      strcpy(abvBuf, "--");
    }
  } else {
    strcpy(sgBuf, "--");
    strcpy(abvBuf, "--");
  }

  tft.fillRect(10, 176, 145, 70, 0xD6BA);
  tft.drawRect(10, 176, 145, 70, TFT_DARKGREY);
  tft.setTextColor(TFT_DARKGREY, 0xD6BA);
  tft.drawCentreString("FINAL SG", 82, 183, 2);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString(sgBuf, 82, 205, 4);

  tft.fillRect(165, 176, 145, 70, 0xD6BA);
  tft.drawRect(165, 176, 145, 70, TFT_DARKGREY);
  tft.setTextColor(TFT_DARKGREY, 0xD6BA);
  tft.drawCentreString("EST. ABV", 237, 183, 2);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString(abvBuf, 237, 205, 4);

  tft.drawFastHLine(10, 252, 300, TFT_DARKGREY);

  // pH + Weight
  char phBuf[16], wtBuf[16];
  if (remoteStatusReceived && incomingData.adsStatus)
    sprintf(phBuf, "%.2f", incomingData.phValue);
  else
    strcpy(phBuf, "--");
  if (hx711Status)
    sprintf(wtBuf, "%.0f g", currentWeight);
  else
    strcpy(wtBuf, "--");

  tft.fillRect(10, 260, 145, 70, 0xD6BA);
  tft.drawRect(10, 260, 145, 70, TFT_DARKGREY);
  tft.setTextColor(TFT_DARKGREY, 0xD6BA);
  tft.drawCentreString("FINAL pH", 82, 267, 2);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString(phBuf, 82, 289, 4);

  tft.fillRect(165, 260, 145, 70, 0xD6BA);
  tft.drawRect(165, 260, 145, 70, TFT_DARKGREY);
  tft.setTextColor(TFT_DARKGREY, 0xD6BA);
  tft.drawCentreString("WEIGHT", 237, 267, 2);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString(wtBuf, 237, 289, 4);

  // Brew start time if available
  if (brewStartTime[0] != '\0') {
    tft.drawFastHLine(10, 338, 300, TFT_DARKGREY);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("STARTED", CENTER_X, 347, 2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString(brewStartTime, CENTER_X, 367, 4);
  }

  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("LEFT / SELECT: BACK", CENTER_X, 455, 2);
  brewSummaryNeedsFullRedraw = false;
}

void drawTransferTestMenu(bool valuesOnly) {
  char buf[48];
  uint32_t p1 = flowPulse1;
  uint32_t p2 = flowPulse2;
  float liters1 = (flowKFactor[0] > 0.0f) ? (float)p1 / flowKFactor[0] : 0.0f;
  float liters2 = (flowKFactor[1] > 0.0f) ? (float)p2 / flowKFactor[1] : 0.0f;

  // selection 0/1 = panel 1 (pump / cal), selection 2/3 = panel 2 (pump / cal)
  bool panel1Active = (transferTestSelection <= 1);
  bool panel2Active = (transferTestSelection >= 2);

  if (valuesOnly) {
    tft.fillRect(161, 93, 138, 56, 0xD6BA);
    tft.setTextColor(TFT_BLACK, 0xD6BA);
    tft.drawCentreString("FLOW", 230, 102, 2);
    tft.setTextPadding(130);
    sprintf(buf, "%.2f L", liters1);
    tft.drawCentreString(buf, 230, 118, 4);

    tft.fillRect(161, 290, 138, 56, 0xD6BA);
    tft.setTextColor(TFT_BLACK, 0xD6BA);
    tft.drawCentreString("FLOW", 230, 299, 2);
    sprintf(buf, "%.2f L", liters2);
    tft.drawCentreString(buf, 230, 315, 4);
    tft.setTextPadding(0);
    return;
  }

  if (transferTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("TRANSFER TEST", 10, 15, 4);
    transferTestNeedsFullRedraw = false;
  }

  // ---- Panel 1: Pre-Heat → Ferm ----
  uint16_t hdr1Bg  = panel1Active ? 0x03E0 : 0xD6BA;
  uint16_t hdr1Fg  = panel1Active ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 58, 300, 28, hdr1Bg);
  tft.setTextColor(hdr1Fg, hdr1Bg);
  tft.drawCentreString("PRE-HEAT  ->  FERM", CENTER_X, 67, 2);
  tft.fillRect(10, 86, 300, 155, TFT_WHITE);

  // pump box — selection highlight, circle indicator for ON/OFF state
  bool pump1Sel = (transferTestSelection == 0);
  uint16_t pump1Bg = pump1Sel ? 0x3566 : 0xD6BA;
  uint16_t pump1Fg = pump1Sel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 92, 130, 60, pump1Bg);
  tft.drawRect(20, 92, 130, 60, TFT_DARKGREY);
  tft.setTextColor(pump1Fg, pump1Bg);
  tft.drawString("PUMP", 32, 100, 2);
  tft.fillCircle(135, 107, 10, pumpPreHeatFermOn ? 0x0400 : TFT_DARKGREY);
  tft.drawCircle(135, 107, 10, TFT_BLACK);
  tft.drawCentreString(pumpPreHeatFermOn ? "ON" : "OFF", 70, 130, 2);

  // flow box — light beige, static
  tft.fillRect(160, 92, 140, 60, 0xD6BA);
  tft.drawRect(160, 92, 140, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("FLOW", 230, 102, 2);
  tft.setTextPadding(130);
  sprintf(buf, "%.2f L", liters1);
  tft.drawCentreString(buf, 230, 118, 4);
  tft.setTextPadding(0);

  // calibrate button
  bool cal1Sel = (transferTestSelection == 1);
  uint16_t cal1Bg = cal1Sel ? 0x3566 : 0xD6BA;
  uint16_t cal1Fg = cal1Sel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 158, 280, 52, cal1Bg);
  tft.setTextColor(cal1Fg, cal1Bg);
  tft.drawCentreString("CALIBRATE FLOW SENSOR", CENTER_X, 178, 2);
  if (cal1Sel) {
    tft.drawRect(20, 158, 280, 52, TFT_WHITE);
    tft.drawRect(21, 159, 278, 50, TFT_WHITE);
    tft.drawRect(22, 160, 276, 48, TFT_WHITE);
  } else {
    tft.drawRect(20, 158, 280, 52, TFT_DARKGREY);
  }

  // panel 1 outer border
  tft.drawRect(10, 58, 300, 185, TFT_DARKGREY);

  // ---- Panel 2: Ferm → Past ----
  uint16_t hdr2Bg  = panel2Active ? 0x03E0 : 0xD6BA;
  uint16_t hdr2Fg  = panel2Active ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 251, 300, 28, hdr2Bg);
  tft.setTextColor(hdr2Fg, hdr2Bg);
  tft.drawCentreString("FERM  ->  PAST", CENTER_X, 260, 2);
  tft.fillRect(10, 279, 300, 155, TFT_WHITE);

  // pump box — selection highlight, circle indicator for ON/OFF state
  bool pump2Sel = (transferTestSelection == 2);
  uint16_t pump2Bg = pump2Sel ? 0x3566 : 0xD6BA;
  uint16_t pump2Fg = pump2Sel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 285, 130, 60, pump2Bg);
  tft.drawRect(20, 285, 130, 60, TFT_DARKGREY);
  tft.setTextColor(pump2Fg, pump2Bg);
  tft.drawString("PUMP", 32, 293, 2);
  tft.fillCircle(135, 300, 10, pumpFermPastOn ? 0x0400 : TFT_DARKGREY);
  tft.drawCircle(135, 300, 10, TFT_BLACK);
  tft.drawCentreString(pumpFermPastOn ? "ON" : "OFF", 70, 323, 2);

  // flow box — light beige, static
  tft.fillRect(160, 285, 140, 60, 0xD6BA);
  tft.drawRect(160, 285, 140, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("FLOW", 230, 295, 2);
  tft.setTextPadding(130);
  sprintf(buf, "%.2f L", liters2);
  tft.drawCentreString(buf, 230, 311, 4);
  tft.setTextPadding(0);

  // calibrate button
  bool cal2Sel = (transferTestSelection == 3);
  uint16_t cal2Bg = cal2Sel ? 0x3566 : 0xD6BA;
  uint16_t cal2Fg = cal2Sel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 351, 280, 52, cal2Bg);
  tft.setTextColor(cal2Fg, cal2Bg);
  tft.drawCentreString("CALIBRATE FLOW SENSOR", CENTER_X, 371, 2);
  if (cal2Sel) {
    tft.drawRect(20, 351, 280, 52, TFT_WHITE);
    tft.drawRect(21, 352, 278, 50, TFT_WHITE);
    tft.drawRect(22, 353, 276, 48, TFT_WHITE);
  } else {
    tft.drawRect(20, 351, 280, 52, TFT_DARKGREY);
  }

  // panel 2 outer border
  tft.drawRect(10, 251, 300, 185, TFT_DARKGREY);

  // footer
  tft.fillRect(0, 443, 320, 37, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE   RETURN: BACK", CENTER_X, 458, 1);
}

void drawFlowCalMenu(bool valuesOnly) {
  char buf[48];
  uint32_t pulses = (flowCalSensor == 0) ? flowPulse1 : flowPulse2;
  float liters = (flowKFactor[flowCalSensor] > 0.0f)
                   ? (float)pulses / flowKFactor[flowCalSensor]
                   : 0.0f;

  if (valuesOnly) {
    tft.fillRect(10, 154, 300, 80, 0x2124);
    tft.setTextColor(TFT_WHITE, 0x2124);
    tft.setTextPadding(290);
    sprintf(buf, "%lu pulses", (unsigned long)pulses);
    tft.drawCentreString(buf, CENTER_X, 162, 2);
    sprintf(buf, "%.3f L", liters);
    tft.drawCentreString(buf, CENTER_X, 182, 4);
    tft.setTextPadding(0);
    tft.drawRect(10, 154, 300, 80, TFT_DARKGREY);
    return;
  }

  if (flowCalNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("FLOW CALIBRATION", 10, 15, 4);
    flowCalNeedsFullRedraw = false;
  }

  // Sensor name bar
  tft.fillRect(0, 52, 320, 28, 0x2124);
  tft.setTextColor(TFT_WHITE, 0x2124);
  const char *sName = (flowCalSensor == 0) ? "SENSOR 1: PRE-HEAT -> FERM"
                                           : "SENSOR 2: FERM -> PAST";
  tft.drawCentreString(sName, CENTER_X, 60, 2);

  // Row 0: Known Volume
  uint16_t knownBg = (flowCalSelection == 0) ? 0x3566 : 0x2124;
  tft.fillRect(10, 84, 300, 60, knownBg);
  tft.setTextColor(TFT_WHITE, knownBg);
  if (flowCalEditing)
    tft.drawCentreString("KNOWN VOLUME  (UP/DOWN TO ADJUST)", CENTER_X, 93, 1);
  else
    tft.drawCentreString("KNOWN VOLUME  (SELECT TO EDIT)", CENTER_X, 93, 1);
  sprintf(buf, "%.1f L", flowCalKnownVolume);
  tft.drawCentreString(buf, CENTER_X, 108, 4);
  if (flowCalSelection == 0) {
    tft.drawRect(10, 84, 300, 60, TFT_WHITE);
    tft.drawRect(11, 85, 298, 58, TFT_WHITE);
  } else {
    tft.drawRect(10, 84, 300, 60, TFT_DARKGREY);
  }

  // Live data box (pulse count + calculated liters)
  tft.fillRect(10, 154, 300, 80, 0x2124);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.setTextPadding(290);
  sprintf(buf, "%lu pulses", (unsigned long)pulses);
  tft.drawCentreString(buf, CENTER_X, 162, 2);
  sprintf(buf, "%.3f L", liters);
  tft.drawCentreString(buf, CENTER_X, 182, 4);
  tft.setTextPadding(0);
  tft.drawRect(10, 154, 300, 80, TFT_DARKGREY);

  // K-factor readout
  tft.fillRect(10, 238, 300, 32, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  sprintf(buf, "K-FACTOR: %.1f pulses/L", flowKFactor[flowCalSensor]);
  tft.drawCentreString(buf, CENTER_X, 248, 2);

  // Row 1: RESET
  uint16_t resetBg = (flowCalSelection == 1) ? 0x8000 : 0x4208;
  tft.fillRect(10, 276, 300, 50, resetBg);
  tft.setTextColor(TFT_WHITE, resetBg);
  tft.drawCentreString("RESET PULSE COUNT", CENTER_X, 295, 2);
  tft.drawRect(10, 276, 300, 50, (flowCalSelection == 1) ? TFT_WHITE : TFT_DARKGREY);

  // Row 2: CAPTURE
  uint16_t capBg = (flowCalSelection == 2) ? 0x0400 : 0x2124;
  tft.fillRect(10, 334, 300, 84, capBg);
  tft.setTextColor(TFT_WHITE, capBg);
  tft.drawCentreString("CAPTURE K-FACTOR", CENTER_X, 354, 2);
  tft.setTextColor(TFT_DARKGREY, capBg);
  tft.drawCentreString("flow known vol, then SELECT", CENTER_X, 374, 1);
  tft.drawRect(10, 334, 300, 84, (flowCalSelection == 2) ? TFT_WHITE : TFT_DARKGREY);

  // Footer
  tft.fillRect(0, 432, 320, 48, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (flowCalEditing) {
    tft.drawCentreString("UP/DOWN: ADJUST VOLUME", CENTER_X, 446, 1);
    tft.drawCentreString("SELECT / RETURN: DONE EDITING", CENTER_X, 462, 1);
  } else {
    tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE", CENTER_X, 446, 1);
    tft.drawCentreString("RETURN: BACK TO TRANSFER TEST", CENTER_X, 462, 1);
  }
}

