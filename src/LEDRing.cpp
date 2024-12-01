#include "LEDRing.h"

LEDRing::LEDRing(const LEDPatternConfig* customPatterns) : 
    strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
    patterns(customPatterns),
    nextPatternId(NO_ORB) {
    // Move pattern initialization to begin() instead of constructor
}

void LEDRing::begin() {
    strip.begin();
    strip.setBrightness(0);
    strip.show();
    // Set initial pattern here after strip is initialized
    setPattern(NO_ORB, NO_ORB);
    
    // Debug output
    Serial.print("LED Ring initialized with pattern interval: ");
    Serial.println(ledPatternConfig.interval);
}

void LEDRing::setPattern(LEDPatternId patternId, LEDPatternId nextPattern) {
    ledPatternConfig = patterns[static_cast<int>(patternId)];
    nextPatternId = nextPattern;
    isNewPattern = true;
}

LEDPatternId LEDRing::getPattern() {
    return ledPatternConfig.id;
}

void LEDRing::update(uint32_t color, uint8_t energy, uint8_t maxEnergy) {
    static unsigned long ledPreviousMillis = millis();
    static uint8_t ledBrightness;
    static unsigned int ledPatternInterval;
    static LEDPatternId lastPatternId = LEDPatternId::NO_ORB;
    static byte lastEnergy = 0;  // Add this to track energy changes

    unsigned long currentMillis = millis();

    // Update the interval when pattern changes or energy changes (for ORB_CONNECTED pattern)
    if (lastPatternId != ledPatternConfig.id || 
        (ledPatternConfig.id == LEDPatternId::ORB_CONNECTED && lastEnergy != energy)) {
        
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
            case LEDPatternId::NO_ORB:
                rainbow();
                break;
            case LEDPatternId::ORB_CONNECTED:
                if (energy == 0) {
                    noEnergy();
                } else {
                    colorChase(color, energy, maxEnergy);
                }
                break;
            case LEDPatternId::FLASH:
                flash(color);
                break;
            case LEDPatternId::ERROR:
                error();
                break;
            default:
                break;
        }

        isNewPattern = false;

        // Set brightness
        if (ledBrightness != ledPatternConfig.brightness) {
            ledBrightness = ledPatternConfig.brightness;
            strip.setBrightness(ledBrightness);
        }
        // Smooth brightness transitions
        // TODO: This causes a bunch of flickering jank. Figure out why.
        // if (ledBrightness != ledPatternConfig.brightness && currentMillis - ledBrightnessPreviousMillis >= ledPatternConfig.brightnessInterval) {
        //     ledBrightnessPreviousMillis = currentMillis;
        //     ledBrightness = lerp(ledBrightness, ledPatternConfig.brightness, ledPatternConfig.brightnessInterval);
        //     strip.setBrightness(ledBrightness);
        // }

        strip.show();
    }

}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void LEDRing::rainbow() {
    static long firstPixelHue = 0;
    if (firstPixelHue < 5*65536) {
      strip.rainbow(firstPixelHue, 1, 255, 255, true);
      firstPixelHue += 256;
    } else {
      firstPixelHue = 0;
    }
}

// Rotates a weakening dot around the NeoPixel ring using the given color
void LEDRing::colorChase(uint32_t color, uint8_t energy, uint8_t maxEnergy) {
    static uint16_t currentPixel = 0;
    static uint8_t intensity = 255;
    const uint16_t HUE_RANGE = 40; // Maximum hue shift in degrees
    const uint8_t MIN_INTENSITY = 30;
    static uint8_t globalIntensity = MIN_INTENSITY;  // Start at MIN_INTENSITY instead of 0
    static int8_t globalDirection = 1;     // Start with increasing direction
    static uint16_t hueOffset = 0;
    static int8_t hueDirection = 1;

    // Reset static variables when pattern changes
    if (isNewPattern) {
        globalIntensity = MIN_INTENSITY;
        globalDirection = 1;
        hueOffset = 0;
        hueDirection = 1;
    }

    // Update global intensity
    globalIntensity += globalDirection * 9;  // Adjust 2 to change global fade speed
    if (globalIntensity >= 255 || globalIntensity <= MIN_INTENSITY) {
        globalDirection *= -1;
        globalIntensity = constrain(globalIntensity, MIN_INTENSITY, 255);
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
    
    // Replace the RGB shift code with proper HSV-based hue shifting
    uint32_t shiftedColor;
    if (hueOffset != 0) {
        // Convert RGB to HSV
        float h, s, v;
        RGBtoHSV(r, g, b, &h, &s, &v);
        
        // Shift the hue (h is 0-360)
        h += (float)hueOffset / HUE_RANGE * h; // Shift by up to HUE_RANGE degrees
        if (h >= 360.0f) h -= 360.0f;
        if (h < 0.0f) h += 360.0f;
        
        // Convert back to RGB
        uint8_t new_r, new_g, new_b;
        HSVtoRGB(h, s, v, &new_r, &new_g, &new_b);
        shiftedColor = strip.Color(new_r, new_g, new_b);
    } else {
        shiftedColor = color;
    }

    // Calculate opposite pixel position
    uint16_t oppositePixel = (currentPixel + (NEOPIXEL_COUNT / 2)) % NEOPIXEL_COUNT;
    
    // Set both bright dots
    uint8_t adjustedIntensity = (uint16_t)intensity * globalIntensity / 255;
    strip.setPixelColor(currentPixel, dimColor(shiftedColor, adjustedIntensity));
    strip.setPixelColor(oppositePixel, dimColor(shiftedColor, adjustedIntensity));
    
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
            strip.setPixelColor(pixel1, dimColor(shiftedColor, adjustedIntensity));
            strip.setPixelColor(pixel2, dimColor(shiftedColor, adjustedIntensity));
        }
    }
    
    // Move to next pixel
    currentPixel = (currentPixel + 1) % NEOPIXEL_COUNT;
}

void LEDRing::flash(uint32_t color) {
    static uint8_t intensity = 255;
    static int8_t intensityDirection = -1;
    static uint16_t hueOffset = 0;
    static bool cycleComplete = false;

    // If cycle is complete, switch to appropriate pattern
    if (cycleComplete) {
        intensity = 255;
        intensityDirection = -1;
        hueOffset = 0;
        cycleComplete = false;
        setPattern(nextPatternId, nextPatternId);
        return;
    }

    // Get base trait color and extract hue
    uint8_t r = (uint8_t)(color >> 16);
    uint8_t g = (uint8_t)(color >> 8); 
    uint8_t b = (uint8_t)color;

    // Fast fade intensity
    intensity += intensityDirection * 12;
    if (intensity <= 30 || intensity >= 255) {
        intensityDirection *= -1;
        intensity = constrain(intensity, 30, 255);
        if (intensity <= 30) {
            cycleComplete = true;
        }
    }

    // Rotate hue offset in opposite direction
    hueOffset = (hueOffset - 8 + 360) % 360;

    // Fill strip with hue-shifted colors
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Calculate hue offset for this pixel
        uint16_t pixelHue = (hueOffset + (360 * i / NEOPIXEL_COUNT)) % 360;
        
        // Create color with similar hue to trait color but varying
        float hueShift = sin(pixelHue * PI / 180.0) * 30; // +/- 30 degree hue shift
        uint32_t shiftedColor = strip.Color(
            r + (r * hueShift/360),
            g + (g * hueShift/360),
            b + (b * hueShift/360)
        );

        strip.setPixelColor(i, dimColor(shiftedColor, intensity));
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
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(intensity, 0, 0));
    }
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

    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, r, 0, b);
    }
}

// Helper function to dim a 32-bit color value by a certain intensity (0-255)
uint32_t LEDRing::dimColor(uint32_t color, uint8_t intensity) {
  uint8_t r = (uint8_t)(color >> 16);
  uint8_t g = (uint8_t)(color >> 8);
  uint8_t b = (uint8_t)color;
  
  r = (r * intensity) >> 8;
  g = (g * intensity) >> 8;
  b = (b * intensity) >> 8;
  
  return strip.Color(r, g, b);
}

float LEDRing::lerp(float start, float end, float t) {
    return start + t * (end - start);
}

// Add these helper functions to the LEDRing class
void LEDRing::RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    float cmax = max(max(rf, gf), bf);
    float cmin = min(min(rf, gf), bf);
    float delta = cmax - cmin;

    // Calculate hue
    if (delta == 0) {
        *h = 0;
    } else if (cmax == rf) {
        *h = 60.0f * fmod(((gf - bf) / delta), 6);
    } else if (cmax == gf) {
        *h = 60.0f * (((bf - rf) / delta) + 2);
    } else {
        *h = 60.0f * (((rf - gf) / delta) + 4);
    }
    
    if (*h < 0) *h += 360.0f;

    // Calculate saturation
    *s = (cmax == 0) ? 0 : (delta / cmax);
    
    // Calculate value
    *v = cmax;
}

void LEDRing::HSVtoRGB(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float c = v * s;
    float x = c * (1 - abs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    float rf, gf, bf;

    if (h >= 0 && h < 60) {
        rf = c; gf = x; bf = 0;
    } else if (h >= 60 && h < 120) {
        rf = x; gf = c; bf = 0;
    } else if (h >= 120 && h < 180) {
        rf = 0; gf = c; bf = x;
    } else if (h >= 180 && h < 240) {
        rf = 0; gf = x; bf = c;
    } else if (h >= 240 && h < 300) {
        rf = x; gf = 0; bf = c;
    } else {
        rf = c; gf = 0; bf = x;
    }

    *r = (rf + m) * 255;
    *g = (gf + m) * 255;
    *b = (bf + m) * 255;
}
