#pragma once
#include <Arduino.h>

// ---- DRV8871 H-Bridge Yeast Dispenser Pinout & Config ----
#ifndef DRV8871_IN1_PIN
#define DRV8871_IN1_PIN 4
#endif

#ifndef DRV8871_IN2_PIN
#define DRV8871_IN2_PIN 5
#endif

#ifndef MAX_YEAST_MOTOR_DUTY
#define MAX_YEAST_MOTOR_DUTY 128 // 50% max duty cap (6.0V max for 6V motor on 11.93V rail)
#endif

#ifndef DEFAULT_MS_PER_GRAM_YEAST
#define DEFAULT_MS_PER_GRAM_YEAST 1000.0f
#endif

#ifndef YEAST_DISPENSE_TIMEOUT_MS
#define YEAST_DISPENSE_TIMEOUT_MS 30000UL
#endif

#ifndef YEAST_BRAKE_DURATION_MS
#define YEAST_BRAKE_DURATION_MS 200
#endif

#ifndef YEAST_PWM_FREQ
#define YEAST_PWM_FREQ 1000
#endif

#ifndef YEAST_PWM_RESOLUTION
#define YEAST_PWM_RESOLUTION 8
#endif

#ifndef YEAST_PWM_CH_IN1
#define YEAST_PWM_CH_IN1 4
#endif

#ifndef YEAST_PWM_CH_IN2
#define YEAST_PWM_CH_IN2 5
#endif

extern float msPerGramYeast;
extern bool isYeastDispensing;
extern bool yeastDispenseErrorFlag;

void initYeastDispenser();
void dispenseYeastDuration(uint32_t durationMs);
void dispenseYeastReverseDuration(uint32_t durationMs);
void dispenseYeastGrams(float grams);
void stopYeastDispenser();
void runYeastDiagnostic();
void setMsPerGramYeast(float msPerGram);
float getMsPerGramYeast();
float calculateMsPerGram(float testDurationMs, float weighedGrams);

