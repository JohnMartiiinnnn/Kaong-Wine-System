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


void drawCoolingMenu() {
  tft.fillRect(0, 0, 320, 50, 0x03E0);
  tft.fillRect(0, 50, 320, 430, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("COOLING CONTROL", 10, 15, 4);

  const char *modeTxt[] = {"OFF", "ON", "AUTO"};
  uint16_t modeColors[] = {TFT_RED, 0x0400, 0x03E0};

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
    wizardNeedsFullRedraw = false;
  }

  // Current weight box
  tft.fillRect(20, 60, 280, 75, 0xD6BA);
  tft.drawRect(20, 60, 280, 75, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("CURRENT WEIGHT", CENTER_X, 70, 2);
  char buf[64];
  if (hx711Status)
    sprintf(buf, "%.1f L", currentWeight);
  else
    strcpy(buf, "FAILED");
  tft.drawCentreString(buf, CENTER_X, 95, 4);

  // Row 0: Min Volume
  uint16_t row0Bg =
      (wizardSelection == 0) ? (wizardEditing ? 0x03E0 : 0x3566) : 0xCE79;
  uint16_t row0Fg = (wizardSelection == 0) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 150, 280, 45, row0Bg);
  tft.drawRect(20, 150, 280, 45, TFT_DARKGREY);
  tft.setTextColor(row0Fg, row0Bg);
  if (wizardSelection == 0 && wizardEditing) {
    sprintf(buf, "MIN VOLUME: [ %.1f L ]", minVolumeReq);
  } else {
    sprintf(buf, "MIN VOLUME: < %.1f L >", minVolumeReq);
  }
  tft.drawCentreString(buf, CENTER_X, 162, 2);

  // Row 1: Preheat Heater
  uint16_t row1Bg = (wizardSelection == 1) ? 0x3566 : 0xCE79;
  uint16_t row1Fg = (wizardSelection == 1) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(20, 205, 280, 45, row1Bg);
  tft.drawRect(20, 205, 280, 45, TFT_DARKGREY);
  tft.setTextColor(row1Fg, row1Bg);
  sprintf(buf, "PREHEAT HEATER: %s",
          skipPreheatHeater ? "DISABLED" : "ENABLED");
  tft.drawCentreString(buf, CENTER_X, 217, 2);

  // Row 2: PROCEED
  bool canProceed = (currentWeight >= minVolumeReq);
  uint16_t row2Bg =
      canProceed ? ((wizardSelection == 2) ? 0x03E0 : 0x0400) : TFT_DARKGREY;
  uint16_t row2Fg = TFT_WHITE;
  tft.drawRect(18, 268, 284, 49,
               (wizardSelection == 2) ? TFT_BLACK : TFT_WHITE);
  tft.fillRect(20, 270, 280, 45, row2Bg);
  tft.drawRect(20, 270, 280, 45, TFT_DARKGREY);
  tft.setTextColor(row2Fg, row2Bg);
  tft.drawCentreString("PROCEED (START BREW)", CENTER_X, 282, 2);

  // Row 3: BYPASS (TEST RUN)
  uint16_t row3Bg = (wizardSelection == 3) ? TFT_ORANGE : 0x9000;
  uint16_t row3Fg = TFT_WHITE;
  tft.drawRect(18, 328, 284, 49,
               (wizardSelection == 3) ? TFT_BLACK : TFT_WHITE);
  tft.fillRect(20, 330, 280, 45, row3Bg);
  tft.drawRect(20, 330, 280, 45, TFT_DARKGREY);
  tft.setTextColor(row3Fg, row3Bg);
  tft.drawCentreString("BYPASS (TEST RUN)", CENTER_X, 342, 2);

  // Help Footer
  tft.fillRect(0, 420, 320, 60, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: NAV   LEFT/RIGHT: ADJUST   SELECT: RUN",
                       CENTER_X, 432, 1);
  tft.drawCentreString("RETURN (LEFT on Row 2/3): CANCEL", CENTER_X, 452, 1);
}

static void formatStageTimer(uint32_t ms, char *out) {
  uint32_t totalSec = ms / 1000;
  uint32_t hours = totalSec / 3600;
  uint32_t days = hours / 24;
  if (days > 0) {
    sprintf(out, "%lud %02luh", (unsigned long)days,
            (unsigned long)(hours % 24));
  } else {
    sprintf(out, "%02lu:%02lu:%02lu", (unsigned long)hours,
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
  uint16_t colors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

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
        const char *tag =
            simManual[i] ? "MAN" : (simDynamic[i] ? "DYN" : "SIM");
        sprintf(simBuf, "[%s %.0fC]", tag, simTempOverride[i]);
        tft.drawRightString(simBuf, 305, y + 35, 1);
      } else if ((i == 0 && preHeatSterilized && isFanOn) ||
                 (i == 2 && pastSterilized && isFanOn)) {
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
      if (rem < 0)
        rem = 0;
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
      sprintf(simBuf, "[%s: %.1f C]", simDynamic[dashSelection] ? "DYN" : "SIM",
              simTempOverride[dashSelection]);
      tft.drawRightString(simBuf, 295, y + 38, 1);
    }
    tft.drawFastHLine(20, y + 48, 280, TFT_WHITE);
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
      tft.drawCentreString("MIXER", CENTER_X, y + 285, 2);
      tft.drawCentreString("ABV", CENTER_X, y + 326, 2);
    } else if (dashSelection == 2) {
      tft.drawCentreString("PAST. TEMP", CENTER_X, y + 60, 2);
      tft.drawCentreString("PROCESS STATUS", CENTER_X, y + 130, 2);
    }
    // Module view button hints
    tft.fillRect(0, y + h, 320, 30, TFT_WHITE);
    tft.fillRect(0, y + h, 320, 1, TFT_DARKGREY);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("LEFT: BACK    RIGHT: STAGE PARAMS", CENTER_X,
                         y + h + 10, 1);
  }
  lastDashSelection = dashSelection;
}

void updateDashboardTimers() {
  if (moduleViewActive || activeBrewStage < 0)
    return;
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
    if (rem < 0)
      rem = 0;
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
  const char *options[] = {"NEW BREW", "CONTINUE BREW", "SYSTEM CHECK",
                           "SENSOR VALUES"};
  for (int i = 0; i < 4; i++) {
    uint16_t color = (menuSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (menuSelection == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 90 + (i * 80), 280, 55, color);
    tft.drawRect(20, 90 + (i * 80), 280, 55, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 108 + (i * 80), 2);
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
    tft.fillRect(10, 70, 300, 95, 0xCE79);
    tft.drawRect(10, 70, 300, 95, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, 0xCE79);
    tft.drawCentreString("LIVE WEIGHT", CENTER_X, 78, 2);

    // TARE button
    uint16_t tareBg = 0x3566; // Blue (always selected since it's the only option)
    uint16_t tareTxt = TFT_WHITE;
    tft.fillRect(10, 195, 300, 80, tareBg);
    tft.drawRect(10, 195, 300, 80, TFT_DARKGREY);
    tft.setTextColor(tareTxt, tareBg);
    tft.drawString("TARE", 25, 221, 4);
    tft.drawRightString("SELECT TO ZERO", 290, 227, 2);

    // HX711 status bar
    uint16_t statusBg = hx711Status ? 0x0400 : TFT_RED;
    tft.fillRect(10, 305, 300, 45, statusBg);
    tft.drawRect(10, 305, 300, 45, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusBg);
    tft.drawCentreString(hx711Status ? "HX711  OK" : "HX711  FAILED", CENTER_X,
                         320, 2);

    // Navigation hints
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("SELECT : TARE SCALE", CENTER_X, 375, 2);
    tft.drawCentreString("RETURN : GO BACK", CENTER_X, 450, 2);
  }

  // Live weight value — always update
  sprintf(b, "%.2f L", currentWeight);
  tft.setTextColor(TFT_BLACK, 0xCE79);
  tft.setTextPadding(200);
  tft.drawCentreString(b, CENTER_X, 100, 4);

  // Show raw ADC value below the live weight for debugging
  sprintf(b, "RAW: %ld", rawHX711);
  tft.setTextPadding(150);
  tft.drawCentreString(b, CENTER_X, 134, 2);

  tft.setTextPadding(0);
}

void drawSystemCheckMenu() {
  static int prevSel = -1;

  const char *options[] = {
      "FAN TEST",     "LIGHT INDICATORS", "RELAY TEST",      "MOTOR TEST",
      "PID CONTROL",  "HEATER OUTPUT",    "SD CARD VERIFY",  "UART MONITOR",
      "SET RTC TIME", "TRANSFER TEST",    "LOAD CELL",       "RAPT PILL",
      "PH & FERM TEMP"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 52 + (i * 27), 300, 24, color);
    tft.drawRect(10, 52 + (i * 27), 300, 24, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 56 + (i * 27), 2);
  };

  if (systemCheckNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SYSTEM CHECK", 10, 15, 4);
    for (int i = 0; i < 13; i++)
      drawTile(i, systemCheckSelection == i);
    tft.fillRect(0, 432, 320, 48, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: ENTER   RETURN: BACK",
                         CENTER_X, 458, 1);
    systemCheckNeedsFullRedraw = false;
    prevSel = systemCheckSelection;
  } else if (prevSel != systemCheckSelection) {
    if (prevSel >= 0)
      drawTile(prevSel, false);
    drawTile(systemCheckSelection, true);
    prevSel = systemCheckSelection;
  }
}

void drawFanTestPick() {
  static int prevSel = -1;

  const char *options[] = {"PRE-HEATING FANS", "FERMENTATION FANS"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color = sel ? 0x3566 : 0xD6BA;
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
    for (int i = 0; i < 2; i++)
      drawTile(i, fanTestFanChoice == i);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK",
                         CENTER_X, 458, 1);
    fanTestNeedsFullRedraw = false;
    prevSel = fanTestFanChoice;
  } else if (prevSel != fanTestFanChoice) {
    if (prevSel >= 0)
      drawTile(prevSel, false);
    drawTile(fanTestFanChoice, true);
    prevSel = fanTestFanChoice;
  }
}

void drawRelayTestPick() {
  static int prevSel = -1;

  const char *options[] = {"AUTOMATIC SEQUENCING", "MANUAL CONTROL"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 100 + (i * 140), 280, 100, color);
    tft.drawRect(20, 100 + (i * 140), 280, 100, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 140 + (i * 140), 4);
  };

  if (relayTestPickNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SELECT RELAY MODE", 10, 15, 4);
    for (int i = 0; i < 2; i++)
      drawTile(i, relayTestPickSelection == i);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK",
                         CENTER_X, 458, 1);
    relayTestPickNeedsFullRedraw = false;
    prevSel = relayTestPickSelection;
  } else if (prevSel != relayTestPickSelection) {
    if (prevSel >= 0)
      drawTile(prevSel, false);
    drawTile(relayTestPickSelection, true);
    prevSel = relayTestPickSelection;
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
  tft.drawCentreString("UP/DOWN: SPEED   SELECT: TOGGLE FAN   RETURN: BACK",
                       CENTER_X, 458, 1);
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
  bool lightStates[] = {isLight1On, isLight2On, isLight3On};
  uint16_t ledColors[] = {TFT_RED, TFT_YELLOW, TFT_GREEN};

  for (int i = 0; i < 3; i++) {
    uint16_t color = (lightTestSelection == i) ? 0x3566 : 0xD6BA;
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
    tft.drawCentreString("RETURN: BACK", CENTER_X, 458, 1);
    relayTestNeedsFullRedraw = false;
  }

  // Draw footer text dynamically based on mode
  tft.fillRect(0, 415, 320, 25, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (relayTestAuto) {
    tft.drawCentreString("AUTO-SEQUENCING  1000ms/CH", CENTER_X, 420, 2);
  } else {
    tft.drawCentreString("UP/DOWN: NAV   SELECT: TOGGLE", CENTER_X, 420, 2);
  }

  const char *labels[9] = {"FERM FAN", "PUMP 1",  "PUMP 2",  "CH4",    "CH5",
                           "LIGHT G",  "LIGHT Y", "LIGHT R", "PRE FAN"};

  // Draw Relays (indices 0 to 8)
  for (int i = 0; i < 9; i++) {
    int y = 85 + (i * 36);
    bool selected = (relayTestSelection == i);
    uint16_t color = selected ? 0x3566 : 0xD6BA;
    uint16_t txtColor = selected ? TFT_WHITE : TFT_BLACK;
    uint16_t ledColor = testRelayStates[i] ? TFT_GREEN : TFT_DARKGREY;

    tft.fillRect(15, y, 290, 32, color);
    tft.drawRect(15, y, 290, 32, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawString(labels[i], 30, y + 8, 2);
    tft.fillCircle(280, y + 16, 8, ledColor);
    tft.drawCircle(280, y + 16, 8, TFT_BLACK);
  }
}

void drawMotorTestMenu() {
  if (motorTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("MOTOR TEST", 10, 15, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SPEED   SELECT: ON/OFF", CENTER_X, 447, 1);
    tft.drawCentreString("RETURN: BACK", CENTER_X, 462, 1);
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

  char telemetryBuf[64];
  float rpmVal = 0.0f;
  float currentA = incomingData.motorSenseVolts * (3.0f / 0.353f);
  if (motorTestSpeed > 0 && incomingData.motorSenseVolts > 0.01f) {
    rpmVal = 66.0f - 160.19f * (incomingData.motorSenseVolts - 0.035f);
    if (rpmVal < 0.0f) rpmVal = 0.0f;
    if (rpmVal > 66.0f) rpmVal = 66.0f;
  }
  sprintf(telemetryBuf, "%.2fA  RPM: %.1f", currentA, rpmVal);
  tft.drawCentreString(telemetryBuf, CENTER_X, 168, 2);

  // ON/OFF tile
  uint16_t onOffColor = motorTestOn ? 0x0400 : 0x4208;
  tft.fillRect(20, 220, 280, 130, onOffColor);
  tft.drawRect(20, 220, 280, 130, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, onOffColor);
  tft.drawCentreString("MOTOR", CENTER_X, 235, 2);
  tft.drawCentreString(motorTestOn ? "ON" : "OFF", CENTER_X, 270, 4);
}

void drawValueTile(int x, int y, const char *label, String value,
                   bool isError) {
  uint16_t bgColor = isError ? TFT_RED : 0x3566;
  tft.fillRect(5, y, 310, 45, bgColor);
  tft.drawRect(5, y, 310, 45, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(label, 20, y + 13, 2);
  tft.drawRightString(value, 300, y + 13, 2);
}

void drawCalibrationValueTile(int y, const char *label, String value,
                              bool isSelected) {
  uint16_t bgColor = isSelected ? (wizardEditing ? 0x03E0 : 0x3566) : 0xD6BA;
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
    tft.drawCentreString("3. ADJUST FACTOR", CENTER_X, 100, 2);
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
      tft.drawCentreString("RETURN: GO BACK", CENTER_X, 450, 2);
      monitorNeedsFullRedraw = false;
    }
    int yStart = 60, yGap = 47;
    char b[32];
    if (bme1Status) {
      sprintf(b, "%.1f C", bme1.readTemperature());
      drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)", b, false);
    } else
      drawValueTile(5, yStart + yGap * 0, "AMBIENT (PH)", "FAILED", true);
    if (liquid2Status) {
      sprintf(b, "%.1f C", getPreheatTemp());
      drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)", b, false);
    } else
      drawValueTile(5, yStart + yGap * 1, "LIQUID (PH)", "FAILED", true);
    if (incomingData.sensor2Status > 0) {
      sprintf(b, "%.1f C", incomingData.room2Temp);
      drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", b, false);
    } else
      drawValueTile(5, yStart + yGap * 2, "AMBIENT (FERM)", "FAILED", true);
    if (incomingData.ds18Status == 1) {
      sprintf(b, "%.1f C", getFermTemp());
      drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)", b, false);
    } else
      drawValueTile(5, yStart + yGap * 3, "LIQUID (FERM)", "FAILED", true);
    if (liquid1Status) {
      sprintf(b, "%.1f C", getPastTemp());
      drawValueTile(5, yStart + yGap * 4, "LIQUID (PST)", b, false);
    } else
      drawValueTile(5, yStart + yGap * 4, "LIQUID (PST)", "FAILED", true);
    if (incomingData.pillGravity > 0.1) {
      sprintf(b, "%.4f SG", incomingData.pillGravity);
      drawValueTile(5, yStart + yGap * 5, "S. GRAVITY", b, false);
    } else
      drawValueTile(5, yStart + yGap * 5, "S. GRAVITY", "FAILED", true);
    if (incomingData.adsStatus == 1) {
      sprintf(b, "%.2f pH", incomingData.phValue);
      drawValueTile(5, yStart + yGap * 6, "PH LEVEL", b, false);
    } else
      drawValueTile(5, yStart + yGap * 6, "PH LEVEL", "FAILED", true);
    if (hx711Status) {
      sprintf(b, "%.1f L", currentWeight);
      drawValueTile(5, yStart + yGap * 7, "EST. VOLUME", b, false);
    } else
      drawValueTile(5, yStart + yGap * 7, "EST. VOLUME", "FAILED", true);
    sprintf(b, "R:%d L:%d U:%d D:%d S:%d HX:%d", ljRight, ljLeft, ljUp, ljDown,
            ljSelect, (int)hx711Status);
    drawValueTile(5, yStart + yGap * 8, "RAW DEBUG", b, false);
    return;
  }

  // Values-only: overwrite just the right-side text of each tile, no
  // border/label redraws
  int yStart = 60, yGap = 47;
  char b[32];
  tft.setTextPadding(160);

  auto rv = [&](int row, const char *val, bool isErr) {
    int y = yStart + yGap * row;
    uint16_t bg = isErr ? TFT_RED : 0x3566;
    tft.setTextColor(TFT_WHITE, bg);
    tft.drawRightString(val, 298, y + 13, 2);
  };

  if (bme1Status) {
    sprintf(b, "%.1f C", bme1.readTemperature());
    rv(0, b, false);
  } else
    rv(0, "FAILED", true);
  if (liquid2Status) {
    sprintf(b, "%.1f C", getPreheatTemp());
    rv(1, b, false);
  } else
    rv(1, "FAILED", true);
  if (incomingData.sensor2Status > 0) {
    sprintf(b, "%.1f C", incomingData.room2Temp);
    rv(2, b, false);
  } else
    rv(2, "FAILED", true);
  if (incomingData.ds18Status == 1) {
    sprintf(b, "%.1f C", getFermTemp());
    rv(3, b, false);
  } else
    rv(3, "FAILED", true);
  if (liquid1Status) {
    sprintf(b, "%.1f C", getPastTemp());
    rv(4, b, false);
  } else
    rv(4, "FAILED", true);
  if (incomingData.pillGravity > 0.1) {
    sprintf(b, "%.4f SG", incomingData.pillGravity);
    rv(5, b, false);
  } else
    rv(5, "FAILED", true);
  if (incomingData.adsStatus == 1) {
    sprintf(b, "%.2f pH", incomingData.phValue);
    rv(6, b, false);
  } else
    rv(6, "FAILED", true);
  if (hx711Status) {
    sprintf(b, "%.1f L", currentWeight);
    rv(7, b, false);
  } else
    rv(7, "FAILED", true);
  sprintf(b, "R:%d L:%d U:%d D:%d S:%d HX:%d", ljRight, ljLeft, ljUp, ljDown,
          ljSelect, (int)hx711Status);
  rv(8, b, false);

  tft.setTextPadding(0);
}

void drawMixerMenu() {
  tft.fillRect(0, 0, 320, 50, TFT_NAVY);
  tft.fillRect(0, 50, 320, 430, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("MIXER CONTROL", 10, 15, 4);

  const char *modeTxt[] = {"OFF", "MANUAL", "AUTO"};
  uint16_t modeColors[] = {TFT_RED, 0x0400, 0x001F};

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

  char telemetryBuf[64];
  float rpmVal = 0.0f;
  if (mixerSpeedPercent > 0 && incomingData.motorSenseVolts > 0.01f) {
    rpmVal = 66.0f - 160.19f * (incomingData.motorSenseVolts - 0.035f);
    if (rpmVal < 0.0f) rpmVal = 0.0f;
    if (rpmVal > 66.0f) rpmVal = 66.0f;
  }
  sprintf(telemetryBuf, "V: %.3fV  RPM: %.1f", incomingData.motorSenseVolts, rpmVal);
  tft.drawCentreString(telemetryBuf, CENTER_X, 292, 2);

  if (currentMixerMode == MIXER_MANUAL)
    tft.drawCentreString("(UP/DOWN TO ADJ)", CENTER_X, 318, 1);
  else
    tft.drawCentreString("AUTO CONTROLLED", CENTER_X, 318, 1);

  if (currentMixerMode == MIXER_AUTO) {
    uint16_t statusColor = mixerRunning ? 0x0400 : 0x3566;
    tft.fillRect(20, 360, 280, 50, statusColor);
    tft.drawRect(20, 360, 280, 50, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusColor);
    tft.drawCentreString(mixerRunning ? "RUNNING" : "STANDBY", CENTER_X, 377,
                         2);
  }

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 440, 2);
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

void drawStageParamMenu() {
  const char *stageNames[] = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  char buf[40];
  bool simActive = (simTempOverride[stageParamStage] > 0.0f);
  bool simIsDyn = simActive && simDynamic[stageParamStage];
  bool simIsMan = simActive && simManual[stageParamStage];

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
    uint16_t bg =
        sel ? 0x3566
            : (simIsDyn ? 0xD7FF
                        : (simIsMan ? 0xCFFD : (simActive ? 0xFBE0 : 0xD6BA)));
    uint16_t bdr =
        simIsDyn
            ? 0x001F
            : (simIsMan ? 0x07FF : (simActive ? TFT_ORANGE : TFT_DARKGREY));
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
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
    uint16_t bg = sel ? 0x3566 : 0xD6BA;
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
    uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
    tft.fillRect(10, 96, 300, 42, bg);
    tft.drawRect(10, 96, 300, 42, bdr);
    tft.setTextColor(fg, bg);
    tft.drawString(editing ? "TARGET TEMP  [L/R=+-1  DOWN=DONE]"
                           : "TARGET TEMP",
                   20, 109, 2);
    sprintf(buf, "%.1f C", stageTargetTemp[stageParamStage]);
    tft.drawRightString(buf, 300, 109, 2);
  }

  if (stageParamStage == 1) {
    // Row 2: Target pH
    {
      bool sel = (stageParamSelection == 2);
      bool editing = sel && stageParamEditing;
      uint16_t bg = sel ? 0x3566 : 0xD6BA;
      uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
      uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
      tft.fillRect(10, 140, 300, 42, bg);
      tft.drawRect(10, 140, 300, 42, bdr);
      tft.setTextColor(fg, bg);
      tft.drawString(editing ? "TARGET PH  [L/R=+-0.1  DOWN=DONE]"
                             : "TARGET PH",
                     20, 153, 2);
      sprintf(buf, "%.2f", fermTargetPH);
      tft.drawRightString(buf, 300, 153, 2);
    }
    // Row 3: Target Gravity
    {
      bool sel = (stageParamSelection == 3);
      bool editing = sel && stageParamEditing;
      uint16_t bg = sel ? 0x3566 : 0xD6BA;
      uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
      uint16_t bdr = editing ? TFT_YELLOW : TFT_DARKGREY;
      tft.fillRect(10, 184, 300, 42, bg);
      tft.drawRect(10, 184, 300, 42, bdr);
      tft.setTextColor(fg, bg);
      tft.drawString(editing ? "TARGET GRAVITY  [L/R=+-0.001  DOWN=DONE]"
                             : "TARGET GRAVITY",
                     20, 197, 2);
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
                           : (liquid2Status ? getPreheatTemp() : -999.0f);

    uint16_t ambBg =
        (ambT > -999 && ambT >= stageTargetTemp[0]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, ambBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, ambBg);
    tft.drawString("AMBIENT", 20, statusY + 30, 2);
    if (ambT > -999)
      sprintf(buf, "%.1fC  %s", ambT,
              ambT >= stageTargetTemp[0] ? "AT TARGET" : "BELOW");
    else
      strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 30, 2);

    uint16_t liqBg =
        (liqT > -999 && liqT >= stageTargetTemp[0]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 64, 300, 40, liqBg);
    tft.drawRect(10, statusY + 64, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, liqBg);
    tft.drawString(simActive ? "LIQUID [SIM]" : "LIQUID", 20, statusY + 74, 2);
    if (liqT > -999)
      sprintf(buf, "%.1fC  %s", liqT,
              liqT >= stageTargetTemp[0] ? "AT TARGET" : "BELOW");
    else
      strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 74, 2);

  } else if (stageParamStage == 1) {
    float ctrlT =
        simActive ? simTempOverride[1]
                  : ((incomingData.ds18Status == 1) ? getFermTemp() : -999.0f);
    float ph = (incomingData.adsStatus == 1) ? incomingData.phValue : -999.0f;
    float grav =
        (incomingData.pillGravity > 0.1f && incomingData.pillGravity < 10.0f)
            ? incomingData.pillGravity
            : -999.0f;

    uint16_t tBg =
        (ctrlT > -999 && ctrlT >= stageTargetTemp[1]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, tBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, tBg);
    tft.drawString(simActive ? "LIQUID [SIM]" : "LIQUID TEMP", 20, statusY + 30,
                   2);
    if (ctrlT > -999)
      sprintf(buf, "%.1fC  %s", ctrlT,
              ctrlT >= stageTargetTemp[1] ? "OK" : "BELOW");
    else
      strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 30, 2);

    uint16_t phBg = (ph > -999 && ph <= fermTargetPH) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 64, 300, 40, phBg);
    tft.drawRect(10, statusY + 64, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, phBg);
    tft.drawString("PH", 20, statusY + 74, 2);
    if (ph > -999)
      sprintf(buf, "%.2f  %s", ph, ph <= fermTargetPH ? "OK" : "ABOVE TARGET");
    else
      strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 74, 2);

    uint16_t gBg = (grav > -999 && grav <= fermTargetGravity) ? 0x0400 : 0x001F;
    tft.fillRect(10, statusY + 108, 300, 40, gBg);
    tft.drawRect(10, statusY + 108, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, gBg);
    tft.drawString("GRAVITY", 20, statusY + 118, 2);
    if (grav > -999)
      sprintf(buf, "%.3f  %s", grav,
              grav <= fermTargetGravity ? "AT TARGET" : "FERMENTING");
    else
      strcpy(buf, "NO DATA");
    tft.drawRightString(buf, 300, statusY + 118, 2);

  } else {
    float pastT = simActive ? simTempOverride[2]
                            : (liquid1Status ? getPastTemp() : -999.0f);
    uint16_t pBg =
        (pastT > -999 && pastT >= stageTargetTemp[2]) ? 0x0400 : 0xF800;
    tft.fillRect(10, statusY + 20, 300, 40, pBg);
    tft.drawRect(10, statusY + 20, 300, 40, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, pBg);
    tft.drawString(simActive ? "PAST. [SIM]" : "PAST. TEMP", 20, statusY + 30,
                   2);
    if (pastT > -999)
      sprintf(buf, "%.1fC  %s", pastT,
              pastT >= stageTargetTemp[2] ? "AT TARGET" : "BELOW");
    else
      strcpy(buf, "NO SENSOR");
    tft.drawRightString(buf, 300, statusY + 30, 2);
  }

  // --- Action button ---
  const char *btnLabels[] = {"ADVANCE TO FERMENTATION",
                             "ADVANCE TO PASTEURIZATION", "MARK BREW COMPLETE"};
  int btnY = (stageParamStage == 1) ? 382 : (stageParamStage == 2 ? 212 : 250);
  bool btnSel = (stageParamSelection == lastRow);
  bool isActive = (activeBrewStage == stageParamStage);
  uint16_t btnBg = btnSel ? 0x3566 : (isActive ? TFT_NAVY : TFT_DARKGREY);
  tft.fillRect(10, btnY, 300, 48, btnBg);
  tft.drawRect(10, btnY, 300, 48, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnBg);
  tft.drawCentreString(btnLabels[stageParamStage], CENTER_X, btnY + 10, 2);
  tft.drawCentreString(isActive ? "SELECT TO CONFIRM" : "NOT CURRENT STAGE",
                       CENTER_X, btnY + 30, 1);

  // --- Hints ---
  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString(stageParamEditing
                           ? "EDITING: L/R=ADJUST   DOWN/SELECT=DONE"
                           : "UP/DOWN: NAVIGATE   SELECT=EDIT   LEFT=BACK",
                       CENTER_X, 447, 1);
  if (simIsMan)
    tft.drawCentreString("MANUAL: UP/DOWN ON DASHBOARD ADJUSTS TEMP", CENTER_X,
                         462, 1);
  else
    tft.drawCentreString("SIM: L/R=VALUE  SELECT=CYCLE(OFF/SIM/DYN/MAN)",
                         CENTER_X, 462, 1);
}

void drawPidTestPick() {
  static int prevSel = -1;

  const char *options[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION", "THERMAL TRACKING"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    int y = 70 + (i * 85);
    tft.fillRect(20, y, 280, 70, color);
    tft.drawRect(20, y, 280, 70, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, y + 22, 4);
  };

  if (pidTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("PID CONTROL MODE", 10, 15, 4);
    for (int i = 0; i < 4; i++)
      drawTile(i, pidTestChoice == i);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK",
                         CENTER_X, 458, 1);
    pidTestNeedsFullRedraw = false;
    prevSel = pidTestChoice;
  } else if (prevSel != pidTestChoice) {
    if (prevSel >= 0)
      drawTile(prevSel, false);
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
  if (pidTestChoice == 0)
    chamberTitle = "PRE-HEAT";
  else if (pidTestChoice == 1)
    chamberTitle = "FERMENTATION";
  else
    chamberTitle = "PASTEURIZATION";
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString(chamberTitle, CENTER_X, 60, 4);

  // Target Heat Setting
  uint16_t heatBg =
      (!pidTestRunning && pidTestTargetSelection == 0) ? TFT_YELLOW : 0xD6BA;
  tft.fillRect(20, 100, 135, 62, heatBg);
  tft.drawRect(20, 100, 135, 62, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, heatBg);
  tft.drawCentreString("HEAT TARGET", 87, 108, 1);
  char buf[32];
  sprintf(buf, "%.1f C", pidTestHeatTarget);
  tft.drawCentreString(buf, 87, 118, 4);

  // Target Cool Setting
  uint16_t coolBg =
      (!pidTestRunning && pidTestTargetSelection == 1) ? TFT_YELLOW : 0xD6BA;
  tft.fillRect(165, 100, 135, 62, coolBg);
  tft.drawRect(165, 100, 135, 62, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, coolBg);
  tft.drawCentreString("COOL TARGET", 232, 108, 1);
  sprintf(buf, "%.1f C", pidTestCoolTarget);
  tft.drawCentreString(buf, 232, 118, 4);

  // Sensor selector (pre-heat + ferm only, y=165-177)
  if (pidTestChoice == 0 || pidTestChoice == 1) {
    uint16_t sensBg =
        (!pidTestRunning && pidTestTargetSelection == 2) ? TFT_YELLOW : 0xD6BA;
    tft.fillRect(20, 165, 280, 13, sensBg);
    tft.drawRect(20, 165, 280, 13, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, sensBg);
    int sel = (pidTestChoice == 0) ? pidPreHeatSensor : pidFermSensor;
    tft.drawCentreString(sel == 0 ? "SENSOR: LIQUID (DS18B20)"
                                  : "SENSOR: AMBIENT (BME280)",
                         CENTER_X, 167, 1);
  } else {
    tft.fillRect(20, 165, 280, 13, TFT_WHITE);
  }

  // Start / Stop Button
  uint16_t btnColor = pidTestRunning ? 0x0400 : 0xF800;
  tft.fillRect(20, 180, 280, 50, btnColor);
  tft.drawRect(20, 180, 280, 50, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnColor);
  tft.drawCentreString(pidTestRunning ? "STOP TEST" : "START TEST", CENTER_X,
                       195, 4);

  // Live Data Dashboard (drawn only if running)
  if (pidTestRunning) {
    float liquidTemp = -999.0f;
    float ambientTemp = -999.0f;
    if (pidTestChoice == 0) {
      if (liquid2Status)
        liquidTemp = getPreheatTemp();
      if (bme1Status)
        ambientTemp = bme1.readTemperature();
    } else if (pidTestChoice == 1) {
      if (incomingData.ds18Status == 1)
        liquidTemp = getFermTemp();
      if (incomingData.sensor2Status == 1)
        ambientTemp = incomingData.room2Temp;
    } else if (pidTestChoice == 2) {
      if (liquid1Status)
        liquidTemp = getPastTemp();
    }

    // Row 1: temp panels (y=240, h=55)
    if (pidTestChoice == 2) {
      // Pasteurization: single full-width liquid temp panel
      tft.fillRect(10, 240, 300, 55, 0x2124);
      tft.drawRect(10, 240, 300, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("LIQUID TEMP", CENTER_X, 250, 1);
      if (liquidTemp > -100.0f)
        sprintf(buf, "%.1f C", liquidTemp);
      else
        strcpy(buf, "---");
      tft.setTextPadding(280);
      tft.drawCentreString(buf, CENTER_X, 264, 2);
      tft.setTextPadding(0);
    } else {
      // Pre-heat and ferm: LIQUID TEMP | AMBIENT TEMP
      tft.fillRect(10, 240, 145, 55, 0x2124);
      tft.drawRect(10, 240, 145, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("LIQUID TEMP", 82, 250, 1);
      if (liquidTemp > -100.0f)
        sprintf(buf, "%.1f C", liquidTemp);
      else
        strcpy(buf, "---");
      tft.setTextPadding(130);
      tft.drawCentreString(buf, 82, 264, 2);

      tft.fillRect(165, 240, 145, 55, 0x2124);
      tft.drawRect(165, 240, 145, 55, TFT_DARKGREY);
      tft.setTextColor(TFT_WHITE, 0x2124);
      tft.drawCentreString("AMBIENT TEMP", 237, 250, 1);
      if (ambientTemp > -100.0f)
        sprintf(buf, "%.1f C", ambientTemp);
      else
        strcpy(buf, "---");
      tft.setTextPadding(130);
      tft.drawCentreString(buf, 237, 264, 2);
      tft.setTextPadding(0);
    }

    // Row 2: HEATER % | FAN % (y=302, h=50)
    bool heaterOn = currentHeatingPercent > 0;
    bool fanOn = isFanOn || isFermFanOn;
    uint16_t htrBg = heaterOn ? 0x0400 : 0xD6BA;
    uint16_t htrFg = heaterOn ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 302, 145, 50, htrBg);
    tft.drawRect(10, 302, 145, 50, TFT_DARKGREY);
    tft.setTextColor(htrFg, htrBg);
    tft.drawCentreString("HEATER", 82, 312, 1);
    if (heaterOn)
      sprintf(buf, "%d%%", currentHeatingPercent);
    else
      strcpy(buf, "OFF");
    tft.setTextPadding(130);
    tft.drawCentreString(buf, 82, 322, 2);

    uint16_t fanBg = fanOn ? 0x001F : 0xD6BA;
    uint16_t fanFg = fanOn ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(165, 302, 145, 50, fanBg);
    tft.drawRect(165, 302, 145, 50, TFT_DARKGREY);
    tft.setTextColor(fanFg, fanBg);
    tft.drawCentreString("FAN", 237, 312, 1);
    if (fanOn)
      sprintf(buf, "%d%%", pidFanPercent);
    else
      strcpy(buf, "OFF");
    tft.setTextPadding(130);
    tft.drawCentreString(buf, 237, 322, 2);
    tft.setTextPadding(0);

    // Row 3: Status banner (y=360, h=48)
    uint16_t statusBg;
    const char *statusText;
    if (pidTestSuccess) {
      statusBg = 0x0400;
      statusText = "STABLE!";
    } else if (heaterOn) {
      statusBg = 0xF800;
      statusText = "HEATING";
    } else if (fanOn) {
      statusBg = 0x001F;
      statusText = "COOLING";
    } else {
      statusBg = TFT_ORANGE;
      statusText = "AUTOMATING...";
    }
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
    uint16_t color = sel ? 0x3566 : 0xD6BA;
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
    for (int i = 0; i < 3; i++)
      drawTile(i, heaterTestStage == i);
    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SELECT   SELECT: CONFIRM   RETURN: BACK",
                         CENTER_X, 458, 1);
    heaterTestNeedsFullRedraw = false;
    prevSel = heaterTestStage;
  } else if (prevSel != heaterTestStage) {
    if (prevSel >= 0)
      drawTile(prevSel, false);
    drawTile(heaterTestStage, true);
    prevSel = heaterTestStage;
  }
}

void drawHeaterTestMenu() {
  const char *stageNames[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION"};
  const char *ssrNames[] = {"SSR PREHEAT", "SSR FERM", "SSR PAST"};
  char buf[40];

  bool dutySel = (heaterTestSelection == 0);
  bool startSel = (heaterTestSelection == 1);

  if (heaterTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("HEATER OUTPUT TEST", CENTER_X, 15, 4);
    tft.fillRect(0, 52, 320, 28, 0x4208);
    tft.setTextColor(TFT_WHITE, 0x4208);
    char sbuf[48];
    sprintf(sbuf, "%s  (%s)", stageNames[heaterTestStage],
            ssrNames[heaterTestStage]);
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
    tft.drawCentreString(heaterTestEditing ? "UP/DN TO ADJUST   SELECT: DONE"
                                           : "SELECT TO EDIT",
                         CENTER_X, 166, 1);
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
  tft.drawCentreString(heaterTestRunning ? "RUNNING" : "STOPPED", CENTER_X, 222,
                       4);
  tft.drawCentreString(heaterTestRunning ? "SELECT TO STOP" : "SELECT TO START",
                       CENTER_X, 268, 2);

  // Safety note
  tft.fillRect(0, 310, 320, 26, 0xFFE0);
  tft.setTextColor(TFT_BLACK, 0xFFE0);
  tft.drawCentreString("! MONITOR TEMPERATURE DURING TEST !", CENTER_X, 320, 1);

  // Footer
  tft.fillRect(0, 434, 320, 46, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (heaterTestEditing)
    tft.drawCentreString("UP/DN: ADJUST DUTY %   SELECT: DONE", CENTER_X, 447,
                         1);
  else
    tft.drawCentreString("UP/DN: NAVIGATE   SELECT: EDIT / START", CENTER_X,
                         447, 1);
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
  tft.drawCentreString(sdStatus ? "Card detected and mounted"
                                : "No card or mount error",
                       CENTER_X, 94, 1);

  // Write/read result
  uint16_t vrBg =
      (sdVerifyResult == 1) ? 0x0400 : (sdVerifyResult == 0 ? 0xF800 : 0xD6BA);
  const char *vrTxt = (sdVerifyResult == 1)   ? "WRITE / READ: PASS"
                      : (sdVerifyResult == 0) ? "WRITE / READ: FAIL"
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
  tft.drawCentreString(sdStatus ? "SELECT TO RUN" : "SD NOT AVAILABLE",
                       CENTER_X, 242, 1);

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

  uint32_t age = (lastDataReceivedMillis > 0)
                     ? (millis() - lastDataReceivedMillis) / 1000
                     : 9999;
  bool linkOk = (age < 5);

  // Link status
  uint16_t lBg = linkOk ? 0x0400 : 0xF800;
  tft.fillRect(10, 58, 300, 55, lBg);
  tft.drawRect(10, 58, 300, 55, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, lBg);
  tft.drawString("LINK", 20, 68, 2);
  tft.drawRightString(linkOk ? "OK" : "TIMEOUT", 300, 68, 2);
  sprintf(buf, "Last packet: %lus ago",
          (unsigned long)min(age, (uint32_t)9999));
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
  sprintf(buf, "Liquid: %.1fC   pH: %.2f", incomingData.room2LiquidTemp,
          incomingData.phValue);
  tft.drawCentreString(buf, CENTER_X, 258, 1);
  sprintf(buf, "Ambient: %.1fC  %.0fhPa", incomingData.room2Temp,
          incomingData.room2Pres);
  tft.drawCentreString(buf, CENTER_X, 274, 1);
  sprintf(buf, "Gravity: %.3f  Pill: %.1fC", incomingData.pillGravity,
          incomingData.pillTemp);
  tft.drawCentreString(buf, CENTER_X, 290, 1);
  sprintf(buf, "BLE: %s   ADS: %s   DS18: %s",
          incomingData.bleStatus ? "OK" : "--",
          incomingData.adsStatus ? "OK" : "--",
          incomingData.ds18Status ? "OK" : "--");
  tft.drawCentreString(buf, CENTER_X, 306, 1);
  sprintf(buf, "Battery: %d%%  RSSI: %d", incomingData.pillBattery,
          incomingData.pillRSSI);
  tft.drawCentreString(buf, CENTER_X, 322, 1);

  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("LIVE - UPDATES EVERY 1s   RETURN: BACK", CENTER_X, 455,
                       1);
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
    sprintf(buf, "CURRENT: %02d:%02d:%02d", now.hour(), now.minute(),
            now.second());
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
  tft.drawCentreString(rtcStatus ? "Writes to DS3231 chip"
                                 : "RTC not available",
                       CENTER_X, 262, 1);

  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: FIELD   L/R: ADJUST VALUE", CENTER_X, 447, 1);
  tft.drawCentreString("SELECT: SAVE   RETURN: CANCEL", CENTER_X, 462, 1);
}

void updateDashboardGraph() {
  if (currentAppState != DASHBOARD_ACTIVE || moduleViewActive ||
      activeBrewStage < 0 || stageTransferring)
    return;

  const int GX = 28;
  const int GPY = 308;
  const int GW = TEMP_GRAPH_W;
  const int GH = 152;

  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  uint16_t lineColor = stageColors[activeBrewStage];

  float yMin, yMax;
  float tickTemps[5];
  if (activeBrewStage == 1) {
    yMin = 20.0f;
    yMax = 40.0f;
    tickTemps[0] = 20;
    tickTemps[1] = 25;
    tickTemps[2] = 30;
    tickTemps[3] = 35;
    tickTemps[4] = 40;
  } else {
    yMin = 0.0f;
    yMax = 100.0f;
    tickTemps[0] = 0;
    tickTemps[1] = 25;
    tickTemps[2] = 50;
    tickTemps[3] = 75;
    tickTemps[4] = 100;
  }

  auto tempToY = [&](float t) -> int {
    int py = GPY + GH - 1 - (int)(((t)-yMin) / (yMax - yMin) * (GH - 1) + 0.5f);
    if (py < GPY)
      py = GPY;
    if (py > GPY + GH - 1)
      py = GPY + GH - 1;
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
    int startPixelX = GX;
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
  tft.drawString("Pre-heat:", 22, 103, 2);
  tft.drawRightString(s0, 302, 103, 2);
  tft.drawString("Fermentation:", 22, 123, 2);
  tft.drawRightString(s1, 302, 123, 2);
  tft.drawString("Pasteurization:", 22, 143, 2);
  tft.drawRightString(s2, 302, 143, 2);

  tft.drawFastHLine(10, 168, 300, TFT_DARKGREY);

  // SG + ABV
  char sgBuf[16], abvBuf[16];
  if (remoteStatusReceived && incomingData.bleStatus) {
    sprintf(sgBuf, "%.3f", incomingData.pillGravity);
    if (originalGravity > 0.0f) {
      float abv = (originalGravity - incomingData.pillGravity) * 131.25f;
      if (abv < 0.0f)
        abv = 0.0f;
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

void drawTransferTestMenu(bool valuesOnly, bool tileOnly) {
  static int prevSel = -1;
  char buf[48];
  uint32_t p1 = flowPulse1;
  uint32_t p2 = flowPulse2;
  float liters1 = (flowKFactor[0] > 0.0f) ? (float)p1 / flowKFactor[0] : 0.0f;
  float liters2 = (flowKFactor[1] > 0.0f) ? (float)p2 / flowKFactor[1] : 0.0f;

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

  // ---- tile draw helpers ----
  auto drawPump1 = [&](bool sel) {
    uint16_t bg = pumpPreHeatFermOn ? 0x0400 : 0xF800;
    tft.fillRect(20, 92, 130, 60, bg);
    tft.setTextColor(TFT_WHITE, bg);
    tft.drawCentreString("PUMP", 85, 97, 1);
    tft.drawCentreString(pumpPreHeatFermOn ? "ON" : "OFF", 85, 112, 4);
    if (sel) {
      tft.drawRect(20, 92, 130, 60, TFT_WHITE);
      tft.drawRect(21, 93, 128, 58, TFT_WHITE);
    } else {
      tft.drawRect(20, 92, 130, 60, TFT_DARKGREY);
    }
  };

  auto drawCal1 = [&](bool sel) {
    uint16_t bg = sel ? 0x3566 : 0xD6BA;
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 158, 280, 52, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString("CALIBRATE FLOW SENSOR", CENTER_X, 178, 2);
    if (sel) {
      tft.drawRect(20, 158, 280, 52, TFT_WHITE);
      tft.drawRect(21, 159, 278, 50, TFT_WHITE);
      tft.drawRect(22, 160, 276, 48, TFT_WHITE);
    } else {
      tft.drawRect(20, 158, 280, 52, TFT_DARKGREY);
    }
  };
  auto drawPump2 = [&](bool sel) {
    uint16_t bg = pumpFermPastOn ? 0x0400 : 0xF800;
    tft.fillRect(20, 285, 130, 60, bg);
    tft.setTextColor(TFT_WHITE, bg);
    tft.drawCentreString("PUMP", 85, 290, 1);
    tft.drawCentreString(pumpFermPastOn ? "ON" : "OFF", 85, 305, 4);
    if (sel) {
      tft.drawRect(20, 285, 130, 60, TFT_WHITE);
      tft.drawRect(21, 286, 128, 58, TFT_WHITE);
    } else {
      tft.drawRect(20, 285, 130, 60, TFT_DARKGREY);
    }
  };
  auto drawCal2 = [&](bool sel) {
    uint16_t bg = sel ? 0x3566 : 0xD6BA;
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 351, 280, 52, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString("CALIBRATE FLOW SENSOR", CENTER_X, 371, 2);
    if (sel) {
      tft.drawRect(20, 351, 280, 52, TFT_WHITE);
      tft.drawRect(21, 352, 278, 50, TFT_WHITE);
      tft.drawRect(22, 353, 276, 48, TFT_WHITE);
    } else {
      tft.drawRect(20, 351, 280, 52, TFT_DARKGREY);
    }
  };
  auto drawHdr1 = [&](bool active) {
    uint16_t bg = active ? 0x03E0 : 0xD6BA;
    uint16_t fg = active ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 58, 300, 28, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString("PRE-HEAT  ->  FERM", CENTER_X, 67, 2);
    tft.drawRect(10, 58, 300, 185, TFT_DARKGREY);
  };
  auto drawHdr2 = [&](bool active) {
    uint16_t bg = active ? 0x03E0 : 0xD6BA;
    uint16_t fg = active ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 251, 300, 28, bg);
    tft.setTextColor(fg, bg);
    tft.drawCentreString("FERM  ->  PAST", CENTER_X, 260, 2);
    tft.drawRect(10, 251, 300, 185, TFT_DARKGREY);
  };

  if (tileOnly) {
    if (transferTestSelection == 0)
      drawPump1(true);
    else if (transferTestSelection == 2)
      drawPump2(true);
    return;
  }

  if (transferTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("TRANSFER TEST", 10, 15, 4);
    transferTestNeedsFullRedraw = false;

    bool p1a = (transferTestSelection <= 1);
    drawHdr1(p1a);
    tft.fillRect(10, 86, 300, 72, TFT_WHITE);
    drawPump1(transferTestSelection == 0);
    tft.fillRect(160, 92, 140, 60, 0xD6BA);
    tft.drawRect(160, 92, 140, 60, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, 0xD6BA);
    tft.drawCentreString("FLOW", 230, 102, 2);
    tft.setTextPadding(130);
    sprintf(buf, "%.2f L", liters1);
    tft.drawCentreString(buf, 230, 118, 4);
    tft.setTextPadding(0);
    tft.fillRect(10, 152, 300, 6, TFT_WHITE);
    drawCal1(transferTestSelection == 1);
    tft.fillRect(10, 210, 300, 41, TFT_WHITE);

    drawHdr2(!p1a);
    tft.fillRect(10, 279, 300, 6, TFT_WHITE);
    drawPump2(transferTestSelection == 2);
    tft.fillRect(160, 285, 140, 60, 0xD6BA);
    tft.drawRect(160, 285, 140, 60, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, 0xD6BA);
    tft.drawCentreString("FLOW", 230, 295, 2);
    tft.setTextPadding(130);
    sprintf(buf, "%.2f L", liters2);
    tft.drawCentreString(buf, 230, 311, 4);
    tft.setTextPadding(0);
    tft.fillRect(10, 345, 300, 6, TFT_WHITE);
    drawCal2(transferTestSelection == 3);
    tft.fillRect(10, 403, 300, 40, TFT_WHITE);

    // redraw panel borders last — gap fills at x=10 overwrite them otherwise
    tft.drawRect(10, 58, 300, 185, TFT_DARKGREY);
    tft.drawRect(10, 251, 300, 185, TFT_DARKGREY);

    tft.fillRect(0, 443, 320, 37, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE   RETURN: BACK",
                         CENTER_X, 458, 1);

    prevSel = transferTestSelection;
  } else if (prevSel != transferTestSelection) {
    // Un-highlight previous tile
    if (prevSel == 0)
      drawPump1(false);
    else if (prevSel == 1)
      drawCal1(false);
    else if (prevSel == 2)
      drawPump2(false);
    else if (prevSel == 3)
      drawCal2(false);

    // Highlight new tile
    if (transferTestSelection == 0)
      drawPump1(true);
    else if (transferTestSelection == 1)
      drawCal1(true);
    else if (transferTestSelection == 2)
      drawPump2(true);
    else if (transferTestSelection == 3)
      drawCal2(true);

    // If crossing panel boundary, update both headers
    bool prevP1 = (prevSel <= 1);
    bool newP1 = (transferTestSelection <= 1);
    if (prevP1 != newP1) {
      drawHdr1(newP1);
      drawHdr2(!newP1);
    }

    prevSel = transferTestSelection;
  }
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
  bool resetSel = (flowCalSelection == 1);
  uint16_t resetBg = resetSel ? 0xF800 : 0xD6BA;
  uint16_t resetFg = resetSel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 276, 300, 50, resetBg);
  tft.setTextColor(resetFg, resetBg);
  tft.drawCentreString("RESET PULSE COUNT", CENTER_X, 295, 2);
  tft.drawRect(10, 276, 300, 50, resetSel ? TFT_WHITE : TFT_DARKGREY);

  // Row 2: CAPTURE
  bool capSel = (flowCalSelection == 2);
  uint16_t capBg = capSel ? 0x3566 : 0xD6BA;
  uint16_t capFg = capSel ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 334, 300, 84, capBg);
  tft.setTextColor(capFg, capBg);
  tft.drawCentreString("CAPTURE K-FACTOR", CENTER_X, 354, 2);
  tft.drawCentreString("flow known vol, then SELECT", CENTER_X, 374, 1);
  tft.drawRect(10, 334, 300, 84, capSel ? TFT_WHITE : TFT_DARKGREY);

  // Footer
  tft.fillRect(0, 432, 320, 48, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (flowCalEditing) {
    tft.drawCentreString("UP/DOWN: ADJUST VOLUME", CENTER_X, 446, 1);
    tft.drawCentreString("SELECT / RETURN: DONE EDITING", CENTER_X, 462, 1);
  } else {
    tft.drawCentreString("UP/DOWN: NAVIGATE   SELECT: ACTIVATE", CENTER_X, 446,
                         1);
    tft.drawCentreString("RETURN: BACK TO TRANSFER TEST", CENTER_X, 462, 1);
  }
}

void drawCalibWizard() {
  char buf[64];

  if (calibNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x4810); // Purple/brown header
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("CALIBRATION WIZARD", 10, 15, 4);
    calibNeedsFullRedraw = false;
  }

  // Row 0: Target
  uint16_t row0Bg =
      (calibSelection == 0) ? (wizardEditing ? 0x03E0 : 0x3566) : 0xD6BA;
  uint16_t row0Fg = (calibSelection == 0) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(15, 60, 290, 45, row0Bg);
  tft.drawRect(15, 60, 290, 45, TFT_DARKGREY);
  tft.setTextColor(row0Fg, row0Bg);
  tft.drawString("TARGET", 25, 75, 2);
  const char *targetNames[] = {"FLOW SENSOR 1", "FLOW SENSOR 2", "LOAD CELL"};
  if (calibSelection == 0) {
    if (wizardEditing) {
      sprintf(buf, "[ %s ]", targetNames[calibTarget]);
    } else {
      sprintf(buf, "< %s >", targetNames[calibTarget]);
    }
  } else {
    sprintf(buf, "%s", targetNames[calibTarget]);
  }
  tft.drawRightString(buf, 295, 75, 2);

  // Row 1: Volume
  uint16_t row1Bg =
      (calibSelection == 1) ? (wizardEditing ? 0x03E0 : 0x3566) : 0xD6BA;
  uint16_t row1Fg = (calibSelection == 1) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(15, 115, 290, 45, row1Bg);
  tft.drawRect(15, 115, 290, 45, TFT_DARKGREY);
  tft.setTextColor(row1Fg, row1Bg);
  tft.drawString("INPUT LIQUID", 25, 130, 2);
  if (calibSelection == 1) {
    if (wizardEditing) {
      sprintf(buf, "[ %.1f L ]", calibVolume);
    } else {
      sprintf(buf, "< %.1f L >", calibVolume);
    }
  } else {
    sprintf(buf, "%.1f L", calibVolume);
  }
  tft.drawRightString(buf, 295, 130, 2);

  // Row 2: Action Button
  uint16_t row2Bg = 0xD6BA;
  uint16_t row2Fg = TFT_BLACK;
  const char *actionText = "";

  if (calibTarget == 0 || calibTarget == 1) {
    // Flow Sensor
    if (calibRunning) {
      row2Bg = TFT_ORANGE;
      row2Fg = TFT_WHITE;
      actionText = "STOP & SAVE";
    } else {
      row2Bg = 0x03E0; // Green
      row2Fg = TFT_WHITE;
      actionText = "START PUMP & MEASURE";
    }
  } else {
    // Load Cell
    if (calibCompleted) {
      row2Bg = 0x03E0; // Green
      row2Fg = TFT_WHITE;
      actionText = "DONE (SUCCESS)";
    } else if (!calibTareDone) {
      row2Bg = 0x3566; // Blue
      row2Fg = TFT_WHITE;
      actionText = "STEP 1: TARE EMPTY VAT";
    } else {
      row2Bg = TFT_ORANGE;
      row2Fg = TFT_WHITE;
      actionText = "STEP 2: ADD LIQUID & CALIB";
    }
  }

  tft.drawRect(13, 178, 294, 54, (calibSelection == 2) ? TFT_BLACK : TFT_WHITE);
  tft.fillRect(15, 180, 290, 50, row2Bg);
  tft.drawRect(15, 180, 290, 50, TFT_DARKGREY);
  tft.setTextColor(row2Fg, row2Bg);
  tft.drawCentreString(actionText, CENTER_X, 195, 2);

  // Row 3: Status / Live Reading Display Box
  tft.fillRect(15, 245, 290, 165, 0xCE79); // Yellow-Green display panel
  tft.drawRect(15, 245, 290, 165, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xCE79);
  tft.drawCentreString("LIVE MONITOR", CENTER_X, 252, 2);

  int yLine = 280;
  if (calibTarget == 0 || calibTarget == 1) {
    // Flow Sensor Readings
    uint32_t pulses = (calibTarget == 0) ? flowPulse1 : flowPulse2;
    float currentK = flowKFactor[calibTarget];
    float estVol = (float)pulses / currentK;

    sprintf(buf, "PULSES: %lu", pulses);
    tft.drawString(buf, 30, yLine, 2);

    sprintf(buf, "EST. VOLUME: %.3f L", estVol);
    tft.drawString(buf, 30, yLine + 25, 2);

    sprintf(buf, "CURRENT K: %.1f", currentK);
    tft.drawString(buf, 30, yLine + 50, 2);

    if (calibRunning) {
      tft.setTextColor(TFT_RED, 0xCE79);
      tft.drawCentreString("PUMPING & MEASURING...", CENTER_X, yLine + 85, 2);
    } else if (calibCompleted) {
      tft.setTextColor(0x03E0, 0xCE79);
      sprintf(buf, "SAVED NEW K: %.1f", currentK);
      tft.drawCentreString(buf, CENTER_X, yLine + 85, 2);
    } else {
      tft.drawCentreString("READY TO START", CENTER_X, yLine + 85, 2);
    }
  } else {
    // Load Cell Readings
    float currentFactor = calibrationFactor;
    float estWt = currentWeight;

    sprintf(buf, "RAW ADC: %ld", rawHX711);
    tft.drawString(buf, 30, yLine, 2);

    sprintf(buf, "EST. WEIGHT: %.2f L", estWt);
    tft.drawString(buf, 30, yLine + 25, 2);

    sprintf(buf, "CURRENT FACTOR: %.2f", currentFactor);
    tft.drawString(buf, 30, yLine + 50, 2);

    if (calibTareDone && !calibCompleted) {
      tft.setTextColor(TFT_RED, 0xCE79);
      tft.drawCentreString("TARED. ADD LIQUID & HIT SELECT", CENTER_X,
                           yLine + 85, 2);
    } else if (calibCompleted) {
      tft.setTextColor(0x03E0, 0xCE79);
      sprintf(buf, "SAVED NEW FACTOR: %.1f", currentFactor);
      tft.drawCentreString(buf, CENTER_X, yLine + 85, 2);
    } else {
      tft.drawCentreString("READY (STEP 1: TARE)", CENTER_X, yLine + 85, 2);
    }
  }

  // Footer / Key Help Hints
  tft.fillRect(0, 420, 320, 60, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (calibRunning) {
    tft.drawCentreString("PUMP ACTIVE! LED FLASHING RAPIDLY", CENTER_X, 432, 1);
    tft.drawCentreString("SELECT: STOP & SAVE CALIBRATION", CENTER_X, 452, 1);
  } else {
    tft.drawCentreString("UP/DOWN: NAV   LEFT/RIGHT: ADJUST   SELECT: RUN",
                         CENTER_X, 432, 1);
    if (calibTareDone) {
      tft.drawCentreString("LEFT (on Row 2): RESET TARE   RETURN: BACK",
                           CENTER_X, 452, 1);
    } else {
      tft.drawCentreString("RETURN (LEFT on Row 0/2): RETURN TO MENU", CENTER_X,
                           452, 1);
    }
  }
}

void drawRaptTestPage(bool valuesOnly) {
  char buf[64];

  if (!valuesOnly) {
    if (raptTestNeedsFullRedraw) {
      tft.fillRect(0, 0, 320, 50, 0x03E0); // Green header
      tft.fillRect(0, 50, 320, 430, TFT_WHITE);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("RAPT PILL TELEMETRY", CENTER_X, 15, 4);

      // Footer
      tft.fillRect(0, 434, 320, 46, TFT_WHITE);
      tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
      tft.drawCentreString("RETURN: BACK", CENTER_X, 458, 2);

      raptTestNeedsFullRedraw = false;
    }
  }

  // Clear the log body area (leaving room for borders/margins)
  tft.fillRect(10, 55, 300, 370, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Draw count header
  sprintf(buf, "LOGS RECEIVED: %d / 10", raptLogCount);
  tft.drawCentreString(buf, CENTER_X, 60, 2);

  // Draw logs
  for (int i = 0; i < raptLogCount; i++) {
    int y = 90 + (i * 32);
    // Draw background panel for each log line
    tft.fillRect(15, y, 290, 28, 0xD6BA);
    tft.drawRect(15, y, 290, 28, TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK, 0xD6BA);

    // Format log: e.g. "[1] SG: 1.0090  Time: 12:34:56"
    sprintf(buf, "[%d] SG: %.4f  Time: %s", i + 1, 
            raptLogs[i].gravity, raptLogs[i].timeStr);
    tft.drawString(buf, 22, y + 6, 2);
  }

  // If no logs yet
  if (raptLogCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("Waiting for RAPT Pill data...", CENTER_X, 220, 2);
  }
}

void drawPhFermMenu(bool valuesOnly) {
  char buf[32];

  if (!valuesOnly) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    // Pseudo-bold: draw twice at 1px vertical offset
    tft.drawCentreString("PH & FERM TEMP", CENTER_X, 11, 4);
    tft.drawCentreString("PH & FERM TEMP", CENTER_X, 12, 4);

    tft.fillRect(0, 52, 320, 28, 0x4208);
    tft.setTextColor(TFT_WHITE, 0x4208);
    tft.drawCentreString("pH SLOPE COMPENSATED BY LIQUID TEMP", CENTER_X, 60, 1);

    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString("pH READING (ADS1115)", CENTER_X, 83, 2);
    tft.drawCentreString("LIQUID TEMP (DS18B20)", CENTER_X, 259, 2);

    tft.drawRect(10, 100, 300, 148, TFT_DARKGREY);
    tft.drawRect(10, 276, 300, 148, TFT_DARKGREY);

    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("RETURN: BACK", CENTER_X, 458, 2);

    phFermNeedsFullRedraw = false;
  }

  // pH panel (y=100, h=148): value fill 125px + status 22px
  bool adsOk = (incomingData.adsStatus == 1);
  uint16_t phBg = adsOk ? 0x2124 : 0x4208;
  tft.fillRect(11, 101, 298, 125, phBg);
  tft.setTextColor(TFT_WHITE, phBg);
  if (adsOk) {
    sprintf(buf, "%.2f", incomingData.phValue);
    tft.drawCentreString(buf, CENTER_X, 151, 4);
  } else {
    tft.drawCentreString("---", CENTER_X, 151, 4);
  }
  uint16_t phStatBg = adsOk ? 0x0400 : 0xF800;
  tft.fillRect(11, 226, 298, 21, phStatBg);
  tft.setTextColor(TFT_WHITE, phStatBg);
  tft.drawCentreString(adsOk ? "OK" : "ERROR", CENTER_X, 230, 1);

  // Liquid panel (y=276, h=148): value fill 125px + status 22px
  bool liqOk = (incomingData.ds18Status == 1)
               && (incomingData.room2LiquidTemp > -100.0f);
  uint16_t liqBg = liqOk ? 0x2124 : 0x4208;
  tft.fillRect(11, 277, 298, 125, liqBg);
  tft.setTextColor(TFT_WHITE, liqBg);
  if (liqOk) {
    float t = incomingData.room2LiquidTemp + fermTempOffset;
    sprintf(buf, "%.1f C", t);
    tft.drawCentreString(buf, CENTER_X, 327, 4);
  } else {
    tft.drawCentreString("---", CENTER_X, 327, 4);
  }
  uint16_t liqStatBg = liqOk ? 0x0400 : 0xF800;
  tft.fillRect(11, 402, 298, 21, liqStatBg);
  tft.setTextColor(TFT_WHITE, liqStatBg);
  tft.drawCentreString(liqOk ? "OK" : "NO DATA", CENTER_X, 406, 1);
}

void drawPidTrackingMenu(bool valuesOnly) {
  char buf[40];

  if (!valuesOnly || pidTrackNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("PID THERMAL TRACKING", 10, 15, 4);

    // Static Metrics Container (Y=90 to 205)
    tft.drawRect(10, 90, 300, 115, TFT_DARKGREY);

    // Graph Area Border & Canvas Setup (Y=215 to 425)
    tft.fillRect(15, 215, 290, 210, 0x2124); // Dark panel
    tft.drawRect(15, 215, 290, 210, TFT_DARKGREY);

    // Graph Title & Target reference
    tft.setTextColor(TFT_LIGHTGREY, 0x2124);
    tft.drawString("TEMP TREND (20C - 100C)", 22, 220, 1);
    
    // Draw 80C target reference line (Y_pixel = 265)
    tft.drawFastHLine(15, 265, 290, TFT_RED);
    tft.setTextColor(TFT_RED, 0x2124);
    tft.drawString("80C SETPOINT", 200, 253, 1);

    // Footer
    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    if (!pidTrackRunning) {
      tft.drawCentreString("UP/DN: TSET   RIGHT: +5C   SELECT: START   RETURN: BACK", CENTER_X, 458, 1);
    } else {
      tft.drawCentreString("SELECT: STOP TEST   RETURN: BACK", CENTER_X, 458, 1);
    }

    pidTrackNeedsFullRedraw = false;
  }

  // --- Dynamic Value Updates ---
  // Status Bar (Y=55 to 85)
  uint16_t statBg = pidTrackRunning ? 0x0400 : 0x4208;
  tft.fillRect(10, 55, 300, 30, statBg);
  tft.drawRect(10, 55, 300, 30, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, statBg);
  if (pidTrackRunning) {
    uint32_t runSec = (millis() - pidTrackStartMs) / 1000;
    sprintf(buf, "RUNNING (%lus)  TSET: %.1f C", (unsigned long)runSec, pidTrackTargetTemp);
  } else {
    sprintf(buf, "TSET: %.1f C  (UP/DN TO EDIT)", pidTrackTargetTemp);
  }
  tft.drawCentreString(buf, CENTER_X, 62, 2);

  // Metrics Grid (Y=90 to 205)
  tft.fillRect(11, 91, 298, 113, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Current Temp / Setpoint
  float curTemp = liquid2Status ? getPreheatTemp() : 0.0f;
  sprintf(buf, "TEMP: %.1f C / %.1f C", curTemp, pidTrackTargetTemp);
  tft.drawString(buf, 20, 96, 2);

  // Steady Error
  sprintf(buf, "ERR: %.2f C", pidTrackMetrics.steadyStateError);
  tft.drawRightString(buf, 300, 96, 2);

  // Rise Time
  if (pidTrackMetrics.riseTimeSec >= 0) {
    sprintf(buf, "RISE TIME: %ds", pidTrackMetrics.riseTimeSec);
  } else {
    sprintf(buf, "RISE TIME: ---");
  }
  tft.drawString(buf, 20, 122, 2);

  // Settling Time
  if (pidTrackMetrics.settlingTimeSec >= 0) {
    sprintf(buf, "SETTLE TIME: %ds", pidTrackMetrics.settlingTimeSec);
  } else {
    sprintf(buf, "SETTLE TIME: ---");
  }
  tft.drawRightString(buf, 300, 122, 2);

  // Overshoot
  sprintf(buf, "OVERSHOOT: +%.1f C", pidTrackMetrics.overshootDeg);
  tft.drawString(buf, 20, 148, 2);

  // Stability Status
  uint16_t stabColor = TFT_BLACK;
  if (strcmp(pidTrackMetrics.stabilityStr, "STABLE") == 0) stabColor = 0x0400;
  else if (strcmp(pidTrackMetrics.stabilityStr, "SETTLING") == 0) stabColor = 0xE7E0;
  else if (strcmp(pidTrackMetrics.stabilityStr, "UNSTABLE") == 0) stabColor = TFT_RED;
  
  tft.setTextColor(stabColor, TFT_WHITE);
  sprintf(buf, "STABILITY: %s", pidTrackMetrics.stabilityStr);
  tft.drawRightString(buf, 300, 148, 2);

  // Divider line inside metrics box
  tft.drawFastHLine(15, 175, 290, TFT_LIGHTGREY);

  // Live Graph Render
  int targetY = 415 - (int)((pidTrackTargetTemp - 20.0f) * 190.0f / 80.0f);
  targetY = constrain(targetY, 218, 415);

  if (pidTrackHistoryCount > 1) {
    // Redraw graph area inside canvas (X: 16..304, Y: 216..424)
    tft.fillRect(16, 216, 288, 208, 0x2124);
    
    // Draw Dynamic Target Reference Line
    tft.drawFastHLine(16, targetY, 288, TFT_RED);
    tft.setTextColor(TFT_RED, 0x2124);
    sprintf(buf, "%.0fC SETPOINT", pidTrackTargetTemp);
    int txtY = (targetY - 12 < 218) ? targetY + 2 : targetY - 12;
    tft.drawString(buf, 190, txtY, 1);

    int count = min(pidTrackHistoryCount, 100);
    float xStep = 280.0f / max(1, count - 1);

    for (int i = 0; i < count - 1; i++) {
      float t1 = pidTrackHistory[i];
      float t2 = pidTrackHistory[i + 1];

      // Constrain temperature between 20C and 100C for plotting
      t1 = constrain(t1, 20.0f, 100.0f);
      t2 = constrain(t2, 20.0f, 100.0f);

      // Y scale: 20C maps to Y=415, 100C maps to Y=225 (height 190px)
      int y1 = 415 - (int)((t1 - 20.0f) * 190.0f / 80.0f);
      int y2 = 415 - (int)((t2 - 20.0f) * 190.0f / 80.0f);

      int x1 = 20 + (int)(i * xStep);
      int x2 = 20 + (int)((i + 1) * xStep);

      tft.drawLine(x1, y1, x2, y2, 0x07E0); // Bright green line
    }
  }
}

