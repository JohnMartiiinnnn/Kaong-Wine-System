#include "YeastDispenser.h"

float msPerGramYeast = DEFAULT_MS_PER_GRAM_YEAST;
bool isYeastDispensing = false;
bool yeastDispenseErrorFlag = false;

void initYeastDispenser() {
  pinMode(DRV8871_IN1_PIN, OUTPUT);
  pinMode(DRV8871_IN2_PIN, OUTPUT);
  digitalWrite(DRV8871_IN1_PIN, LOW);
  digitalWrite(DRV8871_IN2_PIN, LOW);
#if defined(ESP32)
  ledcSetup(YEAST_PWM_CH_IN1, YEAST_PWM_FREQ, YEAST_PWM_RESOLUTION);
  ledcSetup(YEAST_PWM_CH_IN2, YEAST_PWM_FREQ, YEAST_PWM_RESOLUTION);
  ledcAttachPin(DRV8871_IN1_PIN, YEAST_PWM_CH_IN1);
  ledcAttachPin(DRV8871_IN2_PIN, YEAST_PWM_CH_IN2);
  ledcWrite(YEAST_PWM_CH_IN1, 0);
  ledcWrite(YEAST_PWM_CH_IN2, 0);
#endif
  isYeastDispensing = false;
  yeastDispenseErrorFlag = false;
}

void stopYeastDispenser() {
  // Active Brake Pulse: IN1=HIGH, IN2=HIGH for 200ms to stop overrun/drip
#if defined(ESP32)
  ledcWrite(YEAST_PWM_CH_IN1, 255);
  ledcWrite(YEAST_PWM_CH_IN2, 255);
  delay(YEAST_BRAKE_DURATION_MS);
  // Coast / Idle: IN1=0, IN2=0
  ledcWrite(YEAST_PWM_CH_IN1, 0);
  ledcWrite(YEAST_PWM_CH_IN2, 0);
#else
  digitalWrite(DRV8871_IN1_PIN, HIGH);
  digitalWrite(DRV8871_IN2_PIN, HIGH);
  delay(YEAST_BRAKE_DURATION_MS);
  digitalWrite(DRV8871_IN1_PIN, LOW);
  digitalWrite(DRV8871_IN2_PIN, LOW);
#endif
  isYeastDispensing = false;
}

void setYeastManualControl(bool on, uint8_t speedPercent, bool cw) {
  if (!on || speedPercent == 0) {
    stopYeastDispenser();
    return;
  }
  isYeastDispensing = true;
  uint8_t duty = map(constrain((int)speedPercent, 0, 100), 0, 100, 0, MAX_YEAST_MOTOR_DUTY);
#if defined(ESP32)
  if (cw) {
    ledcWrite(YEAST_PWM_CH_IN1, duty);
    ledcWrite(YEAST_PWM_CH_IN2, 0);
  } else {
    ledcWrite(YEAST_PWM_CH_IN1, 0);
    ledcWrite(YEAST_PWM_CH_IN2, duty);
  }
#else
  if (cw) {
    analogWrite(DRV8871_IN1_PIN, duty);
    digitalWrite(DRV8871_IN2_PIN, LOW);
  } else {
    digitalWrite(DRV8871_IN1_PIN, LOW);
    analogWrite(DRV8871_IN2_PIN, duty);
  }
#endif
}

void dispenseYeastDuration(uint32_t durationMs) {
  if (durationMs == 0) return;
  if (durationMs > YEAST_DISPENSE_TIMEOUT_MS) {
    durationMs = YEAST_DISPENSE_TIMEOUT_MS;
    yeastDispenseErrorFlag = true;
  }

  isYeastDispensing = true;

  // Fast-Decay PWM Mode: IN1 = 255 (HIGH), IN2 = Inverted PWM (255 - MAX_YEAST_MOTOR_DUTY)
  // Provides active low-side MOSFET current recirculation for maximum low-speed torque
#if defined(ESP32)
  ledcWrite(YEAST_PWM_CH_IN1, 255);
  ledcWrite(YEAST_PWM_CH_IN2, 255 - MAX_YEAST_MOTOR_DUTY);
#else
  digitalWrite(DRV8871_IN1_PIN, HIGH);
  analogWrite(DRV8871_IN2_PIN, 255 - MAX_YEAST_MOTOR_DUTY);
#endif

  delay(durationMs);

  // Active Brake at the end of dispensing cycle
  stopYeastDispenser();
}

void dispenseYeastReverseDuration(uint32_t durationMs) {
  if (durationMs == 0) return;
  if (durationMs > YEAST_DISPENSE_TIMEOUT_MS) {
    durationMs = YEAST_DISPENSE_TIMEOUT_MS;
    yeastDispenseErrorFlag = true;
  }

  isYeastDispensing = true;

  // Reverse Fast-Decay PWM Mode: IN2 = 255 (HIGH), IN1 = Inverted PWM (255 - MAX_YEAST_MOTOR_DUTY)
#if defined(ESP32)
  ledcWrite(YEAST_PWM_CH_IN2, 255);
  ledcWrite(YEAST_PWM_CH_IN1, 255 - MAX_YEAST_MOTOR_DUTY);
#else
  digitalWrite(DRV8871_IN2_PIN, HIGH);
  analogWrite(DRV8871_IN1_PIN, 255 - MAX_YEAST_MOTOR_DUTY);
#endif

  delay(durationMs);

  stopYeastDispenser();
}

void runYeastDiagnostic() {
  Serial.println("\n[YEAST] Starting DRV8871 Hardware Diagnostic...");
  Serial.println("[YEAST] Driving FORWARD for 3000ms...");
  dispenseYeastDuration(3000);
  delay(500);

  Serial.println("[YEAST] Driving REVERSE for 3000ms...");
  dispenseYeastReverseDuration(3000);
  delay(500);

  Serial.println("[YEAST] Hardware Diagnostic Complete.\n");
}


void dispenseYeastGrams(float grams) {
  if (grams <= 0.0f) return;
  uint32_t durationMs = (uint32_t)(grams * msPerGramYeast);
  dispenseYeastDuration(durationMs);
}

void setMsPerGramYeast(float msPerGram) {
  if (msPerGram > 0.0f) {
    msPerGramYeast = msPerGram;
  }
}

float getMsPerGramYeast() {
  return msPerGramYeast;
}

float calculateMsPerGram(float testDurationMs, float weighedGrams) {
  if (weighedGrams <= 0.0f) return msPerGramYeast;
  float calculated = testDurationMs / weighedGrams;
  setMsPerGramYeast(calculated);
  return calculated;
}
