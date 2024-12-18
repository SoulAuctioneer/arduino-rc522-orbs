#include "OrbDockComms.h"
#include <Arduino.h>

OrbDockComms::OrbDockComms(uint8_t orbPresentPin, uint8_t energyLevelPin, uint8_t toxicTraitPin, uint8_t clearEnergyPin)
    : OrbDock(StationId::GENERIC),
    _orbPresentPin(orbPresentPin),
    _energyLevelPin(energyLevelPin),
    _toxicTraitPin(toxicTraitPin),
    _clearEnergyPin(clearEnergyPin)
{
}

void OrbDockComms::begin() {
    OrbDock::begin();
    
    // Configure input pin with pull-down (default LOW)
    pinMode(_clearEnergyPin, INPUT);
    
    // Configure output pins and set their initial states
    pinMode(_orbPresentPin, OUTPUT);
    pinMode(_energyLevelPin, OUTPUT);
    pinMode(_toxicTraitPin, OUTPUT);
    
    digitalWrite(_clearEnergyPin, LOW);  // switch on pull down
    digitalWrite(_orbPresentPin, LOW);
    analogWrite(_energyLevelPin, 0);
    analogWrite(_toxicTraitPin, 0);
}

void OrbDockComms::loop() {
    OrbDock::loop();

    if (digitalRead(_clearEnergyPin) == HIGH && isOrbConnected) {
        Serial.println(F("Orb Comms clearing energy"));
        setEnergy(0);
    }
}

void OrbDockComms::onOrbConnected() {
    OrbDock::onOrbConnected();
    digitalWrite(_orbPresentPin, HIGH);
    analogWrite(_energyLevelPin, orbInfo.energy);
    analogWrite(_toxicTraitPin, traitToInt(orbInfo.trait));
    // Serial.println(F("Orb Comms orb connected"));
    // Serial.print(F("Energy: "));
    // Serial.println(orbInfo.energy);
    // Serial.print(F("Toxic Trait: "));
    // Serial.println(TRAIT_NAMES[orbInfo.trait]);
}

void OrbDockComms::onOrbDisconnected() {
    OrbDock::onOrbDisconnected();
    digitalWrite(_orbPresentPin, LOW);
    analogWrite(_energyLevelPin, 0);
    analogWrite(_toxicTraitPin, 0);
    Serial.println(F("Orb Comms orb disconnected"));
}

void OrbDockComms::onEnergyLevelChanged(byte newEnergy) {
    analogWrite(_energyLevelPin, newEnergy);
}

void OrbDockComms::onError(const char* errorMessage) {
    OrbDock::onError(errorMessage);
}

void OrbDockComms::onUnformattedNFC() {
    OrbDock::onUnformattedNFC();
    Serial.println(F("Unformatted NFC, formatting to trait NONE / energy 42."));
    formatNFC(NONE);
    setEnergy(42);

    onOrbConnected();
}

uint8_t OrbDockComms::traitToInt(TraitId trait) {
    switch (trait) {
        case NONE:       return 25;
        case RUMINATE:   return 65;
        case SHAME:      return 105;
        case DOUBT:      return 145;
        case DISCONTENT: return 185;
        case HOPELESS:   return 225;
        default:         return 25;
    }
}

TraitId OrbDockComms::intToTrait(uint8_t value) {
    if (value < 45)  return NONE;
    if (value < 85)  return RUMINATE;
    if (value < 125) return SHAME;
    if (value < 165) return DOUBT;
    if (value < 205) return DISCONTENT;
    return HOPELESS;
}
