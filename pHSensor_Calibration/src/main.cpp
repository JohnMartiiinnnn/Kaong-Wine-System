#include <Arduino.h>

#define RX2_PIN 16
#define TX2_PIN 17

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE pH SENSOR CALIBRATION BRIDGE");
  Serial.println("  (Connecting to Enclosed Secondary ESP32)");
  Serial.println("=============================================");
  Serial.println("  Commands for pH Sensor calibration:");
  Serial.println("    '7' - Set pH 7.0 calibration (neutral offset)");
  Serial.println("    '4' - Set pH 4.0 calibration (slope calibration)");
  Serial.println("=============================================\n");

  // Flush buffers
  while (Serial.available() > 0) Serial.read();
  while (Serial2.available() > 0) Serial2.read();
}

void loop() {
  // 1. Forward from USB Serial (PC) to UART (Secondary ESP32)
  while (Serial.available() > 0) {
    Serial2.write(Serial.read());
  }

  // 2. Forward from UART (Secondary ESP32) to USB Serial (PC)
  while (Serial2.available() > 0) {
    Serial.write(Serial2.read());
  }

  delay(1);
}
