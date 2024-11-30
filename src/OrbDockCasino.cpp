/**
 * Orb Casino Dock
 * 
 * For the button screen:
 * I2C pins:
 *  SDA: A4 (Pin 27)
 *  SCL: A5 (Pin 28)
 * Button Pins:
 *  S1: D7     // Add 1 energy
 *  S2: D8     // Add 5 energy  
 *  S3: D9    // Remove 1 energy
 *  S4: D10    // Remove 5 energy
 * 
 *  * orbInfo contains information on connected orb:
 * - trait (byte, one of TraitId enum)
 * - energy (byte, 0-250)
 * - stations[] (array of StationInfo structs, one for each station)
 * 
 * Available methods from base class:
 * - onOrbConnected() (override)
 * - onOrbDisconnected() (override)
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

class OrbDockCasino : public OrbDock {
private:
    const uint8_t* font = u8g_font_fub49n;
    ButtonDisplay display{font};

    void updateDisplay() {
        display.clearDisplay();
        
        if (isOrbConnected) {
            char energyStr[8];
            itoa(orbInfo.energy, energyStr, 10);
            display.println(energyStr);
        } else {
            display.println("::");
        }
        
        display.updateDisplay();
    }

public:
    OrbDockCasino() : OrbDock(StationId::CASINO) {
    }

    void begin() {
        OrbDock::begin();
        display.begin();
        updateDisplay();
    }

    void loop() override {
        OrbDock::loop();

        if (!isOrbConnected) return;

        // Handle button inputs
        if (display.isButton1Pressed()) {
            removeEnergy(5);
            updateDisplay();
        }
        
        if (display.isButton2Pressed()) {
            removeEnergy(1);
            updateDisplay();
        }

        if (display.isButton3Pressed()) {
            addEnergy(1);
            updateDisplay();
        }

        if (display.isButton4Pressed()) {
            addEnergy(5);
            updateDisplay();
        }
    }

protected:
    void onOrbConnected() override {
        // Add energy if station hasn't been visited
        if (!getCurrentStationInfo().visited) {
            addEnergy(10);
        }
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
        display.showError(":::::");
        delay(2000);
        updateDisplay();
    }
};
