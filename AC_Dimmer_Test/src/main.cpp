#include <Adafruit_MCP23X17.h>
#include <Arduino.h>
#include <RBDdimmer.h>
#include <TFT_eSPI.h>
#include <Wire.h>

// Pins based on main configuration
#define AC_ZC_PIN 32
#define DIM2_SHARED 13
#define DIM1_CH2 12
#define DIM1_CH1 14

#define BTN_RIGHT_PIN 0
#define BTN_LEFT_PIN 1
#define BTN_UP_PIN 2
#define BTN_DOWN_PIN 3
#define BTN_SELECT_PIN 4

dimmerLamp dimmerPreheat(DIM2_SHARED, AC_ZC_PIN);
dimmerLamp dimmerFerm(DIM1_CH2, AC_ZC_PIN);
dimmerLamp dimmerPasteur(DIM1_CH1, AC_ZC_PIN);

Adafruit_MCP23X17 mcp;
TFT_eSPI tft = TFT_eSPI();

int powerLevel = 0;
int currentChannel = 0;
const char *channelNames[3] = {"Preheat (CH3/13)", "Ferm (CH2/12)",
                               "Pasteur (CH1/14)"};
bool needsRedraw = true;
bool directBypass = false; // Bypass mode flag

bool lastRight = false, lastLeft = false, lastUp = false, lastDown = false,
     lastSelect = false;

void drawUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AC Dimmer Test", 160, 20, 4);

  for (int i = 0; i < 3; i++) {
    int y = 100 + i * 100;
    if (i == currentChannel) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("->", 20, y, 4);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    tft.drawString(channelNames[i], 160, y - 20, 2);
  }

  // Status
  if (directBypass) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("HARDWARE BYPASS: 100% ON", 160, 360, 4);
  } else {
    char buf[32];
    sprintf(buf, "Power: %d%%", powerLevel);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(buf, 160, 360, 4);
  }

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Up/Down: +/- 10% | Left/Right: Ch", 160, 420, 2);
  tft.drawString("Select: Toggle 100% Bypass", 160, 450, 2);
  needsRedraw = false;
}

void setup() {
  Serial.begin(115200);

  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH); // TFT CS
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH); // Touch CS
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH); // SD CS
  pinMode(19, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  Wire.begin();
  mcp.begin_I2C();
  mcp.pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  mcp.pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  mcp.pinMode(BTN_UP_PIN, INPUT_PULLUP);
  mcp.pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  mcp.pinMode(BTN_SELECT_PIN, INPUT_PULLUP);

  dimmerPreheat.begin(NORMAL_MODE, ON);
  dimmerFerm.begin(NORMAL_MODE, ON);
  dimmerPasteur.begin(NORMAL_MODE, ON);
  dimmerPreheat.setPower(0);
  dimmerFerm.setPower(0);
  dimmerPasteur.setPower(0);

  drawUI();
}

void applyPower() {
  // Reset all
  dimmerPreheat.setPower(0);
  dimmerFerm.setPower(0);
  dimmerPasteur.setPower(0);
  digitalWrite(DIM2_SHARED, LOW);
  digitalWrite(DIM1_CH2, LOW);
  digitalWrite(DIM1_CH1, LOW);

  if (directBypass) {
    // Disable library control and force pin HIGH (100% power)
    dimmerPreheat.setState(OFF);
    dimmerFerm.setState(OFF);
    dimmerPasteur.setState(OFF);
    pinMode(DIM2_SHARED, OUTPUT);
    pinMode(DIM1_CH2, OUTPUT);
    pinMode(DIM1_CH1, OUTPUT);

    if (currentChannel == 0)
      digitalWrite(DIM2_SHARED, HIGH);
    else if (currentChannel == 1)
      digitalWrite(DIM1_CH2, HIGH);
    else if (currentChannel == 2)
      digitalWrite(DIM1_CH1, HIGH);
  } else {
    // Restore library control
    dimmerPreheat.setState(ON);
    dimmerFerm.setState(ON);
    dimmerPasteur.setState(ON);

    if (currentChannel == 0)
      dimmerPreheat.setPower(powerLevel);
    else if (currentChannel == 1)
      dimmerFerm.setPower(powerLevel);
    else if (currentChannel == 2)
      dimmerPasteur.setPower(powerLevel);
  }
}

void loop() {
  bool rawRight = (mcp.digitalRead(BTN_RIGHT_PIN) == LOW);
  bool rawLeft = (mcp.digitalRead(BTN_LEFT_PIN) == LOW);
  bool rawUp = (mcp.digitalRead(BTN_UP_PIN) == LOW);
  bool rawDown = (mcp.digitalRead(BTN_DOWN_PIN) == LOW);
  bool rawSelect = (mcp.digitalRead(BTN_SELECT_PIN) == LOW);

  if (rawRight && !lastRight) {
    currentChannel = (currentChannel + 1) % 3;
    needsRedraw = true;
  }
  if (rawLeft && !lastLeft) {
    currentChannel = (currentChannel + 2) % 3;
    needsRedraw = true;
  }
  if (rawUp && !lastUp && !directBypass) {
    powerLevel += 10;
    if (powerLevel > 100)
      powerLevel = 100;
    needsRedraw = true;
  }
  if (rawDown && !lastDown && !directBypass) {
    powerLevel -= 10;
    if (powerLevel < 0)
      powerLevel = 0;
    needsRedraw = true;
  }
  if (rawSelect && !lastSelect) {
    directBypass = !directBypass;
    needsRedraw = true;
  }

  lastRight = rawRight;
  lastLeft = rawLeft;
  lastUp = rawUp;
  lastDown = rawDown;
  lastSelect = rawSelect;

  if (needsRedraw) {
    applyPower();
    drawUI();
  }
  delay(50);
}
