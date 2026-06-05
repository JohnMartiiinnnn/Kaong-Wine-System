#pragma once
#include "config.h"

void drawSplashScreen();
void drawReturnConfirmation();
void drawEstopPage();
void drawCoolingMenu();
void drawNewBrewWizard();
void drawDashboardLayout();
void drawStartMenu();
void drawValueTile(int x, int y, const char *label, String value, bool isError);
void drawCalibrationValueTile(int y, const char *label, String value, bool isSelected);
void drawCalibrationPage();
void drawSensorMonitorPage();
void drawInitTile(int x, int y, const char *label, int status);
