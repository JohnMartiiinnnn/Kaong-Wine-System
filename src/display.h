#pragma once
#include "config.h"

void drawSplashScreen();
void drawReturnConfirmation();
void drawEstopPage();
void drawCoolingMenu();
void drawMixerMenu();
void drawNewBrewWizard();
void drawDashboardLayout();
void updateDashboardTimers();
void drawStartMenu();
void drawLoadCellPage(bool valuesOnly = false);
void drawSystemCheckMenu();
void drawFanTestPick();
void drawFanTestMenu();
void drawLightTestMenu();
void drawRelayTestMenu();
void drawMotorTestMenu();
void drawPidTestPick();
void drawPidTestMenu();
void drawHeaterTestMenu();
void drawSdVerifyMenu();
void drawUartMonitorMenu();
void drawRtcSetMenu();
void drawValueTile(int x, int y, const char *label, String value, bool isError);
void drawCalibrationValueTile(int y, const char *label, String value, bool isSelected);
void drawCalibrationPage(bool valuesOnly = false);
void drawSensorMonitorPage(bool valuesOnly = false);
void drawInitTile(int x, int y, const char *label, int status);
void drawStageParamMenu();
void drawBrewSummaryMenu();
