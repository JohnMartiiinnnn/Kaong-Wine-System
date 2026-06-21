/*
 * BTS7960 Motor Test — ESP32
 * ------------------------------------------------
 * WIRING:
 *   L_EN  → 3.3V (Enable Left)
 *   R_EN  → 3.3V (Enable Right)
 *   LPWM  → GPIO25 (Left PWM / Forward)
 *   RPWM  → GPIO26 (Right PWM / Reverse)
 *   VCC   → 3.3V / 5V (from ESP32 logic)
 *   GND   → GND
 *   B+    → 12V / 24V supply (+)
 *   B-    → GND
 *   M+    → Motor (+)
 *   M-    → Motor (-)
 *
 * SERIAL COMMANDS (115200 baud):
 *   0–100   → set speed to that percentage instantly (e.g. "75" + Enter)
 *   a / A   → ramp cycle CW  (0→100→0 in 10% steps)
 *   r / R   → ramp cycle CCW (0→100→0 in 10% steps)
 *   s / S   → stop motor (speed = 0%)
 *   h / H   → print this help
 */

#include <Arduino.h>

#define LPWM_PIN     25
#define RPWM_PIN     26

#define LPWM_CH      0
#define RPWM_CH      1
#define LEDC_FREQ    1000
#define LEDC_RES     8

static int currentSpeed = 0;

void setMotor(int percent, bool cw) {
    percent = constrain(percent, 0, 100);
    currentSpeed = percent;
    int pwmVal = map(percent, 0, 100, 0, 255);
    
    if (percent == 0) {
        ledcWrite(LPWM_CH, 0);
        ledcWrite(RPWM_CH, 0);
        Serial.println("[MOTOR] Stopped");
        return;
    }

    if (cw) {
        ledcWrite(RPWM_CH, 0);       // Ensure reverse is OFF
        ledcWrite(LPWM_CH, pwmVal);  // Set forward speed
        Serial.printf("[MOTOR] CW at %d%%\n", percent);
    } else {
        ledcWrite(LPWM_CH, 0);       // Ensure forward is OFF
        ledcWrite(RPWM_CH, pwmVal);  // Set reverse speed
        Serial.printf("[MOTOR] CCW at %d%%\n", percent);
    }
}

void printHelp() {
    Serial.println("-------------------------------");
    Serial.println("BTS7960 Motor Test — Commands:");
    Serial.println("  0-100  : set speed % (maintains current direction)");
    Serial.println("  a / A  : ramp cycle CW");
    Serial.println("  r / R  : ramp cycle CCW");
    Serial.println("  s / S  : stop motor");
    Serial.println("  h / H  : help");
    Serial.println("-------------------------------");
}

void rampCycle(bool cw) {
    setMotor(0, cw);
    delay(300);

    const char* label = cw ? "CW" : "CCW";
    Serial.printf("[AUTO] Ramp UP %s: 0%% → 100%%\n", label);
    for (int s = 0; s <= 100; s += 10) {
        setMotor(s, cw);
        delay(800);
    }
    Serial.println("[AUTO] Hold 3s at 100%");
    delay(3000);
    Serial.printf("[AUTO] Ramp DOWN %s: 100%% → 0%%\n", label);
    for (int s = 100; s >= 0; s -= 10) {
        setMotor(s, cw);
        delay(800);
    }

    Serial.println("[AUTO] Cycle complete.");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    ledcSetup(LPWM_CH, LEDC_FREQ, LEDC_RES);
    ledcSetup(RPWM_CH, LEDC_FREQ, LEDC_RES);
    
    ledcAttachPin(LPWM_PIN, LPWM_CH);
    ledcAttachPin(RPWM_PIN, RPWM_CH);
    
    setMotor(0, true);

    Serial.println("\n=== BTS7960 Motor Test Ready ===");
    printHelp();
}

static bool currentDirCW = true;

void loop() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    char cmd = input.charAt(0);

    if (cmd == 'a' || cmd == 'A') {
        currentDirCW = true;
        rampCycle(true);
    } else if (cmd == 'r' || cmd == 'R') {
        currentDirCW = false;
        rampCycle(false);
    } else if (cmd == 's' || cmd == 'S') {
        setMotor(0, currentDirCW);
    } else if (cmd == 'h' || cmd == 'H') {
        printHelp();
    } else if (isDigit(cmd)) {
        int val = input.toInt();
        setMotor(val, currentDirCW);
    } else {
        Serial.printf("[ERR] Unknown command: \"%s\"\n", input.c_str());
        printHelp();
    }
}
