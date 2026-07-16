#include <Arduino.h>

#define FLOW_SENSOR_1 32
#define FLOW_SENSOR_2 34

volatile uint32_t flow1Pulses = 0;
volatile uint32_t flow2Pulses = 0;

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
  Serial.println("  STANDALONE FLOW SENSOR CALIBRATION");
  Serial.println("=============================================");

  pinMode(FLOW_SENSOR_1, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1), flowISR1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2), flowISR2, RISING);
  Serial.println("Flow Sensors Interrupts Attached.");

  Serial.println("\n--- Commands ---");
  Serial.println("  'r' - Reset pulse counters to 0");
  Serial.println("  'c' - Calculate K-Factor using current pulses");
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    Serial.printf("Flow Sensor 1 (Pre-Heat -> Ferm) Pulses: %u\n", flow1Pulses);
    Serial.printf("Flow Sensor 2 (Ferm -> Past) Pulses: %u\n", flow2Pulses);
  }

  if (Serial.available() > 0) {
    char cmd = Serial.read();
    while (Serial.available() > 0) Serial.read();

    if (cmd == 'r' || cmd == 'R') {
      flow1Pulses = 0;
      flow2Pulses = 0;
      Serial.println("\n[Flow pulses reset to zero]");
    }
    else if (cmd == 'c' || cmd == 'C') {
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
    }
  }
}
