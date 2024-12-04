#pragma once
#include <FastLED.h>
#include "SimpleQueue.h"

// NeoPixel constants
#define NEOPIXEL_PIN    6
#define NEOPIXEL_COUNT  24

#define LED_UPDATE_INTERVAL 8  // Fast, consistent update rate for all patterns

enum LEDPatternId {
    RAINBOW_IDLE,
    COLOR_CHASE,
    TRANSITION_FLASH,
    ERROR,
    PULSE,
    SPARKLE,
    SPARKLE_OUTWARD
};

struct LEDPatternConfig {
    LEDPatternId id;
    uint8_t brightness;
    uint16_t speed;           // Renamed from interval - now represents pattern speed
    uint16_t brightnessInterval;
};

// Default LED pattern configurations (adjusted speeds)
const LEDPatternConfig LED_PATTERNS[] = {
    {RAINBOW_IDLE, 20, 2, 20},         // Slow rainbow
    {COLOR_CHASE, 255, 4, 20},         // Medium speed chase
    {TRANSITION_FLASH, 255, 12, 10},   // Fast flash
    {ERROR, 255, 3, 20},              // Slow error transition
    {PULSE, 100, 1, 10},              // Very slow pulse
    {SPARKLE, 255, 160, 20},          // Quick sparkle effect - increased speed to match hall sensor
    {SPARKLE_OUTWARD, 255, 140, 20}   // Fast outward sparkle expansion
};

class LEDRing {
public:
    LEDRing(const LEDPatternConfig* patterns = LED_PATTERNS);
    void begin();
    void setPattern(LEDPatternId patternId);
    LEDPatternId getPattern();
    void update(CHSV color, uint8_t energy, uint8_t maxEnergy);
    void queuePattern(LEDPatternId patternId);

private:
    CRGB leds[NEOPIXEL_COUNT];
    LEDPatternConfig ledPatternConfig;
    const LEDPatternConfig* patterns;
    LEDPatternId nextPatternId;
    bool isNewPattern;
    bool cycleComplete;
    SimpleQueue<LEDPatternId> patternQueue;
    unsigned long lastUpdateTime;
    float animationProgress;

    const char* getPatternName(LEDPatternId id); 
    void rainbow(float progress);
    void colorChase(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress);
    void flash(CHSV color);
    void sparkle();
    void sparkleOutward(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress);
    void pulse(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress);
    void error();
    
    float lerp(float start, float end, float t);
};
