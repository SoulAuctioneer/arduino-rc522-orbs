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

void LEDRing::update(CHSV color, uint8_t energy, uint8_t maxEnergy) {
    static unsigned long ledPreviousMillis = millis();
    static uint8_t ledBrightness;
    static unsigned int ledPatternInterval;
    static LEDPatternId lastPatternId = LEDPatternId::RAINBOW_IDLE;
    static byte lastEnergy = 0;

    unsigned long currentMillis = millis();

    // Check if we need to switch patterns
    if (cycleComplete) {
        // Log pattern completion
        Serial.print("Pattern complete: ");
        Serial.println(getPatternName(ledPatternConfig.id));
        
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
        Serial.println(getPatternName(ledPatternConfig.id));
        
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
            case LEDPatternId::SPARKLE_OUTWARD:
                sparkleOutward();
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
void LEDRing::colorChase(CHSV color, uint8_t energy, uint8_t maxEnergy) {
    static uint16_t currentPixel = 0;
    static uint8_t phase = 0;
    static uint8_t hueOffset = 0;
    static int8_t hueDirection = -1;
    static CRGB previousLeds[NEOPIXEL_COUNT];
    static uint8_t transitionProgress = 0;
    const uint8_t HUE_RANGE = 40;
    const uint8_t MIN_INTENSITY = 60;

    // Calculate base intensity from energy level
    uint8_t intensity = map(energy, 0, maxEnergy, MIN_INTENSITY, 255);
    
    // Update global intensity using sin8
    uint8_t globalIntensity = map(sin8(phase), 0, 255, MIN_INTENSITY, intensity);
    phase += 4;

    // Reset static variables when pattern changes
    if (isNewPattern) {
        phase = 64;
        hueOffset = 0;
        hueDirection = -1;
        currentPixel = 0;
        transitionProgress = 0;
        
        CRGB rgbColor;
        hsv2rgb_rainbow(color, rgbColor);
        fill_solid(leds, NEOPIXEL_COUNT, rgbColor);
        memcpy(previousLeds, leds, sizeof(CRGB) * NEOPIXEL_COUNT);
        
        Serial.println("Pattern Reset - Initial hue: " + String(color.hue));
        return;
    }

    // Update hue offset - smooth back and forth motion
    hueOffset = (uint8_t)(hueOffset + hueDirection);
    if (hueOffset >= HUE_RANGE || hueOffset == 0) {
        hueDirection *= -1;
        Serial.println("Direction change - hueOffset: " + String(hueOffset) + 
                      ", direction: " + String(hueDirection));
    }

    // Calculate pattern colors with explicit hue modification
    CHSV baseColor = color;
    baseColor.hue = color.hue + hueOffset;
    
    // Debug output every 10 frames
    if (currentPixel % 10 == 0) {
        Serial.println("Hue values - original: " + String(color.hue) + 
                      ", offset: " + String(hueOffset) + 
                      ", final: " + String(baseColor.hue));
    }

    // Store the target pattern colors in a temporary array
    CRGB targetLeds[NEOPIXEL_COUNT];
    memcpy(targetLeds, leds, sizeof(CRGB) * NEOPIXEL_COUNT);

    // Calculate the pattern
    uint8_t adjustedIntensity = globalIntensity;
    CRGB adjustedColor = CRGB(baseColor);
    adjustedColor.nscale8(adjustedIntensity);
    targetLeds[currentPixel] = adjustedColor;
    
    for (int i = 1; i < NEOPIXEL_COUNT/2; i++) {
        uint16_t pixel1 = (currentPixel + i) % NEOPIXEL_COUNT;
        uint16_t pixel2 = (currentPixel - i + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        
        float fadeRatio = pow(float(NEOPIXEL_COUNT/4 - abs(i - NEOPIXEL_COUNT/4)) / (NEOPIXEL_COUNT/4), 2);
        uint8_t fadeIntensity = round(globalIntensity * fadeRatio);
        adjustedIntensity = fadeIntensity;
        
        if (adjustedIntensity > 0) {
            CRGB rgbColor;
            hsv2rgb_rainbow(baseColor, rgbColor);
            rgbColor.nscale8(adjustedIntensity);
            targetLeds[pixel1] = rgbColor;
            targetLeds[pixel2] = rgbColor;
        } else {
            targetLeds[pixel1] = CRGB::Black;
            targetLeds[pixel2] = CRGB::Black;
        }
    }

    // Blend between previous and target states
    transitionProgress = min(255, transitionProgress + 8);  // Increase transition progress
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        leds[i] = blend(previousLeds[i], targetLeds[i], transitionProgress);
    }

    // Store current state as previous for next frame
    memcpy(previousLeds, leds, sizeof(CRGB) * NEOPIXEL_COUNT);
    
    // Move to next pixel
    currentPixel = (currentPixel + 1) % NEOPIXEL_COUNT;
}

void LEDRing::flash(CHSV color) {
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

    // Use HSV color directly
    CHSV baseColor = color;
    
    // Modify hue for effects
    baseColor.hue += hueOffset;
    
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

    // Fill strip with hue-shifted colors and apply intensity scaling
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        CHSV hsv = baseColor;
        hsv.val = scale8(hsv.val, intensity);
        leds[i] = hsv;
    }
}

void LEDRing::sparkle() {
    static uint8_t sparklePositions[NEOPIXEL_COUNT] = {0};
    static uint8_t fadeToWarm[NEOPIXEL_COUNT] = {0};
    static uint16_t sparkleCount = 0;
    static const uint16_t REQUIRED_SPARKLES = 24;
    
    // Reset tracking variables when pattern starts
    if (isNewPattern) {
        memset(sparklePositions, 0, NEOPIXEL_COUNT);
        memset(fadeToWarm, 0, NEOPIXEL_COUNT);
        sparkleCount = 0;
    }
    
    // Warm glow color (orange-yellow)
    CRGB warmGlow = CRGB(255, 140, 0);
    
    // Spark colors
    CRGB colors[] = {
        CRGB(255, 100, 0),    // Orange
        CRGB(255, 160, 0),    // Amber
        CRGB(255, 200, 0),    // Yellow
        CRGB(255, 120, 147)   // Pink
    };
    
    bool anyActiveSparkles = false;
    
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Generate new sparks
        if (sparkleCount < REQUIRED_SPARKLES && random8() < 25 && sparklePositions[i] == 0) {
            sparklePositions[i] = random8(150, 255);
            fadeToWarm[i] = 255;
            sparkleCount++;
            
            // Pick random spark color
            leds[i] = colors[random8(4)];
            leds[i].nscale8(sparklePositions[i]);
        }
        
        // Update existing sparks and fade to warm
        if (sparklePositions[i] > 0) {
            anyActiveSparkles = true;
            
            // Blend between spark color and warm glow
            if (fadeToWarm[i] > 0) {
                CRGB currentColor = leds[i];
                leds[i] = blend(warmGlow, currentColor, fadeToWarm[i]);
                fadeToWarm[i] = fadeToWarm[i] > 20 ? fadeToWarm[i] - 20 : 0;
            } else {
                leds[i] = warmGlow;
            }
            
            leds[i].nscale8(sparklePositions[i]);
            sparklePositions[i] = sparklePositions[i] > 15 ? sparklePositions[i] - 15 : 0;
        } else if (sparkleCount >= REQUIRED_SPARKLES) {
            // Keep warm glow after sparkling is done
            leds[i] = warmGlow;
            leds[i].nscale8(180);  // Slightly dimmed warm glow
        } else {
            leds[i] = CRGB::Black;  // Only black during initial sparkling phase
        }
    }
    
    // Complete the pattern when we've generated enough sparkles and active sparkles are done
    if (sparkleCount >= REQUIRED_SPARKLES && !anyActiveSparkles) {
        cycleComplete = true;
    }
}

void LEDRing::noEnergy() {
    static uint8_t phase = 0;
    
    // Use sin8 for smooth pulsing
    uint8_t intensity = sin8(phase);
    phase += 2; // Controls speed of pulse

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

float LEDRing::lerp(float start, float end, float t) {
    return start + t * (end - start);
}

// Add new method to queue patterns
void LEDRing::queuePattern(LEDPatternId patternId) {
    patternQueue.push(patternId);
    Serial.print("Queued LED pattern: ");
    Serial.println(getPatternName(patternId));
}

const char* LEDRing::getPatternName(LEDPatternId id) {
    switch (id) {
        case LEDPatternId::RAINBOW_IDLE: return "RAINBOW_IDLE";
        case LEDPatternId::COLOR_CHASE: return "COLOR_CHASE";
        case LEDPatternId::TRANSITION_FLASH: return "TRANSITION_FLASH";
        case LEDPatternId::ERROR: return "ERROR";
        case LEDPatternId::LOW_ENERGY_PULSE: return "LOW_ENERGY_PULSE";
        case LEDPatternId::SPARKLE: return "SPARKLE";
        case LEDPatternId::SPARKLE_OUTWARD: return "SPARKLE_OUTWARD";
        default: return "UNKNOWN";
    }
}

void LEDRing::sparkleOutward() {
    static uint8_t sparklePositions[NEOPIXEL_COUNT] = {0};
    static uint8_t fadeToWarm[NEOPIXEL_COUNT] = {0};
    static uint16_t currentRadius = 0;
    static const uint16_t MAX_RADIUS = NEOPIXEL_COUNT / 2;
    
    // Reset tracking variables when pattern starts
    if (isNewPattern) {
        memset(sparklePositions, 0, NEOPIXEL_COUNT);
        memset(fadeToWarm, 0, NEOPIXEL_COUNT);
        currentRadius = 0;
    }
    
    // Warm glow color (orange-yellow)
    CRGB warmGlow = CRGB(255, 140, 0);
    
    // Spark colors
    CRGB colors[] = {
        CRGB(255, 100, 0),    // Orange
        CRGB(255, 160, 0),    // Amber
        CRGB(255, 200, 0),    // Yellow
        CRGB(255, 120, 147)   // Pink
    };
    
    // Center points (for even number of LEDs, we use two center points)
    const uint16_t center1 = NEOPIXEL_COUNT / 2;
    const uint16_t center2 = (NEOPIXEL_COUNT / 2) - 1;
    
    // Expand outward
    if (currentRadius < MAX_RADIUS) {
        // Light up new positions
        int pos1_forward = (center1 + currentRadius) % NEOPIXEL_COUNT;
        int pos1_backward = (center1 - currentRadius + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        int pos2_forward = (center2 + currentRadius) % NEOPIXEL_COUNT;
        int pos2_backward = (center2 - currentRadius + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        
        // Only start new sparks if position isn't already active
        if (sparklePositions[pos1_forward] == 0) {
            sparklePositions[pos1_forward] = random8(150, 255);
            fadeToWarm[pos1_forward] = 255;
            leds[pos1_forward] = colors[random8(4)];
            leds[pos1_forward].nscale8(sparklePositions[pos1_forward]);
        }
        
        if (sparklePositions[pos1_backward] == 0) {
            sparklePositions[pos1_backward] = random8(150, 255);
            fadeToWarm[pos1_backward] = 255;
            leds[pos1_backward] = colors[random8(4)];
            leds[pos1_backward].nscale8(sparklePositions[pos1_backward]);
        }
        
        if (sparklePositions[pos2_forward] == 0) {
            sparklePositions[pos2_forward] = random8(150, 255);
            fadeToWarm[pos2_forward] = 255;
            leds[pos2_forward] = colors[random8(4)];
            leds[pos2_forward].nscale8(sparklePositions[pos2_forward]);
        }
        
        if (sparklePositions[pos2_backward] == 0) {
            sparklePositions[pos2_backward] = random8(150, 255);
            fadeToWarm[pos2_backward] = 255;
            leds[pos2_backward] = colors[random8(4)];
            leds[pos2_backward].nscale8(sparklePositions[pos2_backward]);
        }
        
        currentRadius++;
    }
    
    // Update existing sparks and fade to warm
    bool anyActiveSparkles = false;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        if (sparklePositions[i] > 0) {
            anyActiveSparkles = true;
            
            // Blend between spark color and warm glow
            if (fadeToWarm[i] > 0) {
                CRGB currentColor = leds[i];
                leds[i] = blend(warmGlow, currentColor, fadeToWarm[i]);
                fadeToWarm[i] = fadeToWarm[i] > 20 ? fadeToWarm[i] - 20 : 0;
            } else {
                leds[i] = warmGlow;
            }
            
            leds[i].nscale8(sparklePositions[i]);
            sparklePositions[i] = sparklePositions[i] > 15 ? sparklePositions[i] - 15 : 0;
        } else if (currentRadius >= MAX_RADIUS) {
            // Keep warm glow after sparkling is done
            leds[i] = warmGlow;
            leds[i].nscale8(180);  // Slightly dimmed warm glow
        } else {
            leds[i] = CRGB::Black;  // Only black during initial sparkling phase
        }
    }
    
    // Complete the pattern when we've reached maximum radius and active sparkles are done
    if (currentRadius >= MAX_RADIUS && !anyActiveSparkles) {
        cycleComplete = true;
    }
}


