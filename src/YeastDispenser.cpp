#include "YeastDispenser.h"

float msPerGramYeast = DEFAULT_MS_PER_GRAM_YEAST;
bool isYeastDispensing = false;
bool yeastDispenseErrorFlag = false;

void initYeastDispenser() {
#if defined(ESP32)
  ledcSetup(YEAST_PWM_CH_IN1, YEAST_PWM_FREQ, YEAST_PWM_RESOLUTION);
  ledcSetup(YEAST_PWM_CH_IN2, YEAST_PWM_FREQ, YEAST_PWM_RESOLUTION);
  ledcAttachPin(DRV8871_IN1_PIN, YEAST_PWM_CH_IN1);
  ledcAttachPin(DRV8871_IN2_PIN, YEAST_PWM_CH_IN2);
  ledcWrite(YEAST_PWM_CH_IN1, 0);
  ledcWrite(YEAST_PWM_CH_IN2, 0);
#else
  pinMode(DRV8871_IN1_PIN, OUTPUT);
  pinMode(DRV8871_IN2_PIN, OUTPUT);
  digitalWrite(DRV8871_IN1_PIN, LOW);
  digitalWrite(DRV8871_IN2_PIN, LOW);
#endif
  isYeastDispensing = false;
  yeastDispenseErrorFlag = false;
}

void stopYeastDispenser() {
  sendMotorCommand(mixerSpeedPercent, true, 3, 0); // yeastCmd 3 = Stop
  isYeastDispensing = false;
}

void dispenseYeastDuration(uint32_t durationMs) {
  if (durationMs == 0) return;
  if (durationMs > YEAST_DISPENSE_TIMEOUT_MS) {
    durationMs = YEAST_DISPENSE_TIMEOUT_MS;
    yeastDispenseErrorFlag = true;
  }

  isYeastDispensing = true;
  sendMotorCommand(mixerSpeedPercent, true, 1, durationMs); // yeastCmd 1 = DispenseDuration
}

void dispenseYeastReverseDuration(uint32_t durationMs) {
  if (durationMs == 0) return;
  if (durationMs > YEAST_DISPENSE_TIMEOUT_MS) {
    durationMs = YEAST_DISPENSE_TIMEOUT_MS;
    yeastDispenseErrorFlag = true;
  }

  isYeastDispensing = true;
  sendMotorCommand(mixerSpeedPercent, true, 5, durationMs); // yeastCmd 5 = ReverseDuration
}

void runYeastDiagnostic() {
  sendMotorCommand(mixerSpeedPercent, true, 4, 0); // yeastCmd 4 = Diagnostic
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
