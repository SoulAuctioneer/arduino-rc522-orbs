#pragma once
#include <FastLED.h>
#include "SimpleQueue.h"

// NeoPixel constants
#define NEOPIXEL_PIN    6
#define NEOPIXEL_COUNT  24

enum LEDPatternId {
    RAINBOW_IDLE,
    COLOR_CHASE,
    TRANSITION_FLASH,
    ERROR,
    LOW_ENERGY_PULSE,
    SPARKLE
};

struct LEDPatternConfig {
    LEDPatternId id;
    uint8_t brightness;
    uint16_t interval;
    uint16_t brightnessInterval;
};

// Default LED pattern configurations
const LEDPatternConfig LED_PATTERNS[] = {
    {RAINBOW_IDLE, 20, 20, 20},
    {COLOR_CHASE, 255, 80, 20},
    {TRANSITION_FLASH, 255, 10, 10},
    {ERROR, 255, 20, 20},
    {LOW_ENERGY_PULSE, 100, 200, 10},
    {SPARKLE, 180, 30, 20}  // Medium-high brightness, fast interval for sparkles
};

class LEDRing {
public:
    LEDRing(const LEDPatternConfig* patterns = LED_PATTERNS);
    void begin();
    void setPattern(LEDPatternId patternId);
    LEDPatternId getPattern();
    void update(uint32_t color, uint8_t energy, uint8_t maxEnergy);
    void queuePattern(LEDPatternId patternId);

private:
    CRGB leds[NEOPIXEL_COUNT];
    LEDPatternConfig ledPatternConfig;
    const LEDPatternConfig* patterns;
    LEDPatternId nextPatternId;
    bool isNewPattern;
    bool cycleComplete;
    SimpleQueue<LEDPatternId> patternQueue;
    void rainbow();
    void colorChase(uint32_t color, uint8_t energy, uint8_t maxEnergy);
    void flash(uint32_t color);
    void sparkle();
    void noEnergy();
    void error();
    
    uint32_t dimColor(uint32_t color, uint8_t intensity);
    float lerp(float start, float end, float t);
    void RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v);
    void HSVtoRGB(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);
};
