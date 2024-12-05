#include "LEDRing.h"
#include <FastLED.h>

#define DEBUG_BRIGHTNESS 0  // Set to 0 to disable brightness debugging

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
    setPattern(RAINBOW_IDLE);
    
    Serial.print("LED Ring initialized with pattern speed: ");
    Serial.println(ledPatternConfig.speed);
}

void LEDRing::setPattern(LEDPatternId patternId) {
    #if DEBUG_BRIGHTNESS
    Serial.print("Pattern Change: ");
    Serial.print(getPatternName(ledPatternConfig.id));
    Serial.print(" -> ");
    Serial.print(getPatternName(patternId));
    Serial.print(" (BR: ");
    Serial.print(patterns[static_cast<int>(patternId)].brightness);
    Serial.println(")");
    #endif
    
    ledPatternConfig = patterns[static_cast<int>(patternId)];
    isNewPattern = true;
}

LEDPatternId LEDRing::getPattern() {
    return ledPatternConfig.id;
}

void LEDRing::update(CHSV color, uint8_t energy, uint8_t maxEnergy) {
    unsigned long currentMillis = millis();
    
    // Check if current pattern is complete and queue has next pattern
    if (cycleComplete && !patternQueue.empty()) {
        LEDPatternId nextPattern = patternQueue.getFront();
        patternQueue.pop();
        setPattern(nextPattern);
        cycleComplete = false;
    }
    
    // Only update if enough time has passed
    if (currentMillis - lastUpdateTime >= LED_UPDATE_INTERVAL) {
        #if DEBUG_BRIGHTNESS
        static unsigned long lastBrightnessChange = 0;
        static uint8_t lastBrightness = 0;
        
        // Track time between brightness changes
        if (lastBrightness != ledPatternConfig.brightness) {
            Serial.print("Brightness change after: ");
            Serial.print(currentMillis - lastBrightnessChange);
            Serial.print("ms, Value: ");
            Serial.print(lastBrightness);
            Serial.print(" -> ");
            Serial.println(ledPatternConfig.brightness);
            
            lastBrightnessChange = currentMillis;
            lastBrightness = ledPatternConfig.brightness;
        }
        #endif

        float deltaTime = (currentMillis - lastUpdateTime) / 1000.0f;  // Time in seconds
        lastUpdateTime = currentMillis;

        // Update animation progress based on pattern speed
        float speedMultiplier = ledPatternConfig.speed / 100.0f;
        animationProgress += deltaTime * speedMultiplier;
        if (animationProgress >= 1.0f) {
            animationProgress = 0.0f;
        }
        
        // Update pattern based on current progress
        switch (ledPatternConfig.id) {
            case LEDPatternId::RAINBOW_IDLE:
                rainbow(animationProgress);
                break;
            case LEDPatternId::COLOR_CHASE:
                colorChase(color, energy, maxEnergy, animationProgress);
                break;
            case LEDPatternId::TRANSITION_FLASH:
                flash(color);
                break;
            case LEDPatternId::ERROR:
                error();
                break;
            case LEDPatternId::PULSE:
                pulse(color, energy, maxEnergy, animationProgress);
                break;
            case LEDPatternId::SPARKLE:
                sparkle();
                break;
            case LEDPatternId::SPARKLE_OUTWARD:
                sparkleOutward(color, energy, maxEnergy, animationProgress);
                break;
            case LEDPatternId::WARM_GOLD_ROTATE:
                warmGoldRotate(animationProgress);
                break;
            default:
                break;
        }

        FastLED.setBrightness(ledPatternConfig.brightness);
        FastLED.show();
        
        // Debug actual LED values periodically
        #if DEBUG_BRIGHTNESS
        static unsigned long lastDebugTime = 0;
        if (currentMillis - lastDebugTime > 1000) {  // Debug every second
            lastDebugTime = currentMillis;
            Serial.print("LED Values [0,8,16]: ");
            Serial.print(leds[0].getAverageLight());
            Serial.print(", ");
            Serial.print(leds[8].getAverageLight());
            Serial.print(", ");
            Serial.println(leds[16].getAverageLight());
        }
        #endif
    }
}

void LEDRing::rainbow(float progress) {
    // Calculate hue based on progress
    uint8_t hue = progress * 255;
    fill_rainbow(leds, NEOPIXEL_COUNT, hue);
}

void LEDRing::colorChase(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress) {
    static uint8_t phase = 0;
    static uint8_t hueOffset = 0;
    static int8_t hueDirection = -1;
    static CRGB previousLeds[NEOPIXEL_COUNT];
    const uint8_t MIN_INTENSITY = 60;

    // Reset static variables when pattern changes
    if (isNewPattern) {
        phase = 64;
        hueOffset = 0;
        hueDirection = -1;
        isNewPattern = false;
        
        // Initialize previous state
        uint8_t intensity = map(energy, 0, maxEnergy, MIN_INTENSITY, 255);
        CRGB rgbColor;
        hsv2rgb_rainbow(color, rgbColor);
        rgbColor.nscale8(intensity);
        fill_solid(leds, NEOPIXEL_COUNT, rgbColor);
        memcpy(previousLeds, leds, sizeof(CRGB) * NEOPIXEL_COUNT);
    }

    // Calculate current pixel position based on progress
    uint16_t currentPixel = (progress * NEOPIXEL_COUNT);
    
    // Update phase and hue
    phase += 4;
    hueOffset += hueDirection;
    if (abs(hueOffset) >= 15) hueDirection *= -1;

    // Calculate intensities and colors
    uint8_t intensity = map(energy, 0, maxEnergy, MIN_INTENSITY, 255);
    uint8_t globalIntensity = map(sin8(phase), 0, 255, MIN_INTENSITY, intensity);
    
    CHSV adjustedColor = color;
    adjustedColor.hue += hueOffset;

    // Create the chase effect
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        int distance = (i - currentPixel + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        float fadeRatio = pow(float(NEOPIXEL_COUNT/4 - abs(distance - NEOPIXEL_COUNT/4)) / (NEOPIXEL_COUNT/4), 2);
        uint8_t pixelIntensity = round(globalIntensity * fadeRatio);
        
        CRGB rgbColor;
        hsv2rgb_rainbow(adjustedColor, rgbColor);
        rgbColor.nscale8(pixelIntensity);
        leds[i] = blend(previousLeds[i], rgbColor, 128); // Smooth transition
    }

    memcpy(previousLeds, leds, sizeof(CRGB) * NEOPIXEL_COUNT);
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
        isNewPattern = false;
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
        // Generate new sparks with slightly higher chance
        if (sparkleCount < REQUIRED_SPARKLES && random8() < 35 && sparklePositions[i] == 0) {
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
            leds[i].nscale8(255);
        } else {
            leds[i] = CRGB::Black;  // Only black during initial sparkling phase
        }
    }
    
    // Complete the pattern when we've generated enough sparkles and active sparkles are done
    if (sparkleCount >= REQUIRED_SPARKLES && !anyActiveSparkles) {
        cycleComplete = true;
    }
}

void LEDRing::pulse(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress) {
    static uint8_t lastBrightness = 0;
    
    // Increase minimum brightness and reduce range for more stability
    // Change from (6, 40, 120) to (4, 80, 160)
    uint8_t rawBrightness = beatsin8(4, 80, 160);  // 4 BPM = slower, gentler pulse
    
    // Smooth the brightness changes by only updating when change is significant
    uint8_t brightness = rawBrightness;
    if (abs(brightness - lastBrightness) < 3) {
        brightness = lastBrightness;  // Ignore tiny changes
    }
    
    #if DEBUG_BRIGHTNESS
    if (brightness != lastBrightness) {
        Serial.print("Pulse brightness: ");
        Serial.print(lastBrightness);
        Serial.print(" -> ");
        Serial.print(brightness);
        Serial.print(" at ");
        Serial.println(millis());
        lastBrightness = brightness;
    }
    #endif
    
    // Create temporary HSV color with modified value
    CHSV adjustedColor = color;
    adjustedColor.val = brightness;
    
    // Fill strip with adjusted color
    fill_solid(leds, NEOPIXEL_COUNT, adjustedColor);
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
        case LEDPatternId::PULSE: return "PULSE";
        case LEDPatternId::SPARKLE: return "SPARKLE";
        case LEDPatternId::SPARKLE_OUTWARD: return "SPARKLE_OUTWARD";
        case LEDPatternId::WARM_GOLD_ROTATE: return "WARM_GOLD_ROTATE";
        default: return "UNKNOWN";
    }
}

void LEDRing::sparkleOutward(CHSV color, uint8_t energy, uint8_t maxEnergy, float progress) {
    static uint8_t sparklePositions[NEOPIXEL_COUNT] = {0};
    static uint8_t sparkleIntensities[NEOPIXEL_COUNT] = {0};
    static float currentRadius = 0.0f;
    
    // Reset tracking variables when pattern starts
    if (isNewPattern) {
        memset(sparklePositions, 0, NEOPIXEL_COUNT);
        memset(sparkleIntensities, 0, NEOPIXEL_COUNT);
        currentRadius = 0.0f;
        isNewPattern = false;
    }
    
    // Convert input HSV color to RGB for base color
    CRGB baseColor;
    hsv2rgb_rainbow(color, baseColor);
    
    // Create array of spark colors based on input color
    CRGB colors[] = {
        baseColor,                                          // Base color
        CRGB(baseColor).fadeToBlackBy(40),                 // Darker variant
        CRGB(baseColor).fadeToBlackBy(80),                 // Even darker variant
        CRGB(baseColor).nscale8(255).addToRGB(50)         // Brightened variant
    };
    
    bool anyActiveSparkles = false;
    const uint16_t startPoint = 0;
    
    // Calculate expansion rate based on speed parameter
    float expansionRate = map(ledPatternConfig.speed, 0, 255, 1, 4) * 0.01f;
    currentRadius += expansionRate;
    
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        int clockwiseDistance = (i - startPoint + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        int counterClockwiseDistance = (startPoint - i + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        int minDist = min(clockwiseDistance, counterClockwiseDistance);
        float normalizedDist = float(minDist) / (NEOPIXEL_COUNT/2);
        
        if (sparklePositions[i] == 0) {
            float distFromWaveFront = abs(normalizedDist - currentRadius);
            
            if (normalizedDist <= currentRadius && distFromWaveFront < 0.1f) {
                uint8_t sparkChance = 80;
                
                if (random8() < sparkChance) {
                    sparklePositions[i] = 1;
                    sparkleIntensities[i] = random8(180, 255);
                    
                    // Use color variants for sparkles
                    leds[i] = colors[random8(4)];
                    leds[i].nscale8(sparkleIntensities[i]);
                }
            }
        }
        
        if (sparklePositions[i] > 0) {
            anyActiveSparkles = true;
            sparklePositions[i]++;
            sparkleIntensities[i] = sparkleIntensities[i] > 5 ? 
                                  sparkleIntensities[i] - 5 : 0;
            
            if (sparkleIntensities[i] == 0) {
                sparklePositions[i] = 0;
                anyActiveSparkles = false;
            }
            
            // Use color variants for active sparkles
            leds[i] = colors[sparklePositions[i] % 4];
            leds[i].nscale8(sparkleIntensities[i]);
            
        } else {
            leds[i] = CRGB::Black;
        }
    }
    
    if (currentRadius >= 1.0f && !anyActiveSparkles) {
        // Fill with base color at the end
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            leds[i] = baseColor;
            leds[i].nscale8(ledPatternConfig.brightness);
        }
        cycleComplete = true;
    }
}

void LEDRing::warmGoldRotate(float progress) {
    static const CHSV WARM_GOLD = CHSV(32, 255, 100);  // Warm gold color
    static const uint8_t TRAIL_LENGTH = NEOPIXEL_COUNT;  // Use full ring for trail
    static const uint8_t PEAK_BRIGHTNESS = 100;  // Maximum brightness
    static const uint8_t MIN_BRIGHTNESS = 5;    // Lower minimum for smoother fade
    
    // Calculate current position with higher precision
    float currentPosF = progress * NEOPIXEL_COUNT;
    uint8_t currentPos = currentPosF;
    
    // Create the trail effect
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Calculate angular distance to current position
        float distanceF = abs(i - currentPosF);
        if (distanceF > NEOPIXEL_COUNT/2) {
            distanceF = NEOPIXEL_COUNT - distanceF;
        }
        
        // Use sine wave for smooth brightness falloff
        float normalizedDist = distanceF / (NEOPIXEL_COUNT/2);
        float fadeRatio = cos(normalizedDist * PI) * 0.5 + 0.5;
        
        // Calculate brightness using smooth curve
        uint8_t brightness = lerp(MIN_BRIGHTNESS, PEAK_BRIGHTNESS, fadeRatio * fadeRatio);
        
        // Set colors with fading brightness
        CHSV adjustedColor = WARM_GOLD;
        adjustedColor.val = brightness;
        leds[i] = adjustedColor;
    }
}


