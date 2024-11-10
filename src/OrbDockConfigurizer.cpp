/**
 * Orb Reset and Formatting Dock
 * 
* For the button screen:
 * I2C pins:
 *  SDA: A4 (Pin 27)
 *  SCL: A5 (Pin 28)
 * Button Pins:
 *  S1: D8     // Add 1 energy
 *  S2: D9     // Add 5 energy  
 *  S3: D10    // Remove 1 energy
 *  S4: D11    // Remove 5 energy
 * 
 *  * orbInfo contains information on connected orb:
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
#include "ButtonDisplay.h"

class OrbDockConfigurizer : public OrbDock {
private:

    // See https://github.com/olikraus/u8glib/wiki/fontsize
    // ... and 
    const uint8_t* font =  u8g_font_fub17; // u8g_font_osb21;
    ButtonDisplay display{font};
    TraitId selectedTrait;

    void updateDisplay() {
        display.clearDisplay();
        
        if (isOrbConnected) {
            char shortName[9];
            strncpy(shortName, TRAIT_NAMES[selectedTrait], 8);
            shortName[8] = '\0';
            display.println(shortName);
            display.println(TRAIT_COLOR_NAMES[selectedTrait]);
        } else {
            char shortName[9];
            strncpy(shortName, TRAIT_NAMES[selectedTrait], 8);
            shortName[8] = '\0';
            display.println(shortName);
            display.println(TRAIT_COLOR_NAMES[selectedTrait]);
        }
        
        display.updateDisplay();
    }

public:
    OrbDockConfigurizer() : OrbDock(StationId::CONFIGURE) {
        selectedTrait = TraitId::RUMINATE;
    }

    void begin() {
        OrbDock::begin();
        display.begin();
        updateDisplay();
    }

    void loop() override {
        OrbDock::loop();

        // Handle button inputs
        if (display.isButton1Pressed()) {
            int trait = static_cast<int>(selectedTrait) + 1;
            if (trait >= NUM_TRAITS) trait = 0;
            selectedTrait = static_cast<TraitId>(trait);
            Serial.print(F("Next trait: "));
            Serial.println(TRAIT_NAMES[selectedTrait]);
            delay(200); // Simple debounce
            updateDisplay();
        }
        
        if (display.isButton2Pressed()) {
            int trait = static_cast<int>(selectedTrait) - 1;
            if (trait < 0) trait = NUM_TRAITS - 1;
            selectedTrait = static_cast<TraitId>(trait);
            Serial.print(F("Previous trait: "));
            Serial.println(TRAIT_NAMES[selectedTrait]);
            delay(200); // Simple debounce
            updateDisplay();
        }

        if (display.isButton3Pressed() && isOrbConnected) {
            Serial.println(F("Reset orb"));
            resetOrb();
            delay(200);
            updateDisplay();
        }

        if (display.isButton4Pressed() && isNFCConnected) {
            Serial.println(F("Format orb"));
            formatNFC(selectedTrait);
            delay(200);
            updateDisplay();
        }
    }

protected:
    void onOrbConnected() override {
        updateDisplay();
    }

    void onOrbDisconnected() override {
        updateDisplay();
    }

    void onError(const char* errorMessage) override {
        display.showError(errorMessage);
        delay(2000);
        updateDisplay();
    }

    void onUnformattedNFC() override {
        Serial.println(F("Unformatted NFC, will format with selected trait."));
        formatNFC(selectedTrait);
    }
};
