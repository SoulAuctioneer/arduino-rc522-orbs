#pragma once
#include <Adafruit_NeoPixel.h>

// NeoPixel constants
#define NEOPIXEL_PIN    6
#define NEOPIXEL_COUNT  24

enum LEDPatternId {
    NO_ORB,
    ORB_CONNECTED,
    FLASH,
    ERROR,
    NO_ENERGY
};

struct LEDPatternConfig {
    LEDPatternId id;
    uint8_t brightness;
    uint16_t interval;
    uint16_t brightnessInterval;
};

// Default LED pattern configurations
const LEDPatternConfig LED_PATTERNS[] = {
    {NO_ORB, 20, 20, 20},
    {ORB_CONNECTED, 255, 80, 20},
    {FLASH, 255, 10, 10},
    {ERROR, 255, 20, 20},
    {NO_ENERGY, 100, 200, 10}
};

class LEDRing {
public:
    LEDRing(const LEDPatternConfig* patterns = LED_PATTERNS);
    void begin();
    void setPattern(LEDPatternId patternId, LEDPatternId nextPattern);
    LEDPatternId getPattern();
    void update(uint32_t color, uint8_t energy, uint8_t maxEnergy);

private:
    Adafruit_NeoPixel strip;
    LEDPatternConfig ledPatternConfig;
    const LEDPatternConfig* patterns;
    LEDPatternId nextPatternId;
    bool isNewPattern;
    void rainbow();
    void colorChase(uint32_t color, uint8_t energy, uint8_t maxEnergy);
    void flash(uint32_t color);
    void noEnergy();
    void error();
    
    uint32_t dimColor(uint32_t color, uint8_t intensity);
    float lerp(float start, float end, float t);
    void RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v);
    void HSVtoRGB(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);
};
