/**
 * Basic OrbDock implementation that just prints to Serial,
 * and adds 1 energy to the orb when it's connected
 * 
 * orbInfo contains information on connected orb:
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

class OrbDockBasic : public OrbDock {
public:
    OrbDockBasic() : OrbDock(StationId::GENERIC) {
    }

protected:
    void onOrbConnected() override {
        Serial.println(F("Orb connected"));
        if (!getCurrentStationInfo().visited) {
            addEnergy(5);
        }
    }

    void onOrbDisconnected() override {
        Serial.println(F("Orb disconnected")); 
    }

    void onError(const char* errorMessage) override {
        Serial.print(F("Error: "));
        Serial.println(errorMessage);
    }

    void onUnformattedNFC() override {
        Serial.println(F("Unformatted NFC detected"));
    }
};