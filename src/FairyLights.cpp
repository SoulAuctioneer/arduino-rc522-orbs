#include "FairyLights.h"

FairyLights::FairyLights() : 
    isActive(false),
    activeStartTime(0),
    lastUpdateTime(0),
    hue(0)
{
}

void FairyLights::begin() {
    FastLED.addLeds<WS2812B, FAIRY_LIGHTS_PIN, GRB>(leds, FAIRY_LIGHTS_COUNT);
    FastLED.setBrightness(IDLE_BRIGHTNESS);
    
    // Initialize all LEDs to warm yellow
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        leds[i] = CHSV(32, 255, 255);  // Warm yellow in HSV
    }
    FastLED.show();
}

void FairyLights::setActive(bool active) {
    if (active && !isActive) {
        activeStartTime = millis();
    }
    isActive = active;
    FastLED.setBrightness(active ? ACTIVE_BRIGHTNESS : IDLE_BRIGHTNESS);
}

void FairyLights::update() {
    unsigned long currentTime = millis();
    
    // Check if active state should end
    if (isActive && (currentTime - activeStartTime >= ACTIVE_DURATION)) {
        setActive(false);
    }
    
    // Only update at the specified interval
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        if (isActive) {
            updateActive();
        } else {
            updateIdle();
        }
        
        FastLED.show();
    }
}

void FairyLights::updateIdle() {
    // Slowly rotate warm yellow hues
    hue = (hue + IDLE_SPEED) % 255;
    
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        // Create subtle variation in the warm yellow
        uint8_t pixelHue = 32 + sin8(i * 4 + hue) / 8;  // Vary around hue 32 (yellow)
        leds[i] = CHSV(pixelHue, 255, 255);
    }
}

void FairyLights::updateActive() {
    // Fast rainbow rotation
    hue = (hue + ACTIVE_SPEED) % 255;
    
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        // Create rainbow pattern that moves through the strip
        uint8_t pixelHue = hue + (i * 255 / FAIRY_LIGHTS_COUNT);
        leds[i] = CHSV(pixelHue, 255, 255);
    }
} 