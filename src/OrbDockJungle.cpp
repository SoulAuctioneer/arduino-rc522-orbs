#include "OrbDock.h"
#include <FastLED.h>

#define NUM_LEDS_PER_STRIP 50
#define LED_STRIP1_PIN 7
#define LED_STRIP2_PIN 8
#define MAX_BRIGHTNESS 100
#define MIN_BRIGHTNESS 20

class OrbDockJungle : public OrbDock {
private:
    CRGB leds1[NUM_LEDS_PER_STRIP];
    CRGB leds2[NUM_LEDS_PER_STRIP];
    bool showAmbient = true;
    CRGB baseColor = CRGB::Black;
    uint8_t pulsePos1 = 0;
    uint8_t pulsePos2 = NUM_LEDS_PER_STRIP / 2;
    uint8_t hueShift = 0;

public:
    OrbDockJungle() : OrbDock(StationId::JUNGLE) {
    }

    void begin() override {
        OrbDock::begin();
        FastLED.addLeds<WS2812B, LED_STRIP1_PIN, GRB>(leds1, NUM_LEDS_PER_STRIP);
        FastLED.addLeds<WS2812B, LED_STRIP2_PIN, GRB>(leds2, NUM_LEDS_PER_STRIP);
        FastLED.setBrightness(MAX_BRIGHTNESS);
        Serial.println(F("Running OrbDockJungle"));
    }

    void loop() override {
        OrbDock::loop();
        if (showAmbient) {
            ambientEffect();
        } else {
            pulseEffect();
        }
        FastLED.show();
    }

protected:
    void onOrbConnected() override {
        Serial.println(F("OrbDockJungle Connected"));
        showAmbient = false;

        // Add energy if station hasn't been visited
        if (!getCurrentStationInfo().visited) {
            addEnergy(5);
        }

        // Get trait color
        CHSV hsvColor = TRAIT_COLORS[static_cast<int>(orbInfo.trait)];
        CRGB rgbColor;
        hsv2rgb_rainbow(hsvColor, rgbColor);
        baseColor = rgbColor;
    }

    void onOrbDisconnected() override {
        Serial.println(F("Orb disconnected"));
        showAmbient = true;
        fadeToBlackBy(leds1, NUM_LEDS_PER_STRIP, 255);
        fadeToBlackBy(leds2, NUM_LEDS_PER_STRIP, 255);
    }

private:
    void ambientEffect() {
        static uint16_t sLastMillis = 0;
        uint16_t ms = millis();
        
        if (ms - sLastMillis > 50) {
            sLastMillis = ms;
            
            fadeToBlackBy(leds1, NUM_LEDS_PER_STRIP, 20);
            fadeToBlackBy(leds2, NUM_LEDS_PER_STRIP, 20);
            
            // Add random dim twinkles
            if (random8() < 80) {
                int pos = random16(NUM_LEDS_PER_STRIP);
                leds1[pos] = CRGB(20, 30, 10);
                leds2[pos] = CRGB(20, 30, 10);
            }
        }
    }

    void pulseEffect() {
        static uint16_t sLastMillis = 0;
        uint16_t ms = millis();
        
        if (ms - sLastMillis > 30) {
            sLastMillis = ms;
            
            fadeToBlackBy(leds1, NUM_LEDS_PER_STRIP, 40);
            fadeToBlackBy(leds2, NUM_LEDS_PER_STRIP, 40);

            // Create pulses with base color and occasional hue shifts
            CRGB pulseColor = baseColor;
            if (random8() < 30) { // 30% chance of hue shift
                hueShift = random8(30); // Shift hue by up to 30
                CHSV hsv = rgb2hsv_approximate(baseColor);
                hsv.hue += hueShift;
                pulseColor = CHSV(hsv.hue, hsv.sat, hsv.val);
            }

            // Draw fading pulse trail
            for (int i = 0; i < 5; i++) {
                int pos1 = (pulsePos1 + i) % NUM_LEDS_PER_STRIP;
                int pos2 = (pulsePos2 + i) % NUM_LEDS_PER_STRIP;
                uint8_t fade = 255 - (i * 51); // Fade over 5 pixels
                
                leds1[NUM_LEDS_PER_STRIP - 1 - pos1] = pulseColor;
                leds1[NUM_LEDS_PER_STRIP - 1 - pos1].fadeToBlackBy(255 - fade);
                
                leds2[NUM_LEDS_PER_STRIP - 1 - pos2] = pulseColor;
                leds2[NUM_LEDS_PER_STRIP - 1 - pos2].fadeToBlackBy(255 - fade);
            }

            // Move pulse positions
            pulsePos1 = (pulsePos1 + 1) % NUM_LEDS_PER_STRIP;
            pulsePos2 = (pulsePos2 + 1) % NUM_LEDS_PER_STRIP;
        }
    }
};
