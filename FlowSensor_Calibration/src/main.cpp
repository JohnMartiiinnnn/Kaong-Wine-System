#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <TFT_eSPI.h>

#define FLOW_SENSOR_1 32
#define FLOW_SENSOR_2 34

// MCP23017 Relays
#define PUMP_PREHEAT_FERM 9   // GPB1 - transfer pump 1
#define PUMP_FERM_PAST 10     // GPB2 - transfer pump 2

#define RELAY_ON LOW
#define RELAY_OFF HIGH

// MCP23017 Keypad
#define BTN_RIGHT 0
#define BTN_LEFT 1
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_SELECT 4

Adafruit_MCP23X17 mcp;
TFT_eSPI tft = TFT_eSPI();

volatile uint32_t flow1Pulses = 0;
volatile uint32_t flow2Pulses = 0;

bool pump1State = false;
bool pump2State = false;
bool mcpStatus = false;

// Menu variables
int menuIndex = 0;
float targetVol = 1.0f; // Liters
float calculatedKF1 = 0.0f;
float calculatedKF2 = 0.0f;
bool kFactorCalculated = false;

uint32_t lastDisplayUpdate = 0;
uint32_t lastPress = 0;

void IRAM_ATTR flowISR1() {
  flow1Pulses++;
}

void IRAM_ATTR flowISR2() {
  flow2Pulses++;
}

void drawUI() {
  // Header
  tft.fillRect(0, 0, 320, 60, 0x18C3); // dark blue header
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("FLOW CALIBRATION", 160, 10, 4);
  tft.drawCentreString("Use keypad or Serial to control", 160, 38, 2);

  // Menu items (y starts at 75)
  int yStart = 75;
  int yGap = 40;

  for (int i = 0; i < 5; i++) {
    int y = yStart + i * yGap;
    bool isSelected = (menuIndex == i);

    uint16_t bg = isSelected ? 0x2A6B : 0x0841; // Selected vs non-selected color
    uint16_t border = isSelected ? TFT_CYAN : TFT_DARKGREY;
    tft.fillRect(10, y, 300, 32, bg);
    tft.drawRect(10, y, 300, 32, border);

    tft.setTextColor(isSelected ? TFT_WHITE : TFT_LIGHTGREY);

    char label[48];
    if (i == 0) {
      sprintf(label, "Pump 1 (PH->Ferm): %s", pump1State ? "ON" : "OFF");
    } else if (i == 1) {
      sprintf(label, "Pump 2 (Ferm->Past): %s", pump2State ? "ON" : "OFF");
    } else if (i == 2) {
      sprintf(label, "Target Volume: %.1f L", targetVol);
    } else if (i == 3) {
      strcpy(label, "Reset Counters to 0");
    } else if (i == 4) {
      strcpy(label, "Calculate K-Factor");
    }

    tft.drawString(label, 20, y + 8, 2);
    if (isSelected) {
      tft.fillCircle(290, y + 16, 4, TFT_CYAN);
    }
  }

  // Live Pulse Count Card
  tft.fillRect(10, 280, 300, 155, 0x0821); // deep blue-black card
  tft.drawRect(10, 280, 300, 155, TFT_DARKGREY);

  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("LIVE READINGS & CALIBRATION", 160, 290, 2);

  tft.setTextColor(TFT_WHITE);
  char buf[64];
  sprintf(buf, "Flow 1 Pulses: %u", flow1Pulses);
  tft.drawString(buf, 20, 315, 2);
  
  sprintf(buf, "Flow 2 Pulses: %u", flow2Pulses);
  tft.drawString(buf, 20, 340, 2);

  tft.drawFastHLine(20, 370, 280, TFT_DARKGREY);

  if (kFactorCalculated) {
    tft.setTextColor(TFT_GREEN);
    sprintf(buf, "K-Factor 1: %.2f", calculatedKF1);
    tft.drawString(buf, 20, 385, 2);

    sprintf(buf, "K-Factor 2: %.2f", calculatedKF2);
    tft.drawString(buf, 20, 410, 2);
  } else {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("K-Factor: Run 'Calculate' to see", 20, 395, 2);
  }

  // Footer hint
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawCentreString("UP/D: Move | L/R: Adjust | SEL: Trigger", 160, 448, 1);
}

void executeReset() {
  flow1Pulses = 0;
  flow2Pulses = 0;
  calculatedKF1 = 0.0f;
  calculatedKF2 = 0.0f;
  kFactorCalculated = false;
  Serial.println("\n[Flow pulses reset to zero]");
}

void executeCalculation() {
  if (targetVol <= 0.0f) {
    Serial.println("Aborting: Invalid volume input.");
    return;
  }
  calculatedKF1 = (float)flow1Pulses / targetVol;
  calculatedKF2 = (float)flow2Pulses / targetVol;
  kFactorCalculated = true;

  Serial.println("\n[K-Factor Calculation]");
  Serial.printf("Target Vol: %.2f L\n", targetVol);
  Serial.printf("Flow 1 Pulses: %u -> Calculated K-Factor: %.2f\n", flow1Pulses, calculatedKF1);
  Serial.printf("Flow 2 Pulses: %u -> Calculated K-Factor: %.2f\n", flow2Pulses, calculatedKF2);
  Serial.println("Update FLOW1_KF and FLOW2_KF in src/config.h with these values.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE FLOW SENSOR CALIBRATION + TFT");
  Serial.println("=============================================");

  // Initialize TFT Screen
  tft.init();
  tft.setRotation(0); // Portrait
  tft.fillScreen(TFT_BLACK);

  // Initialize Flow Sensors
  pinMode(FLOW_SENSOR_1, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1), flowISR1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2), flowISR2, RISING);
  Serial.println("Flow Sensors Interrupts Attached.");

  // Initialize MCP23017
  Wire.begin(21, 22);
  if (mcp.begin_I2C(0x20)) {
    mcp.pinMode(PUMP_PREHEAT_FERM, OUTPUT);
    mcp.pinMode(PUMP_FERM_PAST, OUTPUT);
    mcp.digitalWrite(PUMP_PREHEAT_FERM, RELAY_OFF);
    mcp.digitalWrite(PUMP_FERM_PAST, RELAY_OFF);

    mcp.pinMode(BTN_UP, INPUT_PULLUP);
    mcp.pinMode(BTN_DOWN, INPUT_PULLUP);
    mcp.pinMode(BTN_LEFT, INPUT_PULLUP);
    mcp.pinMode(BTN_RIGHT, INPUT_PULLUP);
    mcp.pinMode(BTN_SELECT, INPUT_PULLUP);

    mcpStatus = true;
    Serial.println("[OK] MCP23017 initialized. Pumps configured (OFF).");
  } else {
    Serial.println("[ERR] MCP23017 not detected at 0x20! Control disabled.");
  }

  drawUI();
}

void loop() {
  // 1. Refresh TFT Display periodically (every 1 second)
  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    drawUI();
  }

  // 2. Read Serial Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    while (Serial.available() > 0) Serial.read();

    if (cmd == '1' && mcpStatus) {
      pump1State = !pump1State;
      mcp.digitalWrite(PUMP_PREHEAT_FERM, pump1State ? RELAY_ON : RELAY_OFF);
      drawUI();
    }
    else if (cmd == '2' && mcpStatus) {
      pump2State = !pump2State;
      mcp.digitalWrite(PUMP_FERM_PAST, pump2State ? RELAY_ON : RELAY_OFF);
      drawUI();
    }
    else if (cmd == 'r' || cmd == 'R') {
      executeReset();
      drawUI();
    }
    else if (cmd == 'c' || cmd == 'C') {
      executeCalculation();
      drawUI();
    }
  }

  // 3. Read Keypad Buttons (Lockout Debounced)
  if (mcpStatus && (millis() - lastPress > 200)) {
    bool up = (mcp.digitalRead(BTN_UP) == LOW);
    bool down = (mcp.digitalRead(BTN_DOWN) == LOW);
    bool left = (mcp.digitalRead(BTN_LEFT) == LOW);
    bool right = (mcp.digitalRead(BTN_RIGHT) == LOW);
    bool select = (mcp.digitalRead(BTN_SELECT) == LOW);

    if (up) {
      menuIndex = (menuIndex - 1 + 5) % 5;
      lastPress = millis();
      drawUI();
    } else if (down) {
      menuIndex = (menuIndex + 1) % 5;
      lastPress = millis();
      drawUI();
    } else if (left) {
      if (menuIndex == 2) {
        targetVol = max(0.1f, targetVol - 0.1f);
        lastPress = millis();
        drawUI();
      }
    } else if (right) {
      if (menuIndex == 2) {
        targetVol = targetVol + 0.1f;
        lastPress = millis();
        drawUI();
      }
    } else if (select) {
      if (menuIndex == 0) {
        pump1State = !pump1State;
        mcp.digitalWrite(PUMP_PREHEAT_FERM, pump1State ? RELAY_ON : RELAY_OFF);
      } else if (menuIndex == 1) {
        pump2State = !pump2State;
        mcp.digitalWrite(PUMP_FERM_PAST, pump2State ? RELAY_ON : RELAY_OFF);
      } else if (menuIndex == 3) {
        executeReset();
      } else if (menuIndex == 4) {
        executeCalculation();
      }
      lastPress = millis();
      drawUI();
    }
  }

  delay(10);
}
