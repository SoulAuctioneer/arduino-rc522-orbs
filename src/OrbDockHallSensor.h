#ifndef ORB_DOCK_HALL_SENSOR_H
#define ORB_DOCK_HALL_SENSOR_H

#include "LEDRing.h"

#define HALL_SENSOR_PIN A0

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
    
    // New moving average members
    static const int MOVING_AVG_SIZE = 10;     // Reduced for responsiveness
    static const int DETECTION_MARGIN = 30;    // Reduced from 50 to account for 3.3V range
    
    int baselineValue;  // Will be set during initialization
    bool isBaselineSet = false;  // Track if we've established baseline
    int readings[MOVING_AVG_SIZE];            // Array to store readings
    int readIndex = 0;                        // Current position in array
    int movingAverage = 0;                    // Current moving average
    
    static const unsigned long CHECK_INTERVAL = 50; // Check every x ms
    static const CHSV ORB_PRESENT_COLOR;           // Pure orange color in HSV
    static const CHSV NO_ORB_COLOR;
};

#endif 