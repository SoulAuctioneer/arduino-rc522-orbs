/**
 * OrbDockLedStrip implementation that controls an LED strip based on orb state
 * and displays different patterns when orbs are connected/disconnected
 * 
 * orbInfo contains information on connected orb:
 * - trait (byte, one of TraitId enum)
 * - energy (byte, 0-250)
 * - stations[] (array of StationInfo structs, one for each station)
 * 
 * Available methods from base class:
 * - onOrbConnected() (override)
 * - onOrbDisconnected() (override) 
 * - onEnergyLevelChanged(byte newEnergy) (override)
 * - onError(const char* errorMessage) (override)
 * - onUnformattedNFC() (override)
 * 
 * - addEnergy(byte amount)
 * - setEnergy(byte amount)
 * - getTraitName()
 * - getTraitColor()
 * - printOrbInfo()
 */

#include "OrbDock.h"
#include <FastLED.h>


#define NUM_LEDS 160
#define LED_STRIP_PIN 7
#define MAX_BRIGHTNESS 80
#define MIN_BRIGHTNESS 20

class OrbDockLedDistiller : public OrbDock {
private:
    CRGB leds[NUM_LEDS];
    bool showPrideEffect = true;
    CRGB color = CRGB::Black;
    uint16_t sLastMillis = 0;
    bool inTransition = false;
    uint32_t transitionStartTime = 0;
    const uint32_t TRANSITION_DURATION = 6000; // 6 seconds

public:
    OrbDockLedDistiller() : OrbDock(StationId::DISTILLER) {

    }

 void begin() {
        OrbDock::begin();
        FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(MAX_BRIGHTNESS);
        FastLED.show();
        Serial.println(F("Running OrbDockLedDistiller"));
        //fill_solid(leds, NUM_LEDS, CRGB::Green);
        //FastLED.show();
    }

    void loop() override {
        OrbDock::loop();
        if (showPrideEffect) {
            pride2015(leds, NUM_LEDS);
            FastLED.show();
        }else{
            // strobeWhiteToColorTransition(color, leds, NUM_LEDS);
            flickerFlame(color);
            //lightShow(color);
            FastLED.show();
        }
    }

protected:
    void onOrbConnected() override {
        Serial.println(F("OrbDockLedDistiller Connected"));
        showPrideEffect = false;

        // Add energy if this station hasn't been visited yet
        if (!getCurrentStationInfo().visited) {
            addEnergy(10);
        }

        // Get trait color from TRAIT_COLORS array using orbInfo.trait as index
        uint32_t traitColor = TRAIT_COLORS[static_cast<int>(orbInfo.trait)];
        
        // Extract RGB components from 32-bit color
        uint8_t r = (traitColor >> 16) & 0xFF;
        uint8_t g = (traitColor >> 8) & 0xFF; 
        uint8_t b = traitColor & 0xFF;

        // Set color using RGB values
        color = CRGB(r, g, b);

        inTransition = true;
    }

    void onOrbDisconnected() override {
        Serial.println(F("Orb disconnected"));
        transitionStartTime = 0;
        inTransition = false;
        showPrideEffect = true;
    }

    void onError(const char* errorMessage) override {
        // Serial.print(F("Error: "));
        // Serial.println(errorMessage);
        
        // // Flash red on error
        // for(int i = 0; i < 3; i++) {
        //     fill_solid(leds, NUM_LEDS, CRGB::Red);
        //     FastLED.show();
        //     delay(200);
        //     fill_solid(leds, NUM_LEDS, CRGB::Black);
        //     FastLED.show();
        //     delay(200);
        // }
    }

    void onUnformattedNFC() override {
        Serial.println(F("Unformatted NFC detected"));
        
        // // Pulse white for unformatted NFC
        // for(int brightness = 0; brightness < 255; brightness++) {
        //     fill_solid(leds, NUM_LEDS, CRGB::White);
        //     FastLED.setBrightness(brightness);
        //     FastLED.show();
        //     delay(5);
        // }
        // for(int brightness = 255; brightness >= 0; brightness--) {
        //     fill_solid(leds, NUM_LEDS, CRGB::White);
        //     FastLED.setBrightness(brightness);
        //     FastLED.show();
        //     delay(5);
        // }
        // FastLED.setBrightness(50);
    }

    // Helper function for pride effect - Creates a rainbow animation with smooth transitions
    void pride2015(CRGB* ledArray, uint16_t numLeds) {
        // Track timing and hue state between function calls
        static uint16_t sPseudotime = 0;
        static uint16_t sLastMillis = 0;
        static uint16_t sHue16 = 0;

        // Generate smooth sine wave values for saturation and brightness parameters
        uint8_t sat8 = beatsin88(87, 220, 250);  // Saturation oscillates between 220-250
        uint8_t brightdepth = beatsin88(341, 96, 224);  // Controls depth of brightness modulation
        uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));  // Controls speed of brightness waves
        uint8_t msmultiplier = beatsin88(147, 23, 60);  // Time scaling factor

        // Set up hue parameters for color cycling
        uint16_t hue16 = sHue16;  // Current base hue value
        uint16_t hueinc16 = beatsin88(113, 1, 3000);  // How much to increment hue each pixel

        // Calculate time delta since last update
        uint16_t ms = millis();
        uint16_t deltams = ms - sLastMillis;
        sLastMillis = ms;
        
        // Update timing variables
        sPseudotime += deltams * msmultiplier;  // Advance animation time
        sHue16 += deltams * beatsin88(400, 5, 9);  // Slowly shift base hue over time
        uint16_t brightnesstheta16 = sPseudotime;  // Starting point for brightness wave

        for (uint16_t i = 0; i < numLeds; i++) {
            // Increment hue for each LED to create rainbow pattern
            hue16 += hueinc16;
            uint8_t hue8 = hue16 / 256;  // Convert 16-bit hue to 8-bit

            // Calculate brightness wave value for this LED
            brightnesstheta16 += brightnessthetainc16;
            uint16_t b16 = sin16(brightnesstheta16) + 32768;  // Generate sine wave

            // Apply multiple brightness modulations
            uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
            uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
            bri8 += (255 - brightdepth);
            
            // Map brightness to desired range
            uint8_t new_min = MIN_BRIGHTNESS;
            uint8_t new_max = MAX_BRIGHTNESS;
            uint8_t old_min = 96;
            uint8_t old_max = 224;
            bri8 += new_min + (bri8 - old_min) * (new_max - new_min) / (old_max - old_min);

            // Create new color with calculated hue, saturation and brightness
            CRGB newcolor = CHSV(hue8, sat8, bri8);

            // Reverse LED order for display
            uint16_t pixelnumber = i;
            pixelnumber = (numLeds - 1) - pixelnumber;

            // Blend new color with existing color for smooth transitions
            nblend(ledArray[pixelnumber], newcolor, 64);
        }
    }

    
    // Creates a flickering flame effect using the given base color
    void flickerFlame(CRGB baseColor) {
        // Track timing between updates and transition state
        static uint16_t sLastMillis = 0;
        
        uint16_t ms = millis();
        uint16_t deltams = ms - sLastMillis;

        // Start transition if we haven't yet
        if (inTransition && transitionStartTime == 0) {
            transitionStartTime = ms;
            fill_solid(leds, NUM_LEDS, CRGB::Black);
        }

        // Handle transition phase
        if (inTransition) {
            uint32_t elapsed = ms - transitionStartTime;
            
            if (elapsed >= TRANSITION_DURATION) {
                // Transition complete, switch to normal flame effect
                inTransition = false;
                transitionStartTime = 0;
            } else {
                // Calculate how many sparkles based on transition progress
                float progress = float(elapsed) / TRANSITION_DURATION;
                int numSparkles = int(NUM_LEDS * progress * 0.5); // Max 50% of LEDs can sparkle at once
                
                // Clear previous frame
                fadeToBlackBy(leds, NUM_LEDS, 200);
                
                // Add random white sparkles with brightness capped at MAX_BRIGHTNESS
                for (int i = 0; i < numSparkles; i++) {
                    int pos = random16(NUM_LEDS);
                    if (random8() < 180) { // 70% chance of sparkle
                        // Create white sparkle scaled to MAX_BRIGHTNESS
                        leds[pos] = CRGB(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
                        // Apply random fade while maintaining relative color ratios
                        uint8_t fade = random8(50, 150);
                        leds[pos].fadeToBlackBy(fade);
                    }
                }
                
                // Gradually blend in base color
                if (progress > 0.7) { // Start color blend in last 30% of transition
                    float colorBlend = (progress - 0.7) / 0.3; // 0 to 1 over last 30%
                    for (int i = 0; i < NUM_LEDS; i++) {
                        leds[i] = blend(leds[i], baseColor, colorBlend * 255);
                    }
                }
            }
            FastLED.show();
            return;
        }

        // Normal flame effect (existing code)
        if (deltams >= 150) {
            sLastMillis = ms;
            
            for (int i = 0; i < NUM_LEDS; i++) {
                int flicker = random(MIN_BRIGHTNESS, MAX_BRIGHTNESS);
                leds[i] = baseColor;
                leds[i].r = (leds[i].r * flicker) / 255;
                leds[i].g = (leds[i].g * flicker) / 255;
                leds[i].b = (leds[i].b * flicker) / 255;
            }
        }
    }

/* //still in development
    void lightShow(CRGB color) {
        // currentStep controls which animation phase we're in:
        // 0: Random blinking of individual LEDs
        // 1: Sequential LED activation from start to end
        // 2: All LEDs blinking together
        // 3: Fade up brightness to maximum
        // 4: Final steady state with all LEDs on
        static uint8_t currentStep = 0;
        static uint16_t sLastMillis = 0;
        static uint16_t stepCounter = 0;
        static uint16_t ledIndex = 0;
        static uint8_t brightness = 0;
        
        uint16_t ms = millis();
        uint16_t deltams = ms - sLastMillis;

        switch(currentStep) {

            case 0: // Random blinking of individual LEDs
                if (deltams >= random(5, 10)) {
                    sLastMillis = ms;
                    if (stepCounter < 10 * NUM_LEDS) {
                        ledIndex = stepCounter % NUM_LEDS;
                        if ((stepCounter / NUM_LEDS) % 2 == 0) {
                            leds[ledIndex] = color;
                        } else {
                            leds[ledIndex] = CRGB::Black;
                        }
                        stepCounter++;
                    } else {
                        currentStep++;
                        stepCounter = 0;
                    }
                }
                break;

            case 1: // Sequential LED activation
                if (deltams >= 10) {
                    sLastMillis = ms;
                    if (stepCounter < NUM_LEDS) {
                        leds[stepCounter] = color;
                        stepCounter++;
                    } else {
                        currentStep++;
                        stepCounter = 0;
                    }
                }
                break;

            case 2: // All LEDs blink together
                if (deltams >= 30) {
                    sLastMillis = ms;
                    if (stepCounter < 10) { // 5 on/off cycles
                        if (stepCounter % 2 == 0) {
                            fill_solid(leds, NUM_LEDS, color);
                        } else {
                            fill_solid(leds, NUM_LEDS, CRGB::Black);
                        }
                        stepCounter++;
                    } else {
                        currentStep++;
                        stepCounter = 0;
                        brightness = 0;
                    }
                }
                break;

            case 3: // Fade up brightness
                if (deltams >= 50) {
                    sLastMillis = ms;
                    if (brightness <= MAX_BRIGHTNESS) {
                        FastLED.setBrightness(brightness);
                        brightness += 5;
                    } else {
                        currentStep++;
                    }
                }
                break;

            case 4: // Final steady state
                fill_solid(leds, NUM_LEDS, color);
                break;
        }
        FastLED.show();
    } */

};

