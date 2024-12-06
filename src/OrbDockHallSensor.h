#ifndef ORB_DOCK_HALL_SENSOR_H
#define ORB_DOCK_HALL_SENSOR_H

#include "LEDRing.h"
#include "FairyLights.h"

#define HALL_SENSOR_PIN A0
#define ORB_PRESENT_PIN 2  // Digital pin D2

// Override LED pattern configurations for hall sensor version
const LEDPatternConfig NEST_LED_PATTERNS[] = {
    {RAINBOW_IDLE, 20, 30, 40},         // Gentle rainbow rotation
    {COLOR_CHASE, 250, 120, 40},        // Fast, energetic chase
    {TRANSITION_FLASH, 255, 200, 10},   // Very fast flash transition
    {ERROR, 255, 80, 20},              // Clear error indication
    {PULSE, 50, 10, 10},               // Smooth breathing pulse
    {SPARKLE, 180, 160, 20},           // Quick sparkle effect
    {SPARKLE_OUTWARD, 180, 140, 20},    // Fast outward sparkle expansion
    {WARM_GOLD_ROTATE, 100, 5, 20}    // Very slow rotation (5), moderate brightness
};

// Popcorn patterns use max brightness
const LEDPatternConfig POPCORN_LED_PATTERNS[] = {
    {RAINBOW_IDLE, 255, 30, 40},       // Bright rainbow rotation
    {COLOR_CHASE, 255, 120, 40},       // Bright chase
    {TRANSITION_FLASH, 255, 200, 10},  // Bright flash transition
    {ERROR, 255, 80, 20},             // Bright error indication
    {PULSE, 255, 30, 10},             // Bright breathing pulse
    {SPARKLE, 255, 160, 20},          // Bright sparkle effect
    {SPARKLE_OUTWARD, 255, 140, 20},   // Bright outward sparkle expansion
    {WARM_GOLD_ROTATE, 255, 5, 20}    // Same but with full brightness for popcorn
};

// Add dock type enum before the class definition
enum DockType {
    NEST_DOCK,
    POPCORN_DOCK
};

class OrbDockHallSensor {
public:
    // Update constructor to accept dock type
    OrbDockHallSensor(DockType type = NEST_DOCK, int hallSensorPin = HALL_SENSOR_PIN, FairyLights* fairyLights = nullptr);
    void begin();
    void loop();

private:
    LEDRing ledRing;
    int hallSensorPin;
    bool isOrbPresent = false;
    unsigned long lastCheckTime;
    DockType dockType;
    FairyLights* fairyLights;
    
    // Detection settings
    static const int CALIBRATION_SAMPLES = 5;     // Samples for initial baseline
    static const int NEST_DETECTION_MARGIN = 30;  // Detection threshold
    static const int POPCORN_DETECTION_MARGIN = 15;  // Detection threshold for popcorn
    
    int baselineValue = 0;                    
    static const unsigned long CHECK_INTERVAL = 75; 
    static const CHSV ORB_PRESENT_COLOR;           
    static const CHSV NO_ORB_COLOR;
};

#endif 