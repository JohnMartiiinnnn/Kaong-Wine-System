#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Adafruit_MCP23X17.h>

#define ONE_WIRE_BUS 26
#define RX2_PIN 16
#define TX2_PIN 17

// MCP Pins
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_SELECT 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
TFT_eSPI tft = TFT_eSPI();
Adafruit_MCP23X17 mcp;

// UART Telemetry Package (from Secondary)
typedef struct __attribute__((packed)) {
  uint32_t signature;
  float pillTemp;
  float pillGravity;
  float room2Temp;
  float room2Pres;
  float phValue;
  float room2LiquidTemp;
  uint8_t sensor2Status;
  uint8_t adsStatus;
  uint8_t ds18Status;
  uint8_t bleStatus;
  uint8_t pillBattery;
  int16_t pillRSSI;
  uint8_t checksum;
} struct_message;

struct_message rxData;
bool secondaryActive = false;
uint32_t lastUARTPacket = 0;

enum ProbeSelection {
  PROBE_PASTEURIZATION = 0,
  PROBE_PREHEAT = 1,
  PROBE_FERMENTATION = 2
};

ProbeSelection selectedProbe = PROBE_PASTEURIZATION;

uint32_t lastDisplayUpdate = 0;
uint32_t lastPress = 0;
float currentTemp = -127.0f;

const char* getProbeName(ProbeSelection p) {
  switch (p) {
    case PROBE_PASTEURIZATION: return "Pasteurization (Local 0)";
    case PROBE_PREHEAT:        return "Pre-heat (Local 1)";
    case PROBE_FERMENTATION:   return "Fermentation (Remote UART)";
    default:                   return "Unknown";
  }
}

uint8_t calculateChecksum(const struct_message &msg) {
  uint8_t checksum = 0;
  const uint8_t *ptr = (const uint8_t *)&msg;
  for (size_t i = 0; i < sizeof(struct_message) - 1; i++) {
    checksum ^= ptr[i];
  }
  return checksum;
}

void drawUI(float temp) {
  // Header
  tft.fillRect(0, 0, 320, 60, 0x18C3); // dark blue header
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("DS18B20 TEMP PROBES", 160, 10, 4);
  tft.drawCentreString("Select Probe to Monitor", 160, 38, 2);

  // Draw Probe List
  int yStart = 80;
  int yGap = 45;

  for (int i = 0; i < 3; i++) {
    int y = yStart + i * yGap;
    bool isSelected = ((int)selectedProbe == i);
    
    uint16_t bg = isSelected ? 0x2A6B : 0x0841; // Light blue vs dark gray-blue
    uint16_t border = isSelected ? TFT_CYAN : TFT_DARKGREY;
    
    tft.fillRect(10, y, 300, 36, bg);
    tft.drawRect(10, y, 300, 36, border);
    
    tft.setTextColor(isSelected ? TFT_WHITE : TFT_LIGHTGREY);
    tft.drawString(getProbeName((ProbeSelection)i), 20, y + 10, 2);

    if (isSelected) {
      tft.fillCircle(290, y + 18, 5, TFT_CYAN);
    }
  }

  // Giant value display area
  tft.fillRect(10, 230, 300, 200, 0x0821); // deep dark blue-black card
  tft.drawRect(10, 230, 300, 200, TFT_DARKGREY);
  
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawCentreString("CURRENT TEMPERATURE", 160, 245, 2);

  if (temp > -100.0f) {
    char buf[16];
    dtostrf(temp, 5, 2, buf);
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString(buf, 140, 290, 7); // Giant font
    tft.drawString("C", 240, 290, 4);      // Celsius unit
  } else {
    tft.setTextColor(TFT_RED);
    tft.drawCentreString("DISCONNECTED", 160, 310, 4);
    tft.drawCentreString("Check probe connection / wiring", 160, 350, 1);
  }

  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawCentreString("Press UP/DOWN on Keypad to switch", 160, 445, 1);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(1000);
  Serial.println("\n=== DS18B20 Temp Probe Display Verifier ===");

  // Initialize TFT Screen
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Initialize Sensors (Non-blocking)
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); // Prime first read
  int count = sensors.getDeviceCount();
  Serial.printf("Local DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, count);

  // Initialize MCP23017 Keypad
  Wire.begin(21, 22);
  if (mcp.begin_I2C(0x20)) {
    mcp.pinMode(BTN_UP, INPUT_PULLUP);
    mcp.pinMode(BTN_DOWN, INPUT_PULLUP);
    Serial.println("[OK] MCP23017 initialized.");
  } else {
    Serial.println("[ERR] MCP23017 not detected at 0x20!");
  }

  drawUI(-127.0f);
}

void loop() {
  // 1. Read UART packets from Secondary
  while (Serial2.available() > 0) {
    if (Serial2.peek() != 0xEF) {
      Serial2.read();
      continue;
    }
    if (Serial2.available() < (int)sizeof(struct_message)) {
      break;
    }
    struct_message t;
    Serial2.readBytes((uint8_t *)&t, sizeof(t));
    if (t.signature == 0xDEADBEEF && calculateChecksum(t) == t.checksum) {
      rxData = t;
      secondaryActive = true;
      lastUARTPacket = millis();
    }
  }

  if (secondaryActive && (millis() - lastUARTPacket > 3000)) {
    secondaryActive = false;
  }

  // 2. Read temperature values (Non-blocking: reads previous request, starts next one)
  if (selectedProbe == PROBE_PASTEURIZATION) {
    currentTemp = sensors.getTempCByIndex(0);
  } else if (selectedProbe == PROBE_PREHEAT) {
    currentTemp = sensors.getTempCByIndex(1);
  } else if (selectedProbe == PROBE_FERMENTATION) {
    currentTemp = (secondaryActive && rxData.ds18Status == 1) ? rxData.room2LiquidTemp : -127.0f;
  }
  sensors.requestTemperatures(); // Start conversion for next loop

  // 3. Update the TFT Screen and Serial Monitor (every 1 second)
  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    drawUI(currentTemp);
    Serial.printf("[DISPLAY] Selected Probe: %s | Temp: %.2f C\n", getProbeName(selectedProbe), currentTemp);
  }

  // 4. Handle Keypad Up/Down Navigation (Lockout-based debounce)
  if (millis() - lastPress > 250) {
    bool upPressed = (mcp.digitalRead(BTN_UP) == LOW);
    bool downPressed = (mcp.digitalRead(BTN_DOWN) == LOW);

    if (upPressed) {
      selectedProbe = (ProbeSelection)(((int)selectedProbe + 1) % 3);
      lastPress = millis();
      drawUI(currentTemp);
      Serial.printf("[KEY] Selection changed to: %s\n", getProbeName(selectedProbe));
    } else if (downPressed) {
      selectedProbe = (ProbeSelection)(((int)selectedProbe + 2) % 3); // -1 modulo 3
      lastPress = millis();
      drawUI(currentTemp);
      Serial.printf("[KEY] Selection changed to: %s\n", getProbeName(selectedProbe));
    }
  }

  delay(10);
}
