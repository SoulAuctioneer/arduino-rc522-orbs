#include "OrbDockComms.h"
#include <Arduino.h>

uint8_t scaleTraitValue(uint8_t traitValue) {
    // Scale from 0-4 to 0-255
    return (traitValue * 255) / 4;
}

OrbDockComms::OrbDockComms(uint8_t orbPresentPin, uint8_t energyLevelPin, uint8_t toxicTraitPin, uint8_t clearEnergyPin)
    : OrbDock(StationId::GENERIC),
    _orbPresentPin(orbPresentPin),
    _energyLevelPin(energyLevelPin),
    _toxicTraitPin(toxicTraitPin),
    _clearEnergyPin(clearEnergyPin)
{

    // Single F() string with one print
    // Serial.print(F("OrbDockComms initialized - Present Pin: "));
    // Serial.print(orbPresentPin);
    // Serial.print(F(" Level Pin: "));
    // Serial.print(_energyLevelPin);
    // Serial.print(F(" Trait Pin: "));
    // Serial.println(_toxicTraitPin);
}

void OrbDockComms::begin() {
    OrbDock::begin();
    pinMode(_orbPresentPin, OUTPUT);
    pinMode(_energyLevelPin, OUTPUT);
    pinMode(_toxicTraitPin, OUTPUT);
    pinMode(_clearEnergyPin, INPUT);
    digitalWrite(_orbPresentPin, LOW);
    analogWrite(_energyLevelPin, 0);
    analogWrite(_toxicTraitPin, 0);
    
    // Serial.println(F("OrbDockComms initialized"));
}

void OrbDockComms::loop() {
    OrbDock::loop();

    if (digitalRead(_clearEnergyPin) == HIGH && isOrbConnected) {
        setEnergy(0);
    }
}

void OrbDockComms::onOrbConnected() {
    OrbDock::onOrbConnected();
    digitalWrite(_orbPresentPin, HIGH);
    analogWrite(_energyLevelPin, orbInfo.energy);
    analogWrite(_toxicTraitPin, scaleTraitValue(static_cast<int>(orbInfo.trait)));

    // analogWrite(_energyLevelPin, 90);
    // analogWrite(_toxicTraitPin, 4);
    // Serial.print(F("Orb Comms sending: Orb Present = HIGH, Energy = "));
    // Serial.print(orbInfo.energy);
    // Serial.print(F(", Trait = "));
    // Serial.println(static_cast<int>(orbInfo.trait));
}

void OrbDockComms::onOrbDisconnected() {
    OrbDock::onOrbDisconnected();
    digitalWrite(_orbPresentPin, LOW);
    analogWrite(_energyLevelPin, 0);
    analogWrite(_toxicTraitPin, 0);
    // Serial.println(F("Orb Comms sending: Orb Present = LOW, Energy = 0, Trait = 0"));
}

void OrbDockComms::onEnergyLevelChanged(byte newEnergy) {
    analogWrite(_energyLevelPin, newEnergy);
    // Serial.print(F("Orb Comms sending: Energy = "));
    // Serial.println(newEnergy);
}

void OrbDockComms::onError(const char* errorMessage) {
    OrbDock::onError(errorMessage);
}

void OrbDockComms::onUnformattedNFC() {
    OrbDock::onUnformattedNFC();
}
