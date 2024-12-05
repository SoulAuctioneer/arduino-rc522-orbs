#ifndef FAIRY_LIGHTS_H
#define FAIRY_LIGHTS_H

#include <FastLED.h>

#define FAIRY_LIGHTS_PIN 9
#define FAIRY_LIGHTS_COUNT 50
#define ACTIVE_DURATION 8000  // 8 seconds in milliseconds

class FairyLights {
public:
    FairyLights();
    void begin();
    void setActive(bool active);
    void update();

private:
    CRGB leds[FAIRY_LIGHTS_COUNT];
    bool isActive;
    unsigned long activeStartTime;
    unsigned long lastUpdateTime;
    uint8_t hue;
    
    static const unsigned long UPDATE_INTERVAL = 20;  // 50fps update rate
    static const uint8_t IDLE_SPEED = 1;             // Slow rotation speed for idle
    static const uint8_t ACTIVE_SPEED = 8;           // Fast rotation for active state
    static const uint8_t IDLE_BRIGHTNESS = 64;       // Dim warm glow when idle
    static const uint8_t ACTIVE_BRIGHTNESS = 255;    // Full brightness when active
    
    void updateIdle();
    void updateActive();
};

#endif 