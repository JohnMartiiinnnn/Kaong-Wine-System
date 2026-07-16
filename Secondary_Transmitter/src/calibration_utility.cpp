#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pins based on Secondary Transmitter main.cpp
#define ONE_WIRE_BUS 13

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ADS1115 ads;

// Calibration variables for pH
float phOffsetVoltage = 2.555f; // Voltage at pH 7.0
float phSlope = 0.171f;         // Volts per pH unit (temperature compensated)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=======================================================");
  Serial.println("  SECONDARY ESP32 SENSOR CALIBRATION UTILITY");
  Serial.println("=======================================================");

  // Initialize I2C
  Wire.begin();

  // Initialize ADS1115 for pH
  if (ads.begin()) {
    Serial.println("ADS1115 (pH ADC) Initialized successfully.");
  } else {
    Serial.println("Error: ADS1115 (pH ADC) NOT DETECTED! Check I2C wiring.");
  }

  // Initialize DS18B20
  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.printf("DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, count);

  Serial.println("\n--- Commands ---");
  Serial.println("  '7' - Set pH 7.0 calibration (Calibrate offset voltage)");
  Serial.println("  '4' - Set pH 4.0 calibration (Calibrate slope)");
  Serial.println("  'h' - Print commands guide");
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();

    Serial.println("\n--- Live Data Readings ---");

    // 1. Read DS18B20 temperature
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
      Serial.printf("DS18B20 Fermentation Temp Probe: %.2f C\n", tempC);
    } else {
      Serial.println("DS18B20 Fermentation Temp Probe: [DISCONNECTED]");
      tempC = 25.0f; // Default temp for pH compensation
    }

    // 2. Read pH Sensor
    int16_t adcVal = ads.readADC_SingleEnded(0);
    float voltage = ads.computeVolts(adcVal);
    
    // Nernst temperature compensation factor
    float slope_comp = 0.17126f * (tempC + 273.15f) / 298.15f;
    float calculatedPH = 7.0f - (voltage - phOffsetVoltage) / slope_comp;

    Serial.printf("pH Probe (A0) - Raw ADC: %d   Voltage: %.4f V\n", adcVal, voltage);
    Serial.printf("Calculated pH (Offset V = %.3f, Compensated Slope = %.4f): %.2f pH\n",
                  phOffsetVoltage, slope_comp, calculatedPH);
  }

  // Handle commands from Serial Monitor
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Flush serial buffer
    while (Serial.available() > 0) Serial.read();

    if (cmd == '7') {
      Serial.println("\n[pH 7.0 Calibration]");
      Serial.println("Place pH probe in pH 7.0 buffer solution. Wait 5s for readings to stabilize...");
      for (int i = 5; i > 0; i--) {
        Serial.printf("%d...\n", i);
        delay(1000);
      }
      int16_t adcSum = 0;
      int samples = 20;
      for (int i = 0; i < samples; i++) {
        adcSum += ads.readADC_SingleEnded(0);
        delay(50);
      }
      float avgVoltage = ads.computeVolts(adcSum / samples);
      phOffsetVoltage = avgVoltage;
      Serial.printf("Calibration Successful!\nNew Offset Voltage (pH 7.0): %.4f V\n", phOffsetVoltage);
      Serial.println("Update the offset voltage constant (2.555) in Secondary main.cpp with this value.");
    }
    else if (cmd == '4') {
      Serial.println("\n[pH 4.0 Calibration]");
      Serial.println("Make sure you calibrated pH 7.0 FIRST.");
      Serial.println("Place pH probe in pH 4.0 buffer solution. Wait 5s for readings to stabilize...");
      for (int i = 5; i > 0; i--) {
        Serial.printf("%d...\n", i);
        delay(1000);
      }
      
      sensors.requestTemperatures();
      float tempC = sensors.getTempCByIndex(0);
      if (tempC == DEVICE_DISCONNECTED_C) tempC = 25.0f;
      
      int16_t adcSum = 0;
      int samples = 20;
      for (int i = 0; i < samples; i++) {
        adcSum += ads.readADC_SingleEnded(0);
        delay(50);
      }
      float avgVoltage = ads.computeVolts(adcSum / samples);
      
      // Calculate uncompensated slope at pH 4.0
      // pH = 7.0 - (V - V_7) / slope  => 4.0 = 7.0 - (V_4 - V_7) / slope => slope = (V_4 - V_7) / 3.0
      float rawSlope = (avgVoltage - phOffsetVoltage) / 3.0f;
      
      // Temperature de-compensate to get base slope at 25C (298.15K)
      // rawSlope = baseSlope * (tempC + 273.15) / 298.15 => baseSlope = rawSlope * 298.15 / (tempC + 273.15)
      phSlope = rawSlope * 298.15f / (tempC + 273.15f);

      Serial.printf("Calibration Successful!\n");
      Serial.printf("Measured Voltage at pH 4.0: %.4f V\n", avgVoltage);
      Serial.printf("Calculated Base Slope at 25C: %.5f V/pH\n", phSlope);
      Serial.println("Update the pH slope constant (0.17126) in Secondary main.cpp with this value.");
    }
    else if (cmd == 'h' || cmd == 'H') {
      Serial.println("\n--- Commands Guide ---");
      Serial.println("  '7' - Set pH 7.0 calibration (Calibrate offset voltage)");
      Serial.println("  '4' - Set pH 4.0 calibration (Calibrate slope)");
      Serial.println("  'h' - Print commands guide");
    }
  }
}
