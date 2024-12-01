#include "LEDRing.h"
#include <FastLED.h>

LEDRing::LEDRing(const LEDPatternConfig* customPatterns) : 
    patterns(customPatterns),
    nextPatternId(RAINBOW_IDLE),
    cycleComplete(false) {
    // Move pattern initialization to begin() instead of constructor
}

void LEDRing::begin() {
    FastLED.addLeds<WS2812B, NEOPIXEL_PIN, GRB>(leds, NEOPIXEL_COUNT);
    FastLED.setBrightness(0);
    FastLED.show();
    // Set initial pattern here after strip is initialized
    setPattern(RAINBOW_IDLE);
    
    // Debug output
    Serial.print("LED Ring initialized with pattern interval: ");
    Serial.println(ledPatternConfig.interval);
}

void LEDRing::setPattern(LEDPatternId patternId) {
    ledPatternConfig = patterns[static_cast<int>(patternId)];
    isNewPattern = true;
}

LEDPatternId LEDRing::getPattern() {
    return ledPatternConfig.id;
}

void LEDRing::update(uint32_t color, uint8_t energy, uint8_t maxEnergy) {
    static unsigned long ledPreviousMillis = millis();
    static uint8_t ledBrightness;
    static unsigned int ledPatternInterval;
    static LEDPatternId lastPatternId = LEDPatternId::RAINBOW_IDLE;
    static byte lastEnergy = 0;  // Add this to track energy changes

    unsigned long currentMillis = millis();

    // Check if we need to switch patterns
    if (cycleComplete) {
        // Log pattern completion
        Serial.print("Pattern complete: ");
        Serial.println(static_cast<int>(ledPatternConfig.id));
        
        if (!patternQueue.empty()) {
            LEDPatternId nextPattern = patternQueue.getFront();
            patternQueue.pop();
            setPattern(nextPattern);
        }
        cycleComplete = false;  // Always reset cycleComplete
    }

    // Update the interval when pattern changes or energy changes (for COLOR_CHASE pattern)
    if (lastPatternId != ledPatternConfig.id || 
        (ledPatternConfig.id == LEDPatternId::COLOR_CHASE && lastEnergy != energy)) {
                
        // Log pattern start
        Serial.print("Starting LED pattern: ");
        Serial.println(static_cast<int>(ledPatternConfig.id));
        
        lastPatternId = static_cast<LEDPatternId>(ledPatternConfig.id);
        lastEnergy = energy;
        ledPreviousMillis = currentMillis;
        
        // Use the configured interval from patterns
        ledPatternInterval = ledPatternConfig.interval;
        isNewPattern = true;
    }

    if (currentMillis - ledPreviousMillis >= ledPatternInterval) {
        ledPreviousMillis = currentMillis;

        switch (ledPatternConfig.id) {
            case LEDPatternId::RAINBOW_IDLE:
                rainbow();
                break;
            case LEDPatternId::COLOR_CHASE:
                if (energy == 0) {
                    noEnergy();
                } else {
                    colorChase(color, energy, maxEnergy);
                }
                break;
            case LEDPatternId::TRANSITION_FLASH:
                flash(color);
                break;
            case LEDPatternId::ERROR:
                error();
                break;
            case LEDPatternId::LOW_ENERGY_PULSE:
                noEnergy();
                break;
            case LEDPatternId::SPARKLE:
                sparkle();
                break;
            default:
                break;
        }

        isNewPattern = false;

        // Set brightness
        if (ledBrightness != ledPatternConfig.brightness) {
            ledBrightness = ledPatternConfig.brightness;
            FastLED.setBrightness(ledBrightness);
        }

        FastLED.show();
    }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void LEDRing::rainbow() {
    static long firstPixelHue = 0;
    if (firstPixelHue < 5*65536) {
        fill_rainbow(leds, NEOPIXEL_COUNT, firstPixelHue / 256);
        firstPixelHue += 256;
    } else {
        firstPixelHue = 0;
    }
}

// Rotates a weakening dot around the NeoPixel ring using the given color
void LEDRing::colorChase(uint32_t color, uint8_t energy, uint8_t maxEnergy) {
    static uint16_t currentPixel = 0;
    static uint8_t intensity = 255;
    static uint8_t globalIntensity = 0;
    static int8_t globalDirection = 1;
    static uint16_t hueOffset = 0;
    static int8_t hueDirection = 1;
    uint8_t adjustedIntensity;
    const uint16_t HUE_RANGE = 25;
    const uint8_t MIN_INTENSITY = 40;

    // Reset static variables when pattern changes
    if (isNewPattern) {
        globalIntensity = 0;
        globalDirection = 1;
        hueOffset = 0;
        hueDirection = 1;
    }

    // Update global intensity
    if (globalDirection > 0) {
        // When increasing, check for overflow
        uint16_t newIntensity = globalIntensity + (globalDirection * 9);
        if (newIntensity >= 255) {
            globalIntensity = 255;
            globalDirection = -1;
        } else {
            globalIntensity = newIntensity;
        }
    } else {
        // When decreasing, check for underflow
        int16_t newIntensity = globalIntensity + (globalDirection * 9);
        if (newIntensity <= MIN_INTENSITY) {
            globalIntensity = MIN_INTENSITY;
            globalDirection = 1;
        } else {
            globalIntensity = newIntensity;
        }
    }

    // Update hue offset
    hueOffset += hueDirection;
    if (hueOffset >= HUE_RANGE || hueOffset <= 0) {
        hueDirection *= -1;
        hueOffset = constrain(hueOffset, 0, HUE_RANGE);
    }

    // Find the trait color and apply hue shift
    uint8_t r = (uint8_t)(color >> 16);
    uint8_t g = (uint8_t)(color >> 8);
    uint8_t b = (uint8_t)color;
    
    // Convert uint32_t color to CRGB
    CRGB baseColor = CRGB(r, g, b);
    
    // Use FastLED's HSV functions instead of custom ones
    CHSV hsv = rgb2hsv_approximate(baseColor);
    hsv.hue += hueOffset;
    CRGB shiftedColor = hsv;
    
    // Calculate initial adjustedIntensity
    adjustedIntensity = intensity * globalIntensity / 255;
    leds[currentPixel] = shiftedColor;
    leds[currentPixel].nscale8(adjustedIntensity);
    
    // Set pixels between the dots with decreasing intensity
    for (int i = 1; i < NEOPIXEL_COUNT/2; i++) {
        // Calculate pixels on both sides
        uint16_t pixel1 = (currentPixel + i) % NEOPIXEL_COUNT;
        uint16_t pixel2 = (currentPixel - i + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        
        // Calculate fade based on distance to nearest bright dot
        float fadeRatio = pow(float(NEOPIXEL_COUNT/4 - abs(i - NEOPIXEL_COUNT/4)) / (NEOPIXEL_COUNT/4), 2);
        uint8_t fadeIntensity = round(intensity * fadeRatio);
        adjustedIntensity = (uint16_t)fadeIntensity * globalIntensity / 255;
        
        if (adjustedIntensity > 0) {
            leds[pixel1] = shiftedColor;
            leds[pixel1].nscale8(adjustedIntensity);
            leds[pixel2] = shiftedColor;
            leds[pixel2].nscale8(adjustedIntensity);
        }
    }
    
    // Move to next pixel
    currentPixel = (currentPixel + 1) % NEOPIXEL_COUNT;
}

void LEDRing::flash(uint32_t color) {
    static uint8_t intensity = 255;
    static int8_t intensityDirection = -1;
    static uint16_t hueOffset = 0;

    // Reset static variables when pattern changes
    if (isNewPattern) {
        intensity = 255;
        intensityDirection = -1;
        hueOffset = 0;
        cycleComplete = false;
    }

    // Convert uint32_t color to CRGB
    CRGB baseColor = CRGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);
    
    // Fast fade intensity
    intensity += intensityDirection * 12;
    if (intensity <= 30 || intensity >= 255) {
        intensityDirection *= -1;
        intensity = constrain(intensity, 30, 255);
        if (intensity <= 30) {
            cycleComplete = true;
            return;  // Return early since we're done with this pattern
        }
    }

    // Rotate hue offset in opposite direction
    hueOffset = (hueOffset - 8 + 360) % 360;

    // Fill strip with hue-shifted colors
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Calculate hue offset for this pixel
        uint16_t pixelHue = (hueOffset + (360 * i / NEOPIXEL_COUNT)) % 360;
        
        // Convert base color to HSV
        CHSV hsv = rgb2hsv_approximate(baseColor);
        
        // Apply hue shift
        float hueShift = sin(pixelHue * PI / 180.0) * 30;
        hsv.hue += hueShift;
        
        // Convert back to RGB and apply intensity
        CRGB shiftedColor = hsv;
        leds[i] = shiftedColor;
        leds[i].nscale8(intensity);
    }
}

void LEDRing::sparkle() {
    static uint8_t baseIntensity = 180;
    static uint8_t sparklePositions[NEOPIXEL_COUNT] = {0};
    static bool hasSparkled = false;  // Track if we've had at least one sparkle
    
    // Reset tracking variable when pattern starts
    if (isNewPattern) {
        hasSparkled = false;
    }
    
    // Gold base color (RGB: 255, 215, 0)
    CRGB goldColor = CRGB(255, 215, 0);
    
    // Accent colors
    CRGB orangeColor = CRGB(255, 140, 0);
    CRGB blueColor = CRGB(30, 144, 255);
    
    bool anyActiveSparkles = false;
    
    // Set base shimmer
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Create subtle shimmer effect
        uint8_t shimmer = random8(baseIntensity - 30, baseIntensity + 30);
        leds[i] = goldColor;
        leds[i].nscale8(shimmer);
        
        // Randomly create new sparkles
        if (random8() < 20 && sparklePositions[i] == 0) {  // 8% chance for new sparkle
            sparklePositions[i] = random8(20, 255);  // Random initial brightness
            hasSparkled = true;
            
            // 10% chance for accent color
            if (random8() < 25) {
                leds[i] = random8() < 128 ? orangeColor : blueColor;
            }
        }
        
        // Update existing sparkles
        if (sparklePositions[i] > 0) {
            anyActiveSparkles = true;
            leds[i].nscale8(sparklePositions[i]);
            sparklePositions[i] = sparklePositions[i] > 30 ? sparklePositions[i] - 30 : 0;
        }
    }
    
    // Set cycleComplete when we've had at least one sparkle and all sparkles have faded
    if (hasSparkled && !anyActiveSparkles) {
        cycleComplete = true;
    }
}

void LEDRing::noEnergy() {
    static uint8_t intensity = 0;
    static int8_t direction = 1;
    
    // Slowly pulse intensity up and down
    intensity += direction;
    if (intensity >= 255 || intensity <= 0) {
        direction *= -1;
        intensity = constrain(intensity, 0, 255); 
    }

    // Set all pixels to dimmed red
    CRGB redColor = CRGB(255, 0, 0);
    redColor.nscale8(intensity);
    fill_solid(leds, NEOPIXEL_COUNT, redColor);
}

void LEDRing::error() {
    static uint8_t r = 0;
    static uint8_t b = 255;
    static bool toRed = true;

    if (toRed) {
        r = min(255, r + 1);
        b = max(0, b - 1);
        if (r >= 255 && b <= 0) {
            toRed = false;
        }
    } else {
        r = max(0, r - 1); 
        b = min(255, b + 1);
        if (r <= 0 && b >= 255) {
            toRed = true;
        }
    }

    // Create color and fill strip
    CRGB errorColor = CRGB(r, 0, b);
    fill_solid(leds, NEOPIXEL_COUNT, errorColor);
}

// Helper function to dim a 32-bit color value by a certain intensity (0-255)
uint32_t LEDRing::dimColor(uint32_t color, uint8_t intensity) {
    // Extract RGB components from color
    uint8_t r = (uint8_t)(color >> 16);
    uint8_t g = (uint8_t)(color >> 8);
    uint8_t b = (uint8_t)color;
    
    CRGB rgb = CRGB(r, g, b);
    rgb.nscale8(intensity);
    return (uint32_t)rgb.r << 16 | (uint32_t)rgb.g << 8 | rgb.b;
}

float LEDRing::lerp(float start, float end, float t) {
    return start + t * (end - start);
}

// Add new method to queue patterns
void LEDRing::queuePattern(LEDPatternId patternId) {
    patternQueue.push(patternId);
    // Log pattern queuing
    Serial.print("Queued LED pattern: ");
    Serial.println(static_cast<int>(patternId));
}
