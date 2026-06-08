#include <Arduino.h>
#include <HX711.h>

// ---- Pin config (matches Primary firmware: config.h) ----
#define HX711_DT_PIN 36
#define HX711_SCK_PIN 27

// ---- Starting calibration factor from last known good value ----
#define DEFAULT_CAL_FACTOR 22850.0f

HX711 scale;
float calFactor = DEFAULT_CAL_FACTOR;

void printHelp() {
  Serial.println("=========================================");
  Serial.println("  Load Cell Test — Serial Commands:");
  Serial.println("  t        : tare (zero the scale)");
  Serial.println("  r        : print raw ADC value (no scale)");
  Serial.println("  w        : print single weight reading");
  Serial.println("  a        : print 10-sample average");
  Serial.println("  +        : increase cal factor by 100");
  Serial.println("  -        : decrease cal factor by 100");
  Serial.println("  +10      : increase cal factor by 10");
  Serial.println("  -10      : decrease cal factor by 10");
  Serial.println("  c<value> : set cal factor (e.g. c22850)");
  Serial.println("  k<value> : calibrate with known weight in L");
  Serial.println("             (place known load, then send k<liters>)");
  Serial.println("  s        : stream continuous readings (toggle)");
  Serial.println("  h        : help");
  Serial.println("=========================================");
  Serial.printf("  Current cal factor: %.2f\n", calFactor);
  Serial.println("=========================================");
}

bool streaming = false;
unsigned long lastStream = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== Load Cell Test Ready ===");
  Serial.printf("DT=%d  SCK=%d  CalFactor=%.2f\n", HX711_DT_PIN, HX711_SCK_PIN,
                calFactor);

  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);

  if (!scale.is_ready()) {
    Serial.println("[ERROR] HX711 not detected — check wiring!");
  } else {
    scale.set_scale(calFactor);
    scale.tare();
    Serial.println("[OK] HX711 ready, tared to zero.");
  }

  printHelp();
}

void loop() {
  // Continuous stream mode (1 Hz)
  if (streaming && scale.is_ready() && millis() - lastStream > 1000) {
    float w = scale.get_units(5);
    Serial.printf("[STREAM] %.3f L  (cal=%.2f)\n", w, calFactor);
    lastStream = millis();
  }

  if (!Serial.available())
    return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0)
    return;

  char cmd = input.charAt(0);

  if (cmd == 't') {
    scale.tare();
    Serial.println("[TARE] Zeroed.");

  } else if (cmd == 'r') {
    if (scale.is_ready()) {
      long raw = scale.read_average(10);
      Serial.printf("[RAW] %ld\n", raw);
    } else {
      Serial.println("[ERR] HX711 not ready.");
    }

  } else if (cmd == 'w') {
    if (scale.is_ready()) {
      float w = scale.get_units(5);
      Serial.printf("[WEIGHT] %.3f L\n", w);
    } else {
      Serial.println("[ERR] HX711 not ready.");
    }

  } else if (cmd == 'a') {
    if (scale.is_ready()) {
      Serial.println("[AVG] Sampling 10 readings...");
      float w = scale.get_units(10);
      Serial.printf("[AVG] %.3f L\n", w);
    } else {
      Serial.println("[ERR] HX711 not ready.");
    }

  } else if (cmd == 's') {
    streaming = !streaming;
    Serial.printf("[STREAM] %s\n", streaming ? "ON" : "OFF");

  } else if (cmd == '+') {
    float step =
        (input.length() > 1 && input.substring(1) == "10") ? 10.0f : 100.0f;
    calFactor += step;
    scale.set_scale(calFactor);
    Serial.printf("[CAL] Factor = %.2f\n", calFactor);

  } else if (cmd == '-') {
    float step =
        (input.length() > 1 && input.substring(1) == "10") ? 10.0f : 100.0f;
    calFactor -= step;
    scale.set_scale(calFactor);
    Serial.printf("[CAL] Factor = %.2f\n", calFactor);

  } else if (cmd == 'c') {
    float val = input.substring(1).toFloat();
    if (val != 0.0f) {
      calFactor = val;
      scale.set_scale(calFactor);
      Serial.printf("[CAL] Factor set to %.2f\n", calFactor);
    } else {
      Serial.println("[ERR] Invalid value. Usage: c22850");
    }

  } else if (cmd == 'k') {
    // Calibrate with a known load: user tells us what's on the scale
    float knownLiters = input.substring(1).toFloat();
    if (knownLiters <= 0.0f) {
      Serial.println("[ERR] Invalid weight. Usage: k<liters>  e.g. k10");
    } else if (!scale.is_ready()) {
      Serial.println("[ERR] HX711 not ready.");
    } else {
      // Read raw offset from current tare, compute new factor
      scale.set_scale();              // remove factor temporarily
      long raw = scale.get_value(10); // raw units above tare
      calFactor = (float)raw / knownLiters;
      scale.set_scale(calFactor);
      Serial.printf("[CAL] Raw=%ld  Known=%.3fL  => New factor=%.2f\n", raw,
                    knownLiters, calFactor);
      Serial.println(
          "[CAL] Run 'w' to verify, then copy this factor to config.h");
    }

  } else if (cmd == 'h' || cmd == 'H') {
    printHelp();

  } else {
    Serial.printf("[ERR] Unknown command: \"%s\"\n", input.c_str());
    printHelp();
  }
}
