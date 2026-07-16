#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdarg.h>

// Pins based on Secondary Transmitter main.cpp
#define ONE_WIRE_BUS 13
#define RXD2 16
#define TXD2 17

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ADS1115 ads;

// Calibration variables for pH
float phOffsetVoltage = 2.555f; // Voltage at pH 7.0
float phSlope = 0.171f;         // Volts per pH unit (temperature compensated)

// Helper function to print to both Serial (USB) and Serial2 (UART)
void printMsg(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.print(buf);
  Serial2.print(buf);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  printMsg("\n=======================================================\n");
  printMsg("  SECONDARY ESP32 SENSOR CALIBRATION UTILITY\n");
  printMsg("=======================================================\n");

  // Initialize I2C
  Wire.begin();

  // Initialize ADS1115 for pH
  if (ads.begin()) {
    printMsg("ADS1115 (pH ADC) Initialized successfully.\n");
  } else {
    printMsg("Error: ADS1115 (pH ADC) NOT DETECTED! Check I2C wiring.\n");
  }

  // Initialize DS18B20
  sensors.begin();
  int count = sensors.getDeviceCount();
  printMsg("DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, count);

  printMsg("\n--- Commands ---\n");
  printMsg("  '7' - Set pH 7.0 calibration (Calibrate offset voltage)\n");
  printMsg("  '4' - Set pH 4.0 calibration (Calibrate slope)\n");
  printMsg("  'h' - Print commands guide\n");
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();

    printMsg("\n--- Live Data Readings ---\n");

    // 1. Read DS18B20 temperature
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
      printMsg("DS18B20 Fermentation Temp Probe: %.2f C\n", tempC);
    } else {
      printMsg("DS18B20 Fermentation Temp Probe: [DISCONNECTED]\n");
      tempC = 25.0f; // Default temp for pH compensation
    }

    // 2. Read pH Sensor
    int16_t adcVal = ads.readADC_SingleEnded(0);
    float voltage = ads.computeVolts(adcVal);
    
    // Nernst temperature compensation factor
    float slope_comp = 0.17126f * (tempC + 273.15f) / 298.15f;
    float calculatedPH = 7.0f - (voltage - phOffsetVoltage) / slope_comp;

    printMsg("pH Probe (A0) - Raw ADC: %d   Voltage: %.4f V\n", adcVal, voltage);
    printMsg("Calculated pH (Offset V = %.3f, Compensated Slope = %.4f): %.2f pH\n",
                  phOffsetVoltage, slope_comp, calculatedPH);
  }

  // Handle commands from Serial Monitor or UART Serial2
  char cmd = 0;
  if (Serial.available() > 0) {
    cmd = Serial.read();
  } else if (Serial2.available() > 0) {
    cmd = Serial2.read();
  }

  if (cmd != 0) {
    // Flush both serial buffers
    while (Serial.available() > 0) Serial.read();
    while (Serial2.available() > 0) Serial2.read();

    if (cmd == '7') {
      printMsg("\n[pH 7.0 Calibration]\n");
      printMsg("Place pH probe in pH 7.0 buffer solution. Wait 5s for readings to stabilize...\n");
      for (int i = 5; i > 0; i--) {
        printMsg("%d...\n", i);
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
      printMsg("Calibration Successful!\nNew Offset Voltage (pH 7.0): %.4f V\n", phOffsetVoltage);
      printMsg("Update the offset voltage constant (2.555) in Secondary main.cpp with this value.\n");
    }
    else if (cmd == '4') {
      printMsg("\n[pH 4.0 Calibration]\n");
      printMsg("Make sure you calibrated pH 7.0 FIRST.\n");
      printMsg("Place pH probe in pH 4.0 buffer solution. Wait 5s for readings to stabilize...\n");
      for (int i = 5; i > 0; i--) {
        printMsg("%d...\n", i);
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
      float rawSlope = (avgVoltage - phOffsetVoltage) / 3.0f;
      
      // Temperature de-compensate to get base slope at 25C (298.15K)
      phSlope = rawSlope * 298.15f / (tempC + 273.15f);

      printMsg("Calibration Successful!\n");
      printMsg("Measured Voltage at pH 4.0: %.4f V\n", avgVoltage);
      printMsg("Calculated Base Slope at 25C: %.5f V/pH\n", phSlope);
      printMsg("Update the pH slope constant (0.17126) in Secondary main.cpp with this value.\n");
    }
    else if (cmd == 'h' || cmd == 'H') {
      printMsg("\n--- Commands Guide ---\n");
      printMsg("  '7' - Set pH 7.0 calibration (Calibrate offset voltage)\n");
      printMsg("  '4' - Set pH 4.0 calibration (Calibrate slope)\n");
      printMsg("  'h' - Print commands guide\n");
    }
  }
}
