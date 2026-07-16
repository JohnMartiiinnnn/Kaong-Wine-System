#include <Arduino.h>
#include <HX711.h>

#define HX711_DT 36
#define HX711_SCK 27

HX711 scale;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE LOAD CELL (HX711) CALIBRATION");
  Serial.println("=============================================");

  scale.begin(HX711_DT, HX711_SCK);

  if (scale.is_ready()) {
    Serial.println("[OK] HX711 Load Cell detected.");
  } else {
    Serial.println("[ERR] HX711 Load Cell NOT ready. Check DT/SCK wiring!");
  }

  Serial.println("\n--- Commands ---");
  Serial.println("  't' - Tare the Load Cell (empty scale platform first)");
  Serial.println("  'c' - Calibrate the Load Cell (place known weight, e.g. 1000g)");
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    if (scale.is_ready()) {
      long rawVal = scale.read();
      long avgVal = scale.read_average(5);
      Serial.printf("Load Cell Raw Value: %ld   Average (5 reads): %ld\n", rawVal, avgVal);
    } else {
      Serial.println("Load Cell (HX711): [NOT READY]");
    }
  }

  if (Serial.available() > 0) {
    char cmd = Serial.read();
    while (Serial.available() > 0) Serial.read();

    if (cmd == 't' || cmd == 'T') {
      Serial.println("\n[Taring...] Ensure scale platform is completely empty.");
      delay(1000);
      scale.tare(20);
      Serial.printf("Tare Finished! Offset Value: %ld\n", scale.get_offset());
    } 
    else if (cmd == 'c' || cmd == 'C') {
      Serial.println("\n[Load Cell Calibration Mode]");
      Serial.println("1. Place a known weight on the scale platform (e.g. 1.0kg).");
      Serial.println("2. Input the weight value in grams (e.g. 1000) and press Enter:");
      
      while (Serial.available() == 0) {
        delay(50);
      }
      float weightGrams = Serial.parseFloat();
      while (Serial.available() > 0) Serial.read(); // Flush remainder
      
      if (weightGrams <= 0.0f) {
        Serial.println("Aborting: Invalid weight input.");
      } else {
        Serial.printf("Calibrating with: %.1f grams\n", weightGrams);
        long rawAvg = scale.read_average(10);
        long offset = scale.get_offset();
        float calcFactor = (float)(rawAvg - offset) / weightGrams;
        Serial.printf("Raw Average Value: %ld\n", rawAvg);
        Serial.printf("Offset Value: %ld\n", offset);
        Serial.printf("Calculated Calibration Factor: %.4f\n", calcFactor);
        Serial.println("--- Action ---");
        Serial.println("Use this factor in config.h to replace 'calibrationFactor'.");
        scale.set_scale(calcFactor);
      }
    }
  }
}
