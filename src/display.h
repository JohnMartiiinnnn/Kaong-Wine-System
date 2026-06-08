#pragma once
#include "config.h"

void drawSplashScreen();
void drawReturnConfirmation();
void drawEstopPage();
void drawCoolingMenu();
void drawMixerMenu();
void drawNewBrewWizard();
void drawDashboardLayout();
void drawStartMenu();
void drawSystemCheckMenu();
void drawFanTestPick();
void drawFanTestMenu();
void drawLightTestMenu();
void drawValueTile(int x, int y, const char *label, String value, bool isError);
void drawCalibrationValueTile(int y, const char *label, String value, bool isSelected);
void drawCalibrationPage(bool valuesOnly = false);
void drawSensorMonitorPage(bool valuesOnly = false);
void drawInitTile(int x, int y, const char *label, int status);
