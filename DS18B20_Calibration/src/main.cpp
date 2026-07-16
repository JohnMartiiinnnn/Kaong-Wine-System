#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 26

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  STANDALONE DS18B20 TEMP PROBE VERIFICATION");
  Serial.println("=============================================");

  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.printf("DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, count);
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    sensors.requestTemperatures();
    int count = sensors.getDeviceCount();
    
    Serial.println("\n--- Local Temp Readings ---");
    for (int i = 0; i < count; i++) {
      float t = sensors.getTempCByIndex(i);
      const char *label = (i == 0) ? "Pasteurization (Index 0)" : ((i == 1) ? "Pre-heat (Index 1)" : "Unknown");
      Serial.printf("Probe %d (%s): %.2f C\n", i, label, t);
    }
    if (count == 0) {
      Serial.println("DS18B20 Temp Probes: [NO SENSORS DETECTED] Check wiring and 4.7k pull-up resistor!");
    }
  }
}
