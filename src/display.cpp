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
    tft.fillRect(0, 106, 320, 184, TFT_WHITE);
    for (int i = 0; i < 3; i++) {
      int y = 110 + (i * 60);
      bool sel = (i == dashSelection);
      if (sel) {
        tft.fillRect(1, y - 3, 318, 56, TFT_BLACK);
      }
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
      // ---- Brew Complete Summary ----
      tft.fillRect(0, 290, 320, 190, TFT_WHITE);
      tft.fillRect(0, 290, 320, 20, TFT_NAVY);
      tft.setTextColor(TFT_WHITE, TFT_NAVY);
      tft.drawCentreString("BREW COMPLETE", CENTER_X, 295, 2);

      // Total time
      uint32_t totalMs = stageElapsedMs[0] + stageElapsedMs[1] + stageElapsedMs[2];
      char totBuf[20];
      formatStageTimer(totalMs, totBuf);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.drawString("TOTAL TIME:", 10, 315, 2);
      tft.drawRightString(totBuf, 310, 315, 2);

      // Per-stage times
      char s0[16], s1[16], s2[16];
      formatStageTimer(stageElapsedMs[0], s0);
      formatStageTimer(stageElapsedMs[1], s1);
      formatStageTimer(stageElapsedMs[2], s2);
      tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
      tft.drawString("Pre-heat:", 18, 334, 1);  tft.drawRightString(s0, 310, 334, 1);
      tft.drawString("Ferment:",  18, 345, 1);  tft.drawRightString(s1, 310, 345, 1);
      tft.drawString("Pasteur:",  18, 356, 1);  tft.drawRightString(s2, 310, 356, 1);

      tft.drawFastHLine(10, 368, 300, TFT_DARKGREY);

      // Final SG + Est. ABV
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
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.drawString("FINAL SG:", 10, 375, 2);
      tft.drawString(sgBuf, 120, 375, 2);
      tft.drawString("ABV:", 175, 375, 2);
      tft.drawRightString(abvBuf, 310, 375, 2);

      // Final pH + Weight
      char phBuf[16], wtBuf[16];
      if (remoteStatusReceived && incomingData.adsStatus)
        sprintf(phBuf, "%.2f", incomingData.phValue);
      else
        strcpy(phBuf, "--");
      if (hx711Status)
        sprintf(wtBuf, "%.0fg", currentWeight);
      else
        strcpy(wtBuf, "--");
      tft.drawString("FINAL pH:", 10, 398, 2);
      tft.drawString(phBuf, 120, 398, 2);
      tft.drawString("WT:", 175, 398, 2);
      tft.drawRightString(wtBuf, 310, 398, 2);
    } else {
      tft.fillRect(0, 290, 320, 190, TFT_WHITE);
      // Button hints
      bool isManual = activeBrewStage >= 0 && simManual[activeBrewStage];
      tft.fillRect(0, 292, 320, 1, TFT_DARKGREY);
      tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
      if (isManual) {
        tft.drawString("UP",     10, 300, 2); tft.drawString("+5 C (SIM)",  55, 300, 2);
        tft.drawString("DOWN",   10, 318, 2); tft.drawString("-5 C (SIM)",  55, 318, 2);
        tft.drawString("SELECT", 10, 336, 2); tft.drawString("Expand Stage", 65, 336, 2);
        tft.drawString("LEFT",   10, 354, 2); tft.drawString("Return to Menu", 55, 354, 2);
      } else {
        tft.drawString("UP / DOWN", 10, 300, 2); tft.drawString("Navigate Stages",  100, 300, 2);
        tft.drawString("SELECT",    10, 318, 2); tft.drawString("Expand Stage",      65,  318, 2);
        tft.drawString("LEFT",      10, 336, 2); tft.drawString("Return to Menu",    55,  336, 2);
        tft.drawString("RIGHT",     10, 354, 2); tft.drawString("Navigate Stages",   65,  354, 2);
      }
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
  char timerBuf[16] = "";
  formatStageTimer(millis() - stageStartMillis, timerBuf);
  tft.setTextColor(TFT_WHITE, colors[i]);
  tft.setTextPadding(130);
  tft.drawString(timerBuf, 35, y + 35, 1);
  tft.setTextPadding(0);
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

void drawLoadCellPage(bool valuesOnly) {
  char b[32];

  if (!valuesOnly) {
    if (loadCellNeedsFullRedraw) {
      tft.fillScreen(TFT_WHITE);
      // Header
      tft.fillRect(0, 0, 320, 50, 0x0493);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("LOAD CELL", CENTER_X, 15, 4);
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
  if (systemCheckNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SYSTEM CHECK", CENTER_X, 15, 4);
    systemCheckNeedsFullRedraw = false;
  }
  const char *options[] = {
    "FAN TEST", "LIGHT INDICATORS", "RELAY TEST",
    "MOTOR TEST", "PID CONTROL",
    "HEATER OUTPUT", "SD CARD VERIFY", "UART MONITOR", "SET RTC TIME"
  };
  for (int i = 0; i < 9; i++) {
    uint16_t color    = (systemCheckSelection == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (systemCheckSelection == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 52 + (i * 43), 300, 38, color);
    tft.drawRect(10, 52 + (i * 43), 300, 38, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 62 + (i * 43), 2);
  }
  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO GO BACK", CENTER_X, 455, 2);
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

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("SELECT TO TOGGLE", CENTER_X, 420, 2);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 450, 2);
}

void drawRelayTestMenu() {
  if (relayTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("RELAY TEST", CENTER_X, 15, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("AUTO-SEQUENCING  500ms/CH", CENTER_X, 420, 2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString("RETURN TO EXIT", CENTER_X, 455, 2);
    relayTestNeedsFullRedraw = false;
  }

  // Channel labels and circle centers: 3 rows of 3, x=55,160,265
  const int cx[3]    = {55, 160, 265};
  const int rowY[3]  = {120, 230, 340};
  const int r        = 35;
  const char *labels[9] = {"CH1","CH2","CH3","CH4","CH5","CH6","CH7","CH8","FAN"};

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
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("MOTOR TEST", CENTER_X, 15, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: SPEED   L/R: DIR", CENTER_X, 415, 2);
    tft.drawCentreString("SELECT: STOP   RETURN: EXIT", CENTER_X, 440, 2);
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

void drawStageParamMenu() {
  const char    *stageNames[]  = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  char buf[40];
  bool simActive  = (simTempOverride[stageParamStage] > 0.0f);
  bool simIsDyn   = simActive && simDynamic[stageParamStage];
  bool simIsMan   = simActive && simManual[stageParamStage];

  if (stageParamNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
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
  if (pidTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SELECT CHAMBER", CENTER_X, 15, 4);
    pidTestNeedsFullRedraw = false;
  }
  const char *options[] = {"PRE-HEAT CHAMBER", "FERMENTATION CHAMBER", "PASTEURIZATION CHAMBER"};
  for (int i = 0; i < 3; i++) {
    uint16_t color    = (pidTestChoice == i) ? 0x3566 : 0xD6BA;
    uint16_t txtColor = (pidTestChoice == i) ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(20, 80 + (i * 110), 280, 80, color);
    tft.drawRect(20, 80 + (i * 110), 280, 80, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 105 + (i * 110), 4);
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("RETURN TO GO BACK", CENTER_X, 450, 2);
}

void drawPidTestMenu() {
  if (pidTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("PID CONTROL TEST", CENTER_X, 15, 4);
    pidTestNeedsFullRedraw = false;
  }

  const char *chamberTitle;
  if (pidTestChoice == 0) chamberTitle = "PRE-HEAT";
  else if (pidTestChoice == 1) chamberTitle = "FERMENTATION";
  else chamberTitle = "PASTEURIZATION";
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString(chamberTitle, CENTER_X, 60, 4);

  // Target Temp Setting
  tft.fillRect(20, 100, 280, 70, 0xD6BA);
  tft.drawRect(20, 100, 280, 70, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("TARGET TEMP", CENTER_X, 110, 2);
  char buf[32];
  sprintf(buf, "%.1f C", pidTestTarget);
  tft.drawCentreString(buf, CENTER_X, 135, 4);

  // Start / Stop Button
  uint16_t btnColor = pidTestRunning ? TFT_RED : 0x0400;
  tft.fillRect(20, 180, 280, 50, btnColor);
  tft.drawRect(20, 180, 280, 50, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, btnColor);
  tft.drawCentreString(pidTestRunning ? "STOP TEST" : "START TEST", CENTER_X, 195, 4);

  // Live Data Dashboard (drawn only if running)
  if (pidTestRunning) {
    // Current Temp
    float liquidTemp = -999.0f;
    if (pidTestChoice == 0 && liquid2Status) {
      liquidTemp = sharedLiquidSensors.getTempCByIndex(1);
    } else if (pidTestChoice == 1 && incomingData.ds18Status == 1) {
      liquidTemp = incomingData.room2LiquidTemp;
    } else if (pidTestChoice == 2 && liquid1Status) {
      liquidTemp = sharedLiquidSensors.getTempCByIndex(0);
    }
    
    tft.fillRect(20, 240, 135, 60, 0x3566);
    tft.drawRect(20, 240, 135, 60, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, 0x3566);
    tft.drawCentreString("LIQUID TEMP", 87, 250, 1);
    if (liquidTemp > -100.0f) sprintf(buf, "%.1f C", liquidTemp);
    else strcpy(buf, "--");
    tft.drawCentreString(buf, 87, 270, 2);

    // Heating Percent
    tft.fillRect(165, 240, 135, 60, 0x3566);
    tft.drawRect(165, 240, 135, 60, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, 0x3566);
    tft.drawCentreString("HEATING %", 232, 250, 1);
    sprintf(buf, "%d%%", currentHeatingPercent);
    tft.drawCentreString(buf, 232, 270, 2);

    // Status Banner
    uint16_t statusBg = pidTestSuccess ? 0x0400 : TFT_ORANGE;
    tft.fillRect(20, 310, 280, 70, statusBg);
    tft.drawRect(20, 310, 280, 70, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, statusBg);
    tft.drawCentreString(pidTestSuccess ? "SUCCESS - STABLE!" : "AUTOMATING...", CENTER_X, 335, 4);
  } else {
    tft.fillRect(20, 240, 280, 140, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("SELECT START TO BEGIN", CENTER_X, 290, 2);
  }

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("UP/DN: ADJ TEMP  |  SEL: START/STOP", CENTER_X, 420, 1);
  tft.drawCentreString("RETURN TO EXIT", CENTER_X, 450, 2);
}

void drawHeaterTestMenu() {
  const char *stageNames[] = {"PRE-HEAT", "FERMENTATION", "PASTEURIZATION"};
  const char *pinNames[]   = {"GPIO13 / DIM2_SHARED", "GPIO12 / DIM1_CH2", "GPIO14 / DIM1_CH1"};
  char buf[40];

  if (heaterTestNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("HEATER OUTPUT TEST", CENTER_X, 15, 4);
    heaterTestNeedsFullRedraw = false;
  }

  // Stage
  tft.fillRect(10, 58, 300, 65, 0xD6BA);
  tft.drawRect(10, 58, 300, 65, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawString("STAGE", 20, 68, 2);
  tft.drawCentreString(stageNames[heaterTestStage], CENTER_X, 78, 4);
  tft.setTextColor(TFT_DARKGREY, 0xD6BA);
  tft.drawCentreString(pinNames[heaterTestStage], CENTER_X, 112, 1);

  // Duty cycle
  uint16_t pctBg = heaterTestRunning ? 0xFBE0 : 0xD6BA;
  tft.fillRect(10, 131, 300, 60, pctBg);
  tft.drawRect(10, 131, 300, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, pctBg);
  tft.drawString("DUTY CYCLE", 20, 141, 2);
  sprintf(buf, "%d%%", heaterTestPercent);
  tft.drawCentreString(buf, CENTER_X, 152, 4);

  // Status
  if (heaterTestRunning) {
    uint32_t elapsed   = (millis() - heaterTestStart) / 1000;
    uint32_t remaining = (elapsed < 30) ? 30 - elapsed : 0;
    tft.fillRect(10, 199, 300, 55, 0xF800);
    tft.drawRect(10, 199, 300, 55, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, 0xF800);
    tft.drawString("RUNNING", 20, 209, 2);
    sprintf(buf, "AUTO-OFF IN %lds", (long)remaining);
    tft.drawRightString(buf, 300, 209, 2);
    tft.drawCentreString("SELECT TO STOP NOW", CENTER_X, 232, 2);
  } else {
    tft.fillRect(10, 199, 300, 55, 0x0400);
    tft.drawRect(10, 199, 300, 55, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, 0x0400);
    tft.drawString("STOPPED", 20, 209, 2);
    tft.drawCentreString("SELECT TO START", CENTER_X, 232, 2);
  }

  // Warning
  tft.fillRect(0, 262, 320, 30, 0xFFE0);
  tft.setTextColor(TFT_BLACK, 0xFFE0);
  tft.drawCentreString("! HEATER CUTS OFF AFTER 30 SECONDS !", CENTER_X, 273, 1);

  // Hints
  tft.fillRect(0, 440, 320, 40, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DOWN: STAGE   L/R: DUTY %", CENTER_X, 447, 1);
  tft.drawCentreString("SELECT: START/STOP   RETURN: EXIT", CENTER_X, 462, 1);
}

void drawSdVerifyMenu() {
  char buf[40];

  if (sdVerifyNeedsFullRedraw) {
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SD CARD VERIFY", CENTER_X, 15, 4);
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
  uint16_t btnBg = sdStatus ? TFT_NAVY : TFT_DARKGREY;
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
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("UART MONITOR", CENTER_X, 15, 4);
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
    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SET RTC TIME", CENTER_X, 15, 4);
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
  uint16_t saveBg = rtcStatus ? TFT_NAVY : TFT_DARKGREY;
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
