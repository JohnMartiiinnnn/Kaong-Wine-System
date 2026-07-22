#include "PIDController.h"

PIDController::PIDController(float kp, float ki, float kd, float rampBand, float minOut, float maxOut)
    : kp(kp), ki(ki), kd(kd), rampBand(rampBand), minOut(minOut), maxOut(maxOut),
      integral(0.0f), prevTemp(0.0f), dTempFiltered(0.0f), initialized(false) {}

void PIDController::setGains(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

void PIDController::setRampBand(float band) {
    rampBand = band;
}

void PIDController::setOutputLimits(float minO, float maxO) {
    minOut = minO;
    maxOut = maxO;
}

void PIDController::reset() {
    integral = 0.0f;
    prevTemp = 0.0f;
    dTempFiltered = 0.0f;
    initialized = false;
}

float PIDController::compute(float setpoint, float currentTemp, float dt) {
    if (dt <= 0.0f) dt = 1.0f;

    float error = setpoint - currentTemp;

    // RULE 1: Absolute Zero Power at or above setpoint (zero electrical power past setpoint)
    if (error <= 0.0f) {
        integral = 0.0f;
        prevTemp = currentTemp;
        dTempFiltered = 0.0f;
        initialized = true;
        return 0.0f;
    }

    // Initialize previous temperature on first entry
    if (!initialized) {
        prevTemp = currentTemp;
        dTempFiltered = 0.0f;
        initialized = true;
    }

    // Low-Pass Filtered Derivative: smooths DS18B20 0.0625C quantization steps
    float dTempRaw = (currentTemp - prevTemp) / dt;
    prevTemp = currentTemp;
    dTempFiltered = dTempFiltered * 0.7f + dTempRaw * 0.3f;

    // RULE 2: Reduced Thermal Lag Dampening (Active only when heating rapidly > 0.06 deg C / sec)
    float coastBuffer = (dTempFiltered > 0.06f) ? (dTempFiltered * 4.0f) : 0.0f;
    float effectiveError = error - coastBuffer;

    if (effectiveError <= 0.0f) {
        // Stored heat in heater element is sufficient to carry liquid to setpoint -> turn OFF
        return 0.0f;
    }

    // RULE 3: Full Power Ramp Phase (> rampBand degrees below setpoint)
    if (effectiveError > rampBand) {
        integral = 0.0f;
        return maxOut;
    }

    // RULE 4: Smooth Approach Tapering with 25% Holding Floor for Ambient Heat Loss
    float linearCap = maxOut * (effectiveError / rampBand);
    float maxApproachCap = (linearCap < 25.0f) ? 25.0f : linearCap;

    // RULE 5: Fine Holding Zone (within 0.5C of setpoint)
    // Build integral holding power to overcome ambient thermal loss
    if (effectiveError <= 0.5f) {
        integral += dt * 0.5f; // Add +0.5% per second when inside 0.5C of setpoint
    } else {
        integral += effectiveError * dt * 0.2f;
    }

    if (integral > maxApproachCap) integral = maxApproachCap;
    if (integral < 0.0f) integral = 0.0f;

    // Calculate P + I - D with increased proportional drive
    float pTerm = kp * effectiveError * 12.0f;

    // Minimum proportional drive near setpoint to ensure firm setpoint contact
    if (effectiveError > 0.0f && effectiveError <= 0.5f) {
        float minPDrive = 10.0f * (effectiveError / 0.5f);
        if (pTerm < minPDrive) pTerm = minPDrive;
    }

    // Reduced Derivative Dampening (Braking)
    float dBrake = (dTempFiltered > 0.0f) ? (kd * dTempFiltered * 3.0f) : 0.0f;

    float output = pTerm + integral - dBrake;

    // Clamp output between 0 and maxApproachCap
    if (output > maxApproachCap) output = maxApproachCap;
    if (output < minOut) output = minOut;

    return output;
}
