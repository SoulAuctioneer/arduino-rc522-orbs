#include "FairyLights.h"

FairyLights::FairyLights() : 
    isActive(false),
    activeStartTime(0),
    lastUpdateTime(0),
    hue(0)
{
}

void FairyLights::begin() {
    FastLED.addLeds<WS2812B, FAIRY_LIGHTS_PIN, RGB>(leds, FAIRY_LIGHTS_COUNT).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(IDLE_BRIGHTNESS);
    
    // Initialize all LEDs to warm yellow
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        leds[i] = CRGB(255, 128, 0);  // Red=255, Green=128, Blue=0 for warm yellow
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
    // Use beatsin8 for smooth brightness pulsing (min, max, beats-per-minute, offset)
    uint8_t brightness = beatsin8(15, 96, 255, 0, 0);  // 15 BPM (faster), range 96-255 (stronger contrast)
    
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        // Using CRGB with scaled warm yellow values based on current brightness
        leds[i] = CRGB(brightness, brightness/2, 0);  // GRB order: Green=brightness/2, Red=brightness, Blue=0
    }
}

void FairyLights::updateActive() {
    // Fast rainbow rotation
    hue = (hue + ACTIVE_SPEED) % 255;
    
    for (int i = 0; i < FAIRY_LIGHTS_COUNT; i++) {
        // Use fill_rainbow which handles color conversion correctly
        leds[i] = CHSV(hue + (i * 255 / FAIRY_LIGHTS_COUNT), 255, 255);
        // Convert CHSV to CRGB using FastLED's color correction
        hsv2rgb_rainbow(CHSV(hue + (i * 255 / FAIRY_LIGHTS_COUNT), 255, 255), leds[i]);
    }
} 