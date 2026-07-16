#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HX711.h>
#include <Wire.h>

// Pins based on config.h / PINOUT.md
#define ONE_WIRE_BUS 26
#define HX711_DT 36
#define HX711_SCK 27
#define FLOW_SENSOR_1 32
#define FLOW_SENSOR_2 34
#define RX2_PIN 16
#define TX2_PIN 17

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
HX711 scale;

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
  Serial.println("\n=======================================================");
  Serial.println("  PRIMARY ESP32 SENSOR CALIBRATION UTILITY");
  Serial.println("=======================================================");
  
  // 1. Initialize DS18B20 Temp Probes
  sensors.begin();
  int tempProbeCount = sensors.getDeviceCount();
  Serial.printf("DS18B20 Probes Found on Pin %d: %d\n", ONE_WIRE_BUS, tempProbeCount);
  
  // 2. Initialize HX711 Load Cell
  scale.begin(HX711_DT, HX711_SCK);
  Serial.println("HX711 Load Cell Initialized.");
  
  // 3. Initialize Flow Sensors
  pinMode(FLOW_SENSOR_1, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1), flowISR1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2), flowISR2, RISING);
  Serial.println("Flow Sensors Interrupts Attached.");

  // 4. Initialize UART2 for Secondary Communication
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.println("UART2 (Secondary Comm) Initialized.");
  
  Serial.println("\n--- Commands ---");
  Serial.println("  't' - Tare the Load Cell (empty scale first)");
  Serial.println("  'c' - Calibrate the Load Cell (place known weight, e.g. 1000g)");
  Serial.println("  'r' - Reset Flow sensor pulse counters");
  Serial.println("  's' - Switch to Secondary ESP32 Bridge Mode (over UART)");
  Serial.println("  'h' - Print commands guide");
}

void loop() {
  static bool bridgeMode = false;

  if (bridgeMode) {
    // Forward from USB Serial (PC) to UART (Secondary)
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'p' || c == 'P') {
        bridgeMode = false;
        Serial.println("\n[Exited Bridge Mode. Returned to Primary Calibration Mode.]");
        // Flush remaining Serial input
        while (Serial.available() > 0) Serial.read();
        break;
      } else {
        Serial2.write(c);
      }
    }

    // Forward from UART (Secondary) to USB Serial (PC)
    while (Serial2.available() > 0) {
      Serial.write(Serial2.read());
    }

    delay(10);
    return; // Skip Primary printing and commands
  }

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    
    Serial.println("\n--- Live Data Readings ---");
    
    // 1. Temperatures
    sensors.requestTemperatures();
    int count = sensors.getDeviceCount();
    for (int i = 0; i < count; i++) {
      float t = sensors.getTempCByIndex(i);
      // Primary: Probe 0 is Pasteurization, Probe 1 is Pre-heat
      const char *label = (i == 0) ? "Pasteurization" : ((i == 1) ? "Pre-heat" : "Unknown");
      Serial.printf("DS18B20 Temp Probe %d (%s): %.2f C\n", i, label, t);
    }
    if (count == 0) {
      Serial.println("DS18B20 Temp Probes: [NO SENSORS DETECTED]");
    }
    
    // 2. Load Cell Raw Readings
    if (scale.is_ready()) {
      long rawVal = scale.read();
      long avgVal = scale.read_average(5);
      Serial.printf("Load Cell Raw Value: %ld   Average (5 reads): %ld\n", rawVal, avgVal);
    } else {
      Serial.println("Load Cell (HX711): [NOT READY]");
    }
    
    // 3. Flow Sensors Pulse Counts
    Serial.printf("Flow Sensor 1 (Pre-Heat -> Ferm) Pulses: %u\n", flow1Pulses);
    Serial.printf("Flow Sensor 2 (Ferm -> Past) Pulses: %u\n", flow2Pulses);
  }
  
  // Handle commands from Serial Terminal
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Flush serial buffer
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
    else if (cmd == 'r' || cmd == 'R') {
      flow1Pulses = 0;
      flow2Pulses = 0;
      Serial.println("\n[Flow pulses reset to zero]");
    }
    else if (cmd == 's' || cmd == 'S') {
      bridgeMode = true;
      Serial.println("\n=======================================================");
      Serial.println("  ENTERING SECONDARY ESP32 BRIDGE MODE");
      Serial.println("  All serial data will be forwarded to the Secondary.");
      Serial.println("  Press 'p' to exit and return to Primary Calibration.");
      Serial.println("=======================================================\n");
      // Flush UART Serial2 buffer
      while (Serial2.available() > 0) Serial2.read();
    }
    else if (cmd == 'h' || cmd == 'H') {
      Serial.println("\n--- Commands Guide ---");
      Serial.println("  't' - Tare the Load Cell (empty scale first)");
      Serial.println("  'c' - Calibrate the Load Cell (place known weight, e.g. 1000g)");
      Serial.println("  'r' - Reset Flow sensor pulse counters");
      Serial.println("  's' - Switch to Secondary ESP32 Bridge Mode (over UART)");
      Serial.println("  'h' - Print commands guide");
    }
  }
}
