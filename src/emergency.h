#ifndef EMERGENCY_H
#define EMERGENCY_H

// Smart Edge: Local Sensor Limits
struct SensorLimits {
    float temp_max = 190.0;
    float hum_max = 90.0;
};

// Share limits globally
extern SensorLimits localLimits;

void checkEmergencyAndNotify(float temperature, float humidity);
void sendEmergencyToVercel(float temperature, float humidity);

#endif // EMERGENCY_H
