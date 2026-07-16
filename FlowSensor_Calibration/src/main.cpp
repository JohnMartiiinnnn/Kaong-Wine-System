#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

#define FLOW_SENSOR_1 32
#define FLOW_SENSOR_2 34

// MCP23017 Pins
#define PUMP_PREHEAT_FERM 9   // GPB1 - transfer pump 1
#define PUMP_FERM_PAST 10     // GPB2 - transfer pump 2

#define RELAY_ON LOW
#define RELAY_OFF HIGH

Adafruit_MCP23X17 mcp;
volatile uint32_t flow1Pulses = 0;
volatile uint32_t flow2Pulses = 0;

bool pump1State = false;
bool pump2State = false;
bool mcpStatus = false;

void IRAM_ATTR flowISR1() {
  flow1Pulses++;
}

void IRAM_ATTR flowISR2() {
  flow2Pulses++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE FLOW SENSOR CALIBRATION + PUMP");
  Serial.println("=============================================");

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
    mcpStatus = true;
    Serial.println("[OK] MCP23017 initialized. Pumps configured (OFF).");
  } else {
    Serial.println("[ERR] MCP23017 not detected at 0x20! Pumps disabled.");
  }

  Serial.println("\n--- Commands ---");
  Serial.println("  '1' - Toggle Pump 1 (Pre-Heat -> Ferm)");
  Serial.println("  '2' - Toggle Pump 2 (Ferm -> Past)");
  Serial.println("  'r' - Reset pulse counters to 0");
  Serial.println("  'c' - Calculate K-Factor using current pulses");
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    Serial.printf("Flow 1: %u pulses | Pump 1: %s\n", flow1Pulses, pump1State ? "ON" : "OFF");
    Serial.printf("Flow 2: %u pulses | Pump 2: %s\n", flow2Pulses, pump2State ? "ON" : "OFF");
    Serial.println("-----------------------------------");
  }

  if (Serial.available() > 0) {
    char cmd = Serial.read();
    // Clear any extra characters/newlines
    while (Serial.available() > 0) Serial.read();

    if (cmd == '1' && mcpStatus) {
      pump1State = !pump1State;
      mcp.digitalWrite(PUMP_PREHEAT_FERM, pump1State ? RELAY_ON : RELAY_OFF);
      Serial.printf("\n[Pump 1 toggled %s]\n", pump1State ? "ON" : "OFF");
    }
    else if (cmd == '2' && mcpStatus) {
      pump2State = !pump2State;
      mcp.digitalWrite(PUMP_FERM_PAST, pump2State ? RELAY_ON : RELAY_OFF);
      Serial.printf("\n[Pump 2 toggled %s]\n", pump2State ? "ON" : "OFF");
    }
    else if (cmd == 'r' || cmd == 'R') {
      flow1Pulses = 0;
      flow2Pulses = 0;
      Serial.println("\n[Flow pulses reset to zero]");
    }
    else if (cmd == 'c' || cmd == 'C') {
      // Temporarily stop pumps to avoid overflowing while calculating
      bool prevPump1 = pump1State;
      bool prevPump2 = pump2State;
      if (mcpStatus) {
        mcp.digitalWrite(PUMP_PREHEAT_FERM, RELAY_OFF);
        mcp.digitalWrite(PUMP_FERM_PAST, RELAY_OFF);
        pump1State = false;
        pump2State = false;
      }
      
      Serial.println("\n[K-Factor Calculation]");
      Serial.println("Input the volume of water passed in Liters (e.g. 1.0) and press Enter:");
      while (Serial.available() == 0) {
        delay(50);
      }
      float volumeLiters = Serial.parseFloat();
      while (Serial.available() > 0) Serial.read();

      if (volumeLiters <= 0.0f) {
        Serial.println("Aborting: Invalid volume input.");
      } else {
        float kf1 = (float)flow1Pulses / volumeLiters;
        float kf2 = (float)flow2Pulses / volumeLiters;
        Serial.printf("\nFlow 1 Pulses: %u   Calculated K-Factor: %.2f\n", flow1Pulses, kf1);
        Serial.printf("Flow 2 Pulses: %u   Calculated K-Factor: %.2f\n", flow2Pulses, kf2);
        Serial.println("Update FLOW1_KF and FLOW2_KF in src/config.h with these values.");
      }

      // Restore pumps status
      if (mcpStatus) {
        pump1State = prevPump1;
        pump2State = prevPump2;
        mcp.digitalWrite(PUMP_PREHEAT_FERM, pump1State ? RELAY_ON : RELAY_OFF);
        mcp.digitalWrite(PUMP_FERM_PAST, pump2State ? RELAY_ON : RELAY_OFF);
      }
    }
  }
}
