/*
 * TB6612FN + JGY370 Motor Test — Secondary ESP32
 * ------------------------------------------------
 * WIRING (TB6612FN):
 *   VM   → 12 V supply (+)
 *   VCC  → 3.3 V (ESP32)
 *   GND  → GND (common with ESP32 and 12 V supply)
 *   STBY → 3.3 V  (hardwired HIGH — always active)
 *   AIN1 → 3.3 V  (hardwired HIGH — fixed CW direction)
 *   AIN2 → GND    (hardwired LOW)
 *   PWMA → GPIO25 (this sketch)
 *   AO1  → Motor (+)
 *   AO2  → Motor (-)
 *
 * SERIAL COMMANDS (115200 baud):
 *   0–100   → set speed to that percentage instantly (e.g. "75" + Enter)
 *   a / A   → run automatic ramp cycle (0→100→0 in 10% steps)
 *   s / S   → stop motor (speed = 0%)
 *   h / H   → print this help
 */

#include <Arduino.h>

#define PWMA_PIN     25
#define LEDC_CHANNEL  0
#define LEDC_FREQ  1000
#define LEDC_RES      8   // 8-bit: 0–255

static int currentSpeed = 0;

void setSpeed(int percent) {
    percent = constrain(percent, 0, 100);
    currentSpeed = percent;
    ledcWrite(LEDC_CHANNEL, map(percent, 0, 100, 0, 255));
    Serial.printf("[MOTOR] Speed set to %d%%\n", percent);
}

void printHelp() {
    Serial.println("-------------------------------");
    Serial.println("TB6612FN Motor Test — Commands:");
    Serial.println("  0-100  : set speed %");
    Serial.println("  a / A  : auto ramp cycle");
    Serial.println("  s / S  : stop motor");
    Serial.println("  h / H  : help");
    Serial.println("-------------------------------");
}

void rampCycle() {
    Serial.println("[AUTO] Ramp UP: 0% → 100%");
    for (int s = 0; s <= 100; s += 10) {
        setSpeed(s);
        delay(800);
    }
    Serial.println("[AUTO] Hold 3s at 100%");
    delay(3000);
    Serial.println("[AUTO] Ramp DOWN: 100% → 0%");
    for (int s = 100; s >= 0; s -= 10) {
        setSpeed(s);
        delay(800);
    }
    Serial.println("[AUTO] Cycle complete.");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RES);
    ledcAttachPin(PWMA_PIN, LEDC_CHANNEL);
    setSpeed(0);

    Serial.println("\n=== Motor Test Ready ===");
    printHelp();
}

void loop() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    char cmd = input.charAt(0);

    if (cmd == 'a' || cmd == 'A') {
        rampCycle();
    } else if (cmd == 's' || cmd == 'S') {
        setSpeed(0);
    } else if (cmd == 'h' || cmd == 'H') {
        printHelp();
    } else if (isDigit(cmd)) {
        int val = input.toInt();
        setSpeed(val);
    } else {
        Serial.printf("[ERR] Unknown command: \"%s\"\n", input.c_str());
        printHelp();
    }
}
