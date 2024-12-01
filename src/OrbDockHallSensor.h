#ifndef ORB_DOCK_HALL_SENSOR_H
#define ORB_DOCK_HALL_SENSOR_H

#include "LEDRing.h"

#define HALL_SENSOR_PIN 2

// Override LED pattern configurations for hall sensor version
const LEDPatternConfig HALL_LED_PATTERNS[] = {
    {RAINBOW_IDLE, 20, 20, 40},         // Dimmer, slower rainbow when no orb
    {COLOR_CHASE, 250, 80, 40},         // Medium brightness, slower rotation when orb present
    {TRANSITION_FLASH, 255, 10, 10},    // Keep flash the same
    {ERROR, 255, 20, 20},              // Keep error the same
    {LOW_ENERGY_PULSE, 100, 200, 10},   // Keep no energy the same
    {SPARKLE, 180, 70, 20},             // Medium-high brightness, fast sparkle effect
    {SPARKLE_OUTWARD, 180, 40, 20}      // Medium-high brightness, fast outward sparkle effect
};

class OrbDockHallSensor {
public:
    OrbDockHallSensor(int hallSensorPin = HALL_SENSOR_PIN);
    void begin();
    void loop();

private:
    LEDRing ledRing;
    int hallSensorPin;
    bool isOrbPresent;
    unsigned long lastCheckTime;
    
    static const unsigned long CHECK_INTERVAL = 300; // Check every 300ms
    static const CHSV ORB_PRESENT_COLOR; // Pure orange color in HSV
};

#endif 