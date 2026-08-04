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

  char buf[64];

  // Current weight box
  tft.fillRect(20, 70, 280, 80, 0xD6BA);
  tft.drawRect(20, 70, 280, 80, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("CURRENT WEIGHT", CENTER_X, 80, 2);
  if (hx711Status)
    sprintf(buf, "%.1f L", currentWeight);
  else
    strcpy(buf, "FAILED");
  tft.drawCentreString(buf, CENTER_X, 108, 4);

  // Minimum Required Volume (Read-only)
  tft.fillRect(20, 175, 280, 75, 0xCE79);
  tft.drawRect(20, 175, 280, 75, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xCE79);
  tft.drawCentreString("MINIMUM REQUIRED", CENTER_X, 185, 2);
  sprintf(buf, "%.1f L", minVolumeReq);
  tft.drawCentreString(buf, CENTER_X, 212, 4);

  // PROCEED Button / Safety Lock
  bool canProceed = (currentWeight >= minVolumeReq);
  uint16_t rowBg = canProceed ? 0x03E0 : 0x4208;
  tft.fillRect(20, 280, 280, 60, rowBg);
  tft.drawRect(20, 280, 280, 60, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, rowBg);
  if (canProceed) {
    tft.drawCentreString("PROCEED (START BREW)", CENTER_X, 298, 2);
  } else {
    tft.drawCentreString("LOCKED: MIN VOL NOT MET", CENTER_X, 298, 2);
  }

  // Help Footer
  tft.fillRect(0, 420, 320, 60, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("SELECT: START BREW", CENTER_X, 432, 2);
  tft.drawCentreString("RETURN (LEFT): CANCEL & BACK", CENTER_X, 455, 1);
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

void updateDashboardValues() {
  if (currentAppState != DASHBOARD_ACTIVE) return;

  char buf[64];
  uint16_t bgCard = 0xD6BA; // Light Gray tile fill

  // Live countdown timer text on Header Tile (Y=104 to 144)
  const uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};
  uint16_t viewColor = stageColors[dashSelection];
  tft.fillRect(265, 115, 28, 18, viewColor);
  if (dashSelection != activeBrewStage && activeBrewStage >= 0) {
    int remSec = 5 - (int)((millis() - lastPhaseViewNavMs) / 1000);
    if (remSec < 1) remSec = 1;
    sprintf(buf, "%ds", remSec);
    tft.setTextColor(TFT_YELLOW, viewColor);
    tft.drawString(buf, 266, 118, 2);
  }

  if (dashSelection == 0) { // PRE-HEATING VIEW
    // Ambient Temp Tile (x=5, y=148, w=152, h=54)
    tft.fillRect(7, 168, 148, 32, bgCard);
    String pa = bme1Status ? String(bme1.readTemperature(), 1) + " C" : "-- C";
    tft.setTextColor(TFT_BLACK, bgCard);
    tft.drawCentreString(pa, 81, 172, 4);

    // Liquid Temp Tile (x=163, y=148, w=152, h=54)
    tft.fillRect(165, 168, 148, 32, bgCard);
    String pl = (simTempOverride[0] > 0.0f) ? String(simTempOverride[0], 1) + " C" : (liquid2Status ? String(getPreheatTemp(), 1) + " C" : "-- C");
    tft.drawCentreString(pl, 239, 172, 4);

    // Status Tile (x=5, y=206, w=310, h=108)
    tft.fillRect(7, 226, 306, 86, bgCard);
    tft.setTextColor(TFT_BLACK, bgCard);
    if (stageTransferring && activeBrewStage == 0) {
      float totalVol = (transferStartWeight > 0.0f) ? transferStartWeight : 10.0f;
      float xferVol = 0.0f;
      if (currentWeight < totalVol) xferVol = totalVol - currentWeight;
      else xferVol = (float)((millis() - transferStartMs) / 1000) * (totalVol / 10.0f);
      if (xferVol > totalVol) xferVol = totalVol;

      tft.drawCentreString("TRANSFERRING", CENTER_X, 240, 4);
      sprintf(buf, "(%.1fL of %.1fL)", xferVol, totalVol);
      tft.drawCentreString(buf, CENTER_X, 278, 2);

    } else if (preHeatSterilized && isFanOn) {
      tft.drawCentreString("COOLING", CENTER_X, 240, 4);
      sprintf(buf, "(%d%% Power)", currentSpeedPercent);
      tft.drawCentreString(buf, CENTER_X, 278, 2);

    } else if (preHeatHolding) {
      tft.drawCentreString("HOLDING", CENTER_X, 240, 4);
      tft.drawCentreString("(80.0 C Target)", CENTER_X, 278, 2);

    } else if (currentHeatingPercent > 0) {
      tft.drawCentreString("HEATING", CENTER_X, 240, 4);
      sprintf(buf, "(%d%% Power)", currentHeatingPercent);
      tft.drawCentreString(buf, CENTER_X, 278, 2);

    } else {
      tft.drawCentreString("IDLE", CENTER_X, 252, 4);
    }

  } else if (dashSelection == 1) { // FERMENTATION VIEW
    // Ambient Temp Tile (x=5, y=148, w=152, h=54)
    tft.fillRect(7, 168, 148, 32, bgCard);
    String fa = (incomingData.sensor2Status > 0) ? String(incomingData.room2Temp, 1) + " C" : "-- C";
    tft.setTextColor(TFT_BLACK, bgCard);
    tft.drawCentreString(fa, 81, 172, 4);

    // Liquid Temp Tile (x=163, y=148, w=152, h=54)
    tft.fillRect(165, 168, 148, 32, bgCard);
    String fl = (simTempOverride[1] > 0.0f) ? String(simTempOverride[1], 1) + " C" : (incomingData.ds18Status == 1 ? String(getFermTemp(), 1) + " C" : "-- C");
    tft.drawCentreString(fl, 239, 172, 4);

    // pH Level Tile (x=5, y=206, w=152, h=54)
    tft.fillRect(7, 226, 148, 32, bgCard);
    char phBuf[16];
    if (incomingData.adsStatus == 1) dtostrf(incomingData.phValue, 4, 2, phBuf);
    else strcpy(phBuf, "--");
    tft.setTextColor(TFT_BLACK, bgCard);
    tft.drawCentreString(phBuf, 81, 230, 4);

    // S. Gravity Tile (x=163, y=206, w=152, h=108 - Taller tile, Font 4 value)
    tft.fillRect(165, 226, 148, 86, bgCard);
    char sgBuf[16];
    if (incomingData.pillGravity != 0 && incomingData.pillGravity < 10.0) dtostrf(incomingData.pillGravity, 5, 3, sgBuf);
    else strcpy(sgBuf, "--");

    tft.setTextColor(TFT_BLACK, bgCard);
    tft.drawCentreString(sgBuf, 239, 240, 4); // Same font 4 as Temperature & pH
    sprintf(buf, "%ddBm  %d%%", incomingData.pillRSSI, incomingData.pillBattery);
    tft.drawCentreString(buf, 239, 282, 1);

    // Status Tile (x=5, y=264, w=152, h=50 - Half width matching temp column)
    tft.fillRect(7, 282, 148, 30, bgCard);
    tft.setTextColor(TFT_BLACK, bgCard);
    if (stageTransferring && activeBrewStage == 1) {
      tft.drawCentreString("TRANSFERRING", 81, 286, 2);
    } else {
      tft.drawCentreString("FERMENTING", 81, 286, 2);
    }

  } else if (dashSelection == 2) { // PASTEURIZATION VIEW
    // Ambient Temp Tile (x=5, y=148, w=152, h=54)
    tft.fillRect(7, 168, 148, 32, bgCard);
    String pa = liquid1Status ? String(bme1.readTemperature(), 1) + " C" : "-- C";
    tft.setTextColor(TFT_BLACK, bgCard);
    tft.drawCentreString(pa, 81, 172, 4);

    // Liquid Temp Tile (x=163, y=148, w=152, h=54)
    tft.fillRect(165, 168, 148, 32, bgCard);
    String pt = (simTempOverride[2] > 0.0f) ? String(simTempOverride[2], 1) + " C" : (liquid1Status ? String(getPastTemp(), 1) + " C" : "-- C");
    tft.drawCentreString(pt, 239, 172, 4);

    // Status Tile (x=5, y=206, w=310, h=108)
    tft.fillRect(7, 226, 306, 86, bgCard);
    tft.setTextColor(TFT_BLACK, bgCard);
    if (stageTransferring && activeBrewStage == 2) {
      tft.drawCentreString("TRANSFERRING", CENTER_X, 240, 4);
    } else if (pastSterilized && isFanOn) {
      tft.drawCentreString("COOLING", CENTER_X, 240, 4);
      sprintf(buf, "(%d%% Power)", currentSpeedPercent);
      tft.drawCentreString(buf, CENTER_X, 278, 2);
    } else if (pastHolding) {
      uint32_t elapsed = (millis() - pastHoldStart) / 1000;
      uint32_t rem = (elapsed < 900) ? (900 - elapsed) : 0;
      tft.drawCentreString("HOLDING", CENTER_X, 240, 4);
      sprintf(buf, "(%02lu:%02lu Remaining)", rem / 60, rem % 60);
      tft.drawCentreString(buf, CENTER_X, 278, 2);
    } else if (currentHeatingPercent > 0) {
      tft.drawCentreString("HEATING", CENTER_X, 240, 4);
      sprintf(buf, "(%d%% Power)", currentHeatingPercent);
      tft.drawCentreString(buf, CENTER_X, 278, 2);
    } else {
      tft.drawCentreString("IDLE", CENTER_X, 252, 4);
    }
  }

  // Persistent Trend Graph Canvas
  updateDashboardGraph();
}

void drawDashboardLayout() {
  if (dashNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, TFT_NAVY);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("DASHBOARD", 10, 15, 4);
    tft.setTextPadding(0);

    // Header Status Info (Y=52 to 100)
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

    tft.drawFastHLine(0, 102, 320, TFT_DARKGREY);
    dashNeedsFullRedraw = false;
  }

  // ---- 1. PHASE SELECTOR HEADER TILE (Y: 104 - 144) ----
  const char *stageNames[] = {"PRE-HEATING", "FERMENTATION", "PASTEURIZATION"};
  uint16_t stageColors[] = {TFT_RED, TFT_ORANGE, 0x03E0};

  uint16_t viewColor = stageColors[dashSelection];
  tft.fillRect(5, 104, 310, 40, viewColor);
  tft.drawRect(5, 104, 310, 40, TFT_DARKGREY);

  // Left circle indicator
  if (dashSelection == activeBrewStage) {
    tft.fillCircle(20, 124, 5, TFT_WHITE);
  } else {
    tft.drawCircle(20, 124, 5, TFT_WHITE);
  }

  // Center Phase Title
  tft.setTextColor(TFT_WHITE, viewColor);
  tft.drawCentreString(stageNames[dashSelection], CENTER_X, 114, 4);

  // Right side > arrow
  tft.drawString(">", 295, 114, 4);

  // ---- 2. PARAMETER TILES (Y: 148 - 315) ----
  auto drawUnifiedTile = [](int x, int y, int w, int h, const char *title) {
    tft.fillRect(x, y, w, h, 0xD6BA); // Light Gray fill
    tft.drawRect(x, y, w, h, TFT_DARKGREY); // Dark Grey border
    tft.setTextColor(TFT_BLACK, 0xD6BA);
    tft.drawCentreString(title, x + (w / 2), y + 4, 2); // Centered tile header
  };

  if (dashSelection == 0) { // PRE-HEATING VIEW
    drawUnifiedTile(5, 148, 152, 65, "AMBIENT");
    drawUnifiedTile(163, 148, 152, 65, "LIQUID");
    drawUnifiedTile(5, 218, 310, 95, "STATUS");
  } else if (dashSelection == 1) { // FERMENTATION VIEW
    drawUnifiedTile(5, 148, 152, 65, "AMBIENT");
    drawUnifiedTile(163, 148, 152, 65, "LIQUID");
    drawUnifiedTile(5, 218, 152, 48, "pH LEVEL");
    drawUnifiedTile(163, 218, 152, 48, "ALCOHOL");
    drawUnifiedTile(5, 271, 310, 42, "STATUS");
  } else if (dashSelection == 2) { // PASTEURIZATION VIEW
    drawUnifiedTile(5, 148, 152, 65, "AMBIENT");
    drawUnifiedTile(163, 148, 152, 65, "LIQUID");
    drawUnifiedTile(5, 218, 310, 95, "STATUS");
  }

  // Graph Tile Container (25% of screen height)
  drawUnifiedTile(5, 318, 310, 112, "TEMPERATURE TREND");

  updateDashboardValues();

  // ---- 3. TEMPERATURE TREND GRAPH ----
  updateDashboardGraph();

  // ---- 4. FOOTER HINTS (Y: 434 - 480) ----
  tft.fillRect(0, 434, 320, 46, TFT_WHITE);
  tft.drawFastHLine(0, 434, 320, TFT_DARKGREY);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("RIGHT: NEXT PHASE   SELECT: OPTIONS", CENTER_X, 444, 1);
  tft.drawCentreString("RETURN: BACK", CENTER_X, 460, 1);
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
  const char *opt0 = (activeBrewStage >= 0) ? "VIEW ACTIVE BREW" : "NEW BREW";
  const char *options[] = {opt0, "SETTINGS", "SYSTEM CHECK", "SENSOR VALUES"};
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
      "FAN TEST",       "LIGHT INDICATORS", "RELAY TEST",     "MOTOR TEST",
      "PID CONTROL",    "SD CARD VERIFY",   "UART MONITOR",   "TRANSFER TEST",
      "RAPT PILL",      "PH & FERM TEMP",   "DISPENSER TEST"};

  auto drawTile = [&](int i, bool sel) {
    uint16_t color = sel ? 0x3566 : 0xD6BA;
    uint16_t txtColor = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 52 + (i * 34), 300, 30, color);
    tft.drawRect(10, 52 + (i * 34), 300, 30, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawCentreString(options[i], CENTER_X, 57 + (i * 34), 2);
  };

  if (systemCheckNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SYSTEM CHECK", 10, 15, 4);
    for (int i = 0; i < 11; i++)
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

  const char *title = (fanTestFanChoice == 0) ? "PRE-HEATING FANS" : "FERMENTATION FANS";
  tft.fillRect(0, 52, 320, 28, 0x4208);
  tft.setTextColor(TFT_WHITE, 0x4208);
  tft.drawCentreString(title, CENTER_X, 60, 2);

  // Temperature Monitor Tiles (Ambient & Liquid Temp)
  char ambBuf[16] = "--";
  char liqBuf[16] = "--";

  if (fanTestFanChoice == 0) {
    if (bme1Status) sprintf(ambBuf, "%.1f C", bme1.readTemperature());
    if (liquid2Status) sprintf(liqBuf, "%.1f C", getPreheatTemp());
  } else {
    if (incomingData.sensor2Status > 0) sprintf(ambBuf, "%.1f C", incomingData.room2Temp);
    if (incomingData.ds18Status == 1) sprintf(liqBuf, "%.1f C", getFermTemp());
  }

  // Ambient Temp Tile (Left)
  tft.fillRect(20, 88, 135, 75, 0x2124);
  tft.drawRect(20, 88, 135, 75, TFT_DARKGREY);
  tft.setTextColor(TFT_LIGHTGREY, 0x2124);
  tft.drawCentreString("AMBIENT TEMP", 87, 96, 1);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.drawCentreString(ambBuf, 87, 122, 4);

  // Liquid Temp Tile (Right)
  tft.fillRect(165, 88, 135, 75, 0x2124);
  tft.drawRect(165, 88, 135, 75, TFT_DARKGREY);
  tft.setTextColor(TFT_LIGHTGREY, 0x2124);
  tft.drawCentreString("LIQUID TEMP", 232, 96, 1);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.drawCentreString(liqBuf, 232, 122, 4);

  // Fan Running Status Panel
  bool activeFanOn = (currentFanMode == FAN_ON);
  uint16_t statusColor = activeFanOn ? 0x0400 : 0xF800;
  tft.fillRect(20, 173, 280, 120, statusColor);
  tft.drawRect(20, 173, 280, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, statusColor);
  tft.drawCentreString(activeFanOn ? "RUNNING" : "STOPPED", CENTER_X, 205, 4);
  tft.drawCentreString("SELECT TO TOGGLE", CENTER_X, 255, 2);

  // Fan Speed Control Panel
  tft.fillRect(20, 303, 280, 115, 0xD6BA);
  tft.drawRect(20, 303, 280, 115, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.drawCentreString("FAN SPEED", CENTER_X, 318, 2);
  char buf[16];
  sprintf(buf, "%d %%", fanTestSpeed);
  tft.drawCentreString(buf, CENTER_X, 350, 4);

  tft.fillRect(0, 434, 320, 46, TFT_WHITE);
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
    relayTestNeedsFullRedraw = false;
  }

  // Draw footer text dynamically based on mode
  tft.fillRect(0, 415, 320, 65, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (relayTestAuto) {
    tft.drawCentreString("AUTO-SEQUENCING  1000ms/CH", CENTER_X, 422, 2);
  } else {
    tft.drawCentreString("UP/DOWN: NAV   SELECT: TOGGLE", CENTER_X, 422, 2);
  }
  tft.drawCentreString("RETURN: BACK TO MENU", CENTER_X, 452, 1);

  const char *labels[9] = {"FERM FAN", "PUMP 1",  "PUMP 2",  "CH4",    "CH5",
                           "LIGHT G",  "LIGHT Y", "LIGHT R", "PRE FAN"};

  // Draw Relays (indices 0 to 8)
  for (int i = 0; i < 9; i++) {
    int y = 60 + (i * 38);
    bool selected = (relayTestSelection == i);
    uint16_t color = selected ? 0x3566 : 0xD6BA;
    uint16_t txtColor = selected ? TFT_WHITE : TFT_BLACK;
    uint16_t ledColor = testRelayStates[i] ? TFT_GREEN : TFT_DARKGREY;

    tft.fillRect(15, y, 290, 34, color);
    tft.drawRect(15, y, 290, 34, TFT_DARKGREY);
    tft.setTextColor(txtColor, color);
    tft.drawString(labels[i], 30, y + 9, 2);
    tft.fillCircle(280, y + 17, 8, ledColor);
    tft.drawCircle(280, y + 17, 8, TFT_BLACK);
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
  tft.fillRect(20, 70, 280, 140, speedColor);
  tft.drawRect(20, 70, 280, 140, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, speedColor);
  tft.drawCentreString("SPEED", CENTER_X, 90, 2);
  char buf[16];
  sprintf(buf, "%d%%", motorTestSpeed);
  tft.drawCentreString(buf, CENTER_X, 125, 7);

  // ON/OFF tile
  uint16_t onOffColor = motorTestOn ? 0x0400 : 0x4208;
  tft.fillRect(20, 230, 280, 140, onOffColor);
  tft.drawRect(20, 230, 280, 140, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, onOffColor);
  tft.drawCentreString("MOTOR", CENTER_X, 245, 2);
  tft.drawCentreString(motorTestOn ? "ON" : "OFF", CENTER_X, 280, 4);
}

void drawDispenserTestMenu() {
  static int prevSel = -1;
  static bool prevEdit = false;

  auto drawRowTile = [&](int row, bool sel, bool edit) {
    int y = 75 + (row * 165);
    int w = 280;
    int h = 140;
    int x = 20;

    uint16_t bg = sel ? (edit ? 0x03E0 : 0x3566) : 0xD6BA;
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;

    if (row == 0) {
      bg = dispenserTestOn ? (sel ? 0x0400 : 0x03E0) : (sel ? 0xF800 : 0x4208);
      fg = TFT_WHITE;
    }

    tft.fillRect(x, y, w, h, bg);
    tft.setTextColor(fg, bg);

    if (sel && edit) {
      tft.drawRect(x, y, w, h, TFT_WHITE);
      tft.drawRect(x + 1, y + 1, w - 2, h - 2, TFT_WHITE);
    } else if (sel) {
      tft.drawRect(x, y, w, h, TFT_WHITE);
    } else {
      tft.drawRect(x, y, w, h, TFT_DARKGREY);
    }

    char valBuf[32];
    if (row == 0) {
      tft.drawCentreString("DISPENSER POWER", CENTER_X, y + 25, 2);
      tft.drawCentreString(dispenserTestOn ? "ON" : "OFF", CENTER_X, y + 65, 4);
    } else if (row == 1) {
      tft.drawCentreString("DISPENSER SPEED", CENTER_X, y + 25, 2);
      sprintf(valBuf, "%d%%", dispenserTestSpeed);
      tft.drawCentreString(valBuf, CENTER_X, y + 65, 4);
    }
  };

  if (dispenserTestNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("DISPENSER TEST", 10, 15, 4);

    for (int i = 0; i < 2; i++) {
      drawRowTile(i, dispenserTestSelection == i, (dispenserTestSelection == i) && dispenserTestEditing);
    }

    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    tft.drawCentreString("UP/DOWN: NAV/ADJUST   SELECT: TOGGLE/EDIT", CENTER_X, 447, 1);
    tft.drawCentreString("RETURN: BACK", CENTER_X, 462, 1);

    dispenserTestNeedsFullRedraw = false;
    prevSel = dispenserTestSelection;
    prevEdit = dispenserTestEditing;
  } else {
    for (int i = 0; i < 2; i++) {
      drawRowTile(i, dispenserTestSelection == i, (dispenserTestSelection == i) && dispenserTestEditing);
    }
    prevSel = dispenserTestSelection;
    prevEdit = dispenserTestEditing;
  }
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

  if (currentMixerMode == MIXER_MANUAL)
    tft.drawCentreString("(UP/DOWN TO ADJ)", CENTER_X, 305, 1);
  else
    tft.drawCentreString("AUTO CONTROLLED", CENTER_X, 305, 1);

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
  char buf[40];

  if (rtcSetNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SET RTC DATE & TIME", 10, 15, 4);
    rtcSetNeedsFullRedraw = false;
  }

  // Current RTC reading
  tft.fillRect(0, 54, 320, 24, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  if (rtcStatus) {
    DateTime now = rtc.now();
    sprintf(buf, "CURRENT: %04d/%02d/%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
  } else {
    strcpy(buf, "RTC: NOT DETECTED");
  }
  tft.drawCentreString(buf, CENTER_X, 58, 2);

  // --- Row 1: DATE FIELDS (Y=82 to 182) ---
  // Year field (Field 0)
  uint16_t yBg = (rtcSetField == 0) ? 0x3566 : 0xD6BA;
  uint16_t yFg = (rtcSetField == 0) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(10, 82, 95, 100, yBg);
  tft.drawRect(10, 82, 95, 100, TFT_DARKGREY);
  tft.setTextColor(yFg, yBg);
  tft.drawCentreString("YEAR", 57, 90, 2);
  sprintf(buf, "%04d", rtcSetYear);
  tft.drawCentreString(buf, 57, 120, 4);

  // Month field (Field 1)
  uint16_t moBg = (rtcSetField == 1) ? 0x3566 : 0xD6BA;
  uint16_t moFg = (rtcSetField == 1) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(112, 82, 95, 100, moBg);
  tft.drawRect(112, 82, 95, 100, TFT_DARKGREY);
  tft.setTextColor(moFg, moBg);
  tft.drawCentreString("MONTH", 159, 90, 2);
  sprintf(buf, "%02d", rtcSetMonth);
  tft.drawCentreString(buf, 159, 120, 4);

  // Day field (Field 2)
  uint16_t dBg = (rtcSetField == 2) ? 0x3566 : 0xD6BA;
  uint16_t dFg = (rtcSetField == 2) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(215, 82, 95, 100, dBg);
  tft.drawRect(215, 82, 95, 100, TFT_DARKGREY);
  tft.setTextColor(dFg, dBg);
  tft.drawCentreString("DAY", 262, 90, 2);
  sprintf(buf, "%02d", rtcSetDay);
  tft.drawCentreString(buf, 262, 120, 4);

  // --- Row 2: TIME FIELDS (Y=192 to 292) ---
  // Hour field (Field 3)
  uint16_t hBg = (rtcSetField == 3) ? 0x3566 : 0xD6BA;
  uint16_t hFg = (rtcSetField == 3) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(15, 192, 130, 100, hBg);
  tft.drawRect(15, 192, 130, 100, TFT_DARKGREY);
  tft.setTextColor(hFg, hBg);
  tft.drawCentreString("HOUR", 80, 200, 2);
  sprintf(buf, "%02d", rtcSetHour);
  tft.drawCentreString(buf, 80, 230, 4);

  // Colon separator
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillRect(147, 210, 26, 60, TFT_WHITE);
  tft.drawCentreString(":", CENTER_X, 230, 4);

  // Minute field (Field 4)
  uint16_t mBg = (rtcSetField == 4) ? 0x3566 : 0xD6BA;
  uint16_t mFg = (rtcSetField == 4) ? TFT_WHITE : TFT_BLACK;
  tft.fillRect(175, 192, 130, 100, mBg);
  tft.drawRect(175, 192, 130, 100, TFT_DARKGREY);
  tft.setTextColor(mFg, mBg);
  tft.drawCentreString("MIN", 240, 200, 2);
  sprintf(buf, "%02d", rtcSetMinute);
  tft.drawCentreString(buf, 240, 230, 4);

  // Field 5: SAVE & EXIT button
  uint16_t saveBg = (rtcSetField == 5) ? 0x03E0 : (rtcStatus ? 0x0400 : TFT_DARKGREY);
  tft.fillRect(10, 302, 142, 55, saveBg);
  tft.drawRect(10, 302, 142, 55, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, saveBg);
  tft.drawCentreString("SAVE & EXIT", 81, 318, 2);

  // Field 6: CANCEL button
  uint16_t cancelBg = (rtcSetField == 6) ? 0xF800 : 0x4208;
  tft.fillRect(168, 302, 142, 55, cancelBg);
  tft.drawRect(168, 302, 142, 55, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, cancelBg);
  tft.drawCentreString("CANCEL", 239, 318, 2);

  // Footer
  tft.fillRect(0, 434, 320, 46, TFT_WHITE);
  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawCentreString("UP/DN: SELECT FIELD   L/R: ADJUST (+/-)", CENTER_X, 447, 1);
  tft.drawCentreString("SELECT: CONFIRM / SAVE   RETURN: BACK", CENTER_X, 462, 1);
}

void drawSettingsMenu() {
  static int prevSel = -1;

  auto drawTile = [&](int i, bool sel) {
    bool isEditingThis = sel && settingsEditing;
    uint16_t bg = isEditingThis ? 0x03E0 : (sel ? 0x3566 : 0xD6BA);
    uint16_t fg = sel ? TFT_WHITE : TFT_BLACK;
    tft.fillRect(10, 65 + (i * 85), 300, 70, bg);

    if (isEditingThis) {
      tft.drawRect(10, 65 + (i * 85), 300, 70, TFT_WHITE);
      tft.drawRect(11, 66 + (i * 85), 298, 68, TFT_WHITE);
    } else if (sel) {
      tft.drawRect(10, 65 + (i * 85), 300, 70, TFT_WHITE);
    } else {
      tft.drawRect(10, 65 + (i * 85), 300, 70, TFT_DARKGREY);
    }

    tft.setTextColor(fg, bg);

    char buf[48];
    if (i == 0) {
      if (isEditingThis)
        sprintf(buf, "MIN VOLUME: < %.1f L >", minVolumeReq);
      else
        sprintf(buf, "MIN VOLUME: %.1f L", minVolumeReq);
    } else if (i == 1) {
      if (isEditingThis)
        sprintf(buf, "FERM FAN BASELINE: < %d%% >", pidFanPercent > 0 ? pidFanPercent : 20);
      else
        sprintf(buf, "FERM FAN BASELINE: %d%%", pidFanPercent > 0 ? pidFanPercent : 20);
    } else if (i == 2) {
      strcpy(buf, "SET RTC DATE & TIME");
    } else {
      strcpy(buf, "TARE LOAD CELL");
    }
    tft.drawCentreString(buf, CENTER_X, 80 + (i * 85), 2);

    if (i == 0 || i == 1) {
      if (isEditingThis) {
        tft.drawCentreString("[ EDITING - UP/DN/L/R TO ADJUST ]",
                             CENTER_X, 107 + (i * 85), 1);
      } else if (sel) {
        tft.drawCentreString("[ PRESS SELECT TO EDIT ]",
                             CENTER_X, 107 + (i * 85), 1);
      } else {
        tft.drawCentreString("[ ADJUSTABLE VALUE ]",
                             CENTER_X, 107 + (i * 85), 1);
      }
    } else {
      if (sel) {
        tft.drawCentreString("[ PRESS SELECT TO OPEN ]",
                             CENTER_X, 107 + (i * 85), 1);
      } else {
        tft.drawCentreString("[ MENU ACTION ]",
                             CENTER_X, 107 + (i * 85), 1);
      }
    }
  };

  if (settingsNeedsFullRedraw) {
    tft.fillRect(0, 0, 320, 50, 0x03E0);
    tft.fillRect(0, 50, 320, 430, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("SETTINGS", 10, 15, 4);
    for (int i = 0; i < 4; i++) {
      drawTile(i, settingsSelection == i);
    }
    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    if (settingsEditing) {
      tft.drawCentreString("UP/DN/L/R: ADJUST VALUE   SELECT: SAVE",
                           CENTER_X, 447, 1);
      tft.drawCentreString("RETURN (LEFT): BACK TO SETTINGS", CENTER_X, 462, 1);
    } else {
      tft.drawCentreString("UP/DN: NAVIGATE   SELECT: EDIT/OPEN",
                           CENTER_X, 447, 1);
      tft.drawCentreString("RETURN (LEFT): BACK TO MAIN MENU", CENTER_X, 462, 1);
    }
    settingsNeedsFullRedraw = false;
    prevSel = settingsSelection;
  } else {
    for (int i = 0; i < 4; i++) {
      drawTile(i, settingsSelection == i);
    }
  }
}

void updateDashboardGraph() {
  if (currentAppState != DASHBOARD_ACTIVE || moduleViewActive ||
      activeBrewStage < 0 || stageTransferring)
    return;

  const int GX = 32;
  const int GPY = 342;
  const int GW = 270;
  const int GH = 104;

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
    if (py < GPY) py = GPY;
    if (py > GPY + GH - 1) py = GPY + GH - 1;
    return py;
  };

  // Y-axis label strip (left of plot area inside tile)
  tft.fillRect(6, GPY, GX - 7, GH, 0xD6BA);
  tft.setTextColor(TFT_BLACK, 0xD6BA);
  tft.setTextPadding(0);
  char lbuf[8];
  for (int i = 0; i < 5; i++) {
    int ly = tempToY(tickTemps[i]);
    sprintf(lbuf, "%3.0f", tickTemps[i]);
    tft.drawRightString(lbuf, GX - 3, ly - 3, 1);
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

  // Border around plot area
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

    // Format log: e.g. "[1] SG:1.0090  31.2C  -73dBm  12:34"
    sprintf(buf, "[%d] SG:%.4f %.1fC %ddBm %s", i + 1,
            raptLogs[i].gravity, raptLogs[i].temp, raptLogs[i].rssi, raptLogs[i].timeStr);
    tft.drawString(buf, 20, y + 6, 2);
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

    // Static Metrics Container (Y=90 to 180)
    tft.drawRect(10, 90, 300, 90, TFT_DARKGREY);

    // Graph Area Border & Canvas Setup (Y=188 to 428)
    tft.fillRect(15, 188, 290, 240, 0x2124); // Dark panel
    tft.drawRect(15, 188, 290, 240, TFT_DARKGREY);

    // Graph Title & Active Log File
    tft.setTextColor(TFT_LIGHTGREY, 0x2124);
    tft.drawString("TEMP TREND (AUTO-RANGE)", 22, 192, 1);
    if (pidLogFileName[0] != '\0') {
      tft.drawRightString(pidLogFileName, 298, 192, 1);
    }

    // Footer
    tft.fillRect(0, 434, 320, 46, TFT_WHITE);
    tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
    if (!pidTrackRunning) {
      tft.drawCentreString("UP/DN: TSET   RIGHT: HEATER   SELECT: START   RETURN: BACK", CENTER_X, 458, 1);
    } else {
      tft.drawCentreString("SELECT: STOP TEST   RETURN: BACK", CENTER_X, 458, 1);
    }

    pidTrackNeedsFullRedraw = false;
  }

  // --- Dynamic Value Updates ---
  // Status Bar (Y=55 to 85) - Selectable Heater Output
  const char *hName = "NOT SET";
  if (pidTestChoice == 0) hName = "PRE-HEAT";
  else if (pidTestChoice == 1) hName = "FERMENTATION";
  else if (pidTestChoice == 2) hName = "PASTEURIZATION";

  uint16_t statBg = pidTrackRunning ? 0x0400 : (pidTestChoice == -1 ? 0x9000 : 0x3566);
  tft.fillRect(10, 55, 300, 30, statBg);
  tft.drawRect(10, 55, 300, 30, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, statBg);

  if (pidTrackRunning) {
    uint32_t runSec = (millis() - pidTrackStartMs) / 1000;
    sprintf(buf, "[%s] %lus  TSET: %.1f C", (pidTestChoice==0?"PRE":(pidTestChoice==1?"FERM":"PAST")), (unsigned long)runSec, pidTrackTargetTemp);
  } else {
    sprintf(buf, "HEATER: %s", hName);
  }
  tft.drawCentreString(buf, CENTER_X, 62, 2);

  // Metrics Grid (Y=90 to 180)
  tft.fillRect(11, 91, 298, 88, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Current Temp / Setpoint based on selected heater chamber
  float curTemp = 0.0f;
  float fermLiqTemp = -999.0f;
  if (pidTestChoice == 0) {
    curTemp = liquid2Status ? getPreheatTemp() : 0.0f;
  } else if (pidTestChoice == 1) {
    // Ambient Temp is the only temperature tracked and plotted in the graph
    curTemp = (incomingData.sensor2Status > 0) ? incomingData.room2Temp : 0.0f;
    if (incomingData.ds18Status == 1 && incomingData.room2LiquidTemp > -100.0f) {
      fermLiqTemp = getFermTemp();
    }
  } else if (pidTestChoice == 2) {
    curTemp = liquid1Status ? getPastTemp() : 0.0f;
  }

  uint16_t stabColor = TFT_BLACK;
  if (strcmp(pidTrackMetrics.stabilityStr, "STABLE") == 0) stabColor = 0x0400;
  else if (strcmp(pidTrackMetrics.stabilityStr, "SETTLING") == 0) stabColor = 0xE7E0;
  else if (strcmp(pidTrackMetrics.stabilityStr, "UNSTABLE") == 0) stabColor = TFT_RED;

  if (pidTestChoice == 1) {
    // Fermentation Metrics Layout (Displays Ambient + Liquid Temp)
    sprintf(buf, "AMB: %.1f C / %.1f C", curTemp, pidTrackTargetTemp);
    tft.drawString(buf, 20, 96, 2);

    sprintf(buf, "ERR: %.2f C", pidTrackMetrics.steadyStateError);
    tft.drawRightString(buf, 300, 96, 2);

    if (fermLiqTemp > -999.0f)
      sprintf(buf, "LIQUID: %.1f C", fermLiqTemp);
    else
      strcpy(buf, "LIQUID: ---");
    tft.drawString(buf, 20, 122, 2);

    tft.setTextColor(stabColor, TFT_WHITE);
    sprintf(buf, "STABILITY: %s", pidTrackMetrics.stabilityStr);
    tft.drawRightString(buf, 300, 122, 2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);

    sprintf(buf, "POWER: %d%%", currentHeatingPercent);
    tft.drawString(buf, 20, 148, 2);

    sprintf(buf, "OVERSHOOT: +%.1f C", pidTrackMetrics.overshootDeg);
    tft.drawRightString(buf, 300, 148, 2);
  } else {
    // Pre-Heat & Pasteurization Metrics Layout
    sprintf(buf, "TEMP: %.1f C / %.1f C", curTemp, pidTrackTargetTemp);
    tft.drawString(buf, 20, 96, 2);

    sprintf(buf, "ERR: %.2f C", pidTrackMetrics.steadyStateError);
    tft.drawRightString(buf, 300, 96, 2);

    sprintf(buf, "POWER: %d%%", currentHeatingPercent);
    tft.drawString(buf, 20, 122, 2);

    tft.setTextColor(stabColor, TFT_WHITE);
    sprintf(buf, "STABILITY: %s", pidTrackMetrics.stabilityStr);
    tft.drawRightString(buf, 300, 122, 2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);

    sprintf(buf, "OVERSHOOT: +%.1f C", pidTrackMetrics.overshootDeg);
    tft.drawString(buf, 20, 148, 2);

    sprintf(buf, "TARGET: %s", (pidTestChoice == 0 ? "PRE-HEAT" : "PAST"));
    tft.drawRightString(buf, 300, 148, 2);
  }

  // --- Professional Dynamic Auto-Ranging Line Graph ---
  const int gX = 40;     // Graph canvas left offset (leaving 40px for Y-axis labels)
  const int gY = 205;    // Graph canvas top
  const int gW = 265;    // Graph width
  const int gH = 215;    // Graph height
  const int gBottom = gY + gH; // Y = 420

  // 1. Calculate Dynamic Temperature Min/Max Range from starting temp & setpoint
  float startT = (pidTrackMetrics.startTemp > 0.0f) ? pidTrackMetrics.startTemp : ((curTemp > 0.0f) ? curTemp : pidTrackTargetTemp);
  float baseLow = min(startT, pidTrackTargetTemp);
  float baseHigh = max(startT, pidTrackTargetTemp);

  float yMin = baseLow - 5.0f;
  float yMax = baseHigh + 5.0f;

  // Auto-expand range if temperature drops below yMin or exceeds yMax
  if (curTemp > 0.0f) {
    if (curTemp < yMin) yMin = floor(curTemp);
    if (curTemp > yMax) yMax = ceil(curTemp);
  }

  for (int i = 0; i < pidTrackHistoryCount; i++) {
    if (pidTrackHistory[i] > 0.0f) {
      if (pidTrackHistory[i] < yMin) yMin = floor(pidTrackHistory[i]);
      if (pidTrackHistory[i] > yMax) yMax = ceil(pidTrackHistory[i]);
    }
  }

  if (yMin < 0.0f) yMin = 0.0f;
  float yRange = yMax - yMin;
  if (yRange < 1.0f) yRange = 1.0f;

  // 2. Draw Graph Background & Frame
  tft.fillRect(gX, gY, gW, gH, 0x10A2); // Sleek dark slate canvas
  tft.drawRect(gX - 1, gY - 1, gW + 2, gH + 2, TFT_DARKGREY);

  // 3. Draw Dynamic Y-Axis Ticks & Temperature Labels
  tft.drawLine(gX - 1, gY, gX - 1, gBottom, TFT_WHITE); // Y-axis solid line
  tft.setTextColor(TFT_LIGHTGREY, 0x2124);
  tft.setTextDatum(MR_DATUM); // Middle-Right text alignment for Y axis labels

  for (int step = 0; step <= 4; step++) {
    float val = yMin + (yRange * step / 4.0f);
    int yPos = gBottom - (int)((val - yMin) * (gH - 10) / yRange) - 5;
    tft.drawFastHLine(gX, yPos, gW, 0x2965); // Horizontal gridline
    tft.drawFastHLine(gX - 4, yPos, 4, TFT_WHITE); // Tick mark on Y axis
    if (yRange <= 20.0f) {
      sprintf(buf, "%.1f", val);
    } else {
      sprintf(buf, "%.0f", val);
    }
    tft.drawString(buf, gX - 6, yPos, 1);
  }

  // 4. Draw Dynamic X-Axis (Time Scaling: 0s to actual elapsed duration)
  tft.drawLine(gX - 1, gBottom + 1, gX + gW, gBottom + 1, TFT_WHITE); // X-axis solid line
  tft.setTextDatum(TC_DATUM); // Top-Center text alignment for X axis labels

  uint32_t totalSec = pidTrackRunning ? ((millis() - pidTrackStartMs) / 1000) : 0;
  uint32_t sampleCountSec = (uint32_t)pidTrackHistoryCount * pidTrackSampleIntervalSec; // Dynamic sample interval
  uint32_t xSpanSec = max(totalSec, sampleCountSec);
  if (xSpanSec < 30) xSpanSec = 30; // Minimum 30s initial X scale

  for (int xPct = 0; xPct <= 100; xPct += 25) {
    int xPos = gX + (int)(xPct * gW / 100.0f);
    tft.drawFastVLine(xPos, gY, gH, 0x2965); // Vertical gridline
    tft.drawFastVLine(xPos, gBottom + 1, 4, TFT_WHITE); // Tick mark on X axis
    uint32_t secLabel = (xSpanSec * xPct) / 100;
    sprintf(buf, "%lus", (unsigned long)secLabel);
    tft.drawString(buf, xPos, gBottom + 5, 1);
  }

  tft.setTextDatum(TL_DATUM); // Reset text datum to Top-Left default

  // 5. Target Setpoint Reference Line (Dashed Red Line)
  int targetY = gBottom - (int)((pidTrackTargetTemp - yMin) * (gH - 10) / yRange) - 5;
  targetY = constrain(targetY, gY + 5, gBottom - 5);

  for (int dx = gX; dx < gX + gW; dx += 8) {
    tft.drawFastHLine(dx, targetY, 4, TFT_RED);
  }
  tft.setTextColor(TFT_RED, 0x10A2);
  sprintf(buf, "SET: %.0fC", pidTrackTargetTemp);
  int txtY = (targetY - 10 < gY + 2) ? targetY + 2 : targetY - 10;
  tft.drawString(buf, gX + gW - 65, txtY, 1);

  // 6. Plot Dynamic Temperature Curve across canvas
  if (pidTrackHistoryCount > 1) {
    int count = pidTrackHistoryCount;
    float xStep = (float)gW / max(1, count - 1);

    for (int i = 0; i < count - 1; i++) {
      float t1 = constrain(pidTrackHistory[i], yMin, yMax);
      float t2 = constrain(pidTrackHistory[i + 1], yMin, yMax);

      int y1 = gBottom - (int)((t1 - yMin) * (gH - 10) / yRange) - 5;
      int y2 = gBottom - (int)((t2 - yMin) * (gH - 10) / yRange) - 5;

      int x1 = gX + (int)(i * xStep);
      int x2 = gX + (int)((i + 1) * xStep);

      // Draw double-thick bright cyan line for high clarity
      tft.drawLine(x1, y1, x2, y2, 0x07FF);
      tft.drawLine(x1, y1 + 1, x2, y2 + 1, 0x07FF);
    }
  }
}

