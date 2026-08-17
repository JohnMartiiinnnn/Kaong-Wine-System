#pragma once
#include "config.h"

void drawSplashScreen();
void drawReturnConfirmation();
void drawCoolingMenu();
void drawMixerMenu();
void drawNewBrewWizard();
void drawDashboardLayout();
void drawDashboardHeaderInfo();
void updateDashboardValues();
void updateDashboardTimers();
void updateDashboardGraph();

void drawStartMenu();
void drawLoadCellPage(bool valuesOnly = false);
void drawSystemCheckMenu();
void drawFanTestPick();
void drawFanTestMenu();
void drawLightTestMenu();
void drawRelayTestPick();
void drawRelayTestMenu();
void drawMotorTestMenu();
void drawHeaterTestPick();
void drawHeaterTestMenu();
void drawSdVerifyMenu();
void drawUartMonitorMenu();
void drawRtcSetMenu();
void drawValueTile(int x, int y, const char *label, String value, bool isError);
void drawCalibrationValueTile(int y, const char *label, String value,
                              bool isSelected);
void drawCalibrationPage(bool valuesOnly = false);
void drawSensorMonitorPage(bool valuesOnly = false);
void drawInitTile(int x, int y, const char *label, int status);
void drawStageParamMenu();
void drawBrewSummaryMenu();
void drawTransferTestMenu(bool valuesOnly = false, bool tileOnly = false);
void drawFlowCalMenu(bool valuesOnly = false);
void drawCalibWizard();
void drawRaptTestPage(bool valuesOnly = false);
void drawPhFermMenu(bool valuesOnly = false);
void drawPidTrackingMenu(bool valuesOnly = false);
void drawDispenserTestMenu();
void drawSettingsMenu();
void drawGraphPickMenu();
void drawPidChamberPick();
void drawPidConfigMenu();
