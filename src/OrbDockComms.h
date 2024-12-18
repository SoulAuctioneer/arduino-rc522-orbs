#ifndef ORBDOCKCOMMS_H
#define ORBDOCKCOMMS_H

#include "OrbDock.h"

class OrbDockComms : public OrbDock {
private:
    static uint8_t traitToInt(TraitId trait);
    static TraitId intToTrait(uint8_t value);

    uint8_t _orbPresentPin;
    uint8_t _energyLevelPin;
    uint8_t _toxicTraitPin;
    uint8_t _clearEnergyPin;

public:
    OrbDockComms(uint8_t orbPresentPin = 10, uint8_t energyLevelPin = 11, uint8_t toxicTraitPin = 9, uint8_t clearEnergyPin = 13);
    void begin() override;
    void loop() override;

protected:
    void onOrbConnected() override;
    void onOrbDisconnected() override;
    void onEnergyLevelChanged(byte newEnergy) override;
    void onError(const char* errorMessage) override;
    void onUnformattedNFC() override;
};

#endif // ORBDOCKCOMMS_H