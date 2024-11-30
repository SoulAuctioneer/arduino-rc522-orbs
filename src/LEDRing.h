#pragma once
#include <Adafruit_NeoPixel.h>

// NeoPixel constants
#define NEOPIXEL_PIN    6
#define NEOPIXEL_COUNT  24

enum class LEDPatternId {
    NO_ORB,
    ORB_CONNECTED,
    FLASH,
    ERROR
};

struct LEDPatternConfig {
    LEDPatternId id;
    uint8_t brightness;
    uint16_t interval;
    uint16_t brightnessInterval;
};

class LEDRing {
public:
    LEDRing();
    void begin();
    void setPattern(LEDPatternId patternId, LEDPatternId nextPattern);
    LEDPatternId getPattern();
    void update(uint32_t color, uint8_t energy, uint8_t maxEnergy);

private:
    Adafruit_NeoPixel strip;
    LEDPatternConfig ledPatternConfig;
    LEDPatternId nextPatternId;
    void rainbow();
    void colorChase(uint32_t color, uint8_t energy, uint8_t maxEnergy);
    void flash(uint32_t color);
    void noEnergy();
    void error();
    
    uint32_t dimColor(uint32_t color, uint8_t intensity);
    float lerp(float start, float end, float t);
};
