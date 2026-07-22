#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PIDController {
public:
    PIDController(float kp = 2.5f, float ki = 0.05f, float kd = 0.8f, 
                  float rampBand = 3.0f, float minOut = 0.0f, float maxOut = 100.0f);

    void setGains(float kp, float ki, float kd);
    void setRampBand(float band);
    void setOutputLimits(float minOut, float maxOut);
    void reset();

    // Computes duty cycle percentage (0.0 to 100.0)
    float compute(float setpoint, float currentTemp, float dt);

    float getIntegral() const { return integral; }

private:
    float kp;
    float ki;
    float kd;
    float rampBand;
    float minOut;
    float maxOut;

    float integral;
    float prevTemp;
    float dTempFiltered;
    bool initialized;
};

#endif // PID_CONTROLLER_H
