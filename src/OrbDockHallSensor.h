#ifndef ORB_DOCK_HALL_SENSOR_H
#define ORB_DOCK_HALL_SENSOR_H

#include "LEDRing.h"

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
    {SPARKLE_OUTWARD, 180, 140, 20}    // Fast outward sparkle expansion
};

// Popcorn patterns use max brightness
const LEDPatternConfig POPCORN_LED_PATTERNS[] = {
    {RAINBOW_IDLE, 255, 30, 40},       // Bright rainbow rotation
    {COLOR_CHASE, 255, 120, 40},       // Bright chase
    {TRANSITION_FLASH, 255, 200, 10},  // Bright flash transition
    {ERROR, 255, 80, 20},             // Bright error indication
    {PULSE, 255, 30, 10},             // Bright breathing pulse
    {SPARKLE, 255, 160, 20},          // Bright sparkle effect
    {SPARKLE_OUTWARD, 255, 140, 20}   // Bright outward sparkle expansion
};

// Add dock type enum before the class definition
enum DockType {
    NEST_DOCK,
    POPCORN_DOCK
};

class OrbDockHallSensor {
public:
    // Update constructor to accept dock type
    OrbDockHallSensor(DockType type = NEST_DOCK, int hallSensorPin = HALL_SENSOR_PIN);
    void begin();
    void loop();

private:
    LEDRing ledRing;
    int hallSensorPin;
    bool isOrbPresent;
    unsigned long lastCheckTime;
    DockType dockType;
    
    // Detection settings
    static const int MOVING_AVG_SIZE = 5;        // Small window for current readings
    static const int CALIBRATION_SAMPLES = 30;   // More samples for stable baseline
    static const int NEST_DETECTION_MARGIN = 20;     
    static const int POPCORN_DETECTION_MARGIN = 8;  
    
    int readings[MOVING_AVG_SIZE];            
    int readIndex = 0;                        
    int movingAverage = 0;                    
    int baselineValue = 0;                    
    
    int lastReading;                    // Store previous reading
    static const unsigned long CHECK_INTERVAL = 50; 
    static const CHSV ORB_PRESENT_COLOR;           
    static const CHSV NO_ORB_COLOR;
    bool isPopcornDock();
};

#endif 