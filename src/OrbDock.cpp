#include "OrbDock.h"
#include "NFCReader.h"


// Constructor
OrbDock::OrbDock(StationId id) :
    ledRing() {
    // Initialize member variables
    stationId = id;
    isNFCConnected = false;
    isOrbConnected = false;
    isUnformattedNFC = false;
    currentMillis = 0;
    ledRing.setPattern(LEDPatternId::NO_ORB, LEDPatternId::NO_ORB);
}

// Destructor
OrbDock::~OrbDock() {
    // Cleanup code if needed
}

void OrbDock::begin() {
    // Initialize LED ring
    ledRing.begin();

    // Initialize NFC reader
    if (!nfcReader.begin()) {
        // Flash red LED to indicate error
        while (1) {
            ledRing.update(TRAIT_COLORS[getTraitIndex()], orbInfo.energy, ALCHEMIZATION_ENERGY);
            delay(1000);
        }
    }

    Serial.print(F("Station: "));
    Serial.println(STATION_NAMES[stationId]);
    Serial.println(F("Put your orbs in me!"));
}

void OrbDock::loop() {
    currentMillis = millis();

    // Update LED patterns
    uint32_t traitColor = TRAIT_COLORS[getTraitIndex()];
    ledRing.update(traitColor, orbInfo.energy, ALCHEMIZATION_ENERGY);

    // Check for NFC / Orb presence periodically
    static unsigned long lastNFCCheckTime = 0;
    if (currentMillis - lastNFCCheckTime < NFC_CHECK_INTERVAL) {
        return;
    }
    lastNFCCheckTime = currentMillis;

    // First, check if previously connected NFC is still present
    if (isNFCConnected) {
        if (!isNFCPresent()) {
            // NFC has been removed, reset all states
            endOrbSession();
            return;
        }
    }

    // Check for new NFC presence
    if(!isNFCConnected && isNFCPresent()) {
        isNFCConnected = true;
        // NFC is present! Check if it's an orb
        int orbStatus = isOrb();
        switch (orbStatus) {
            case STATUS_FAILED:
                handleError("Failed to check orb header");
                endOrbSession();
                return;
            case STATUS_FALSE:
                if (!isUnformattedNFC) {
                    Serial.println(F("Unformatted NFC connected"));
                    isUnformattedNFC = true;
                    ledRing.setPattern(LEDPatternId::ERROR, LEDPatternId::ERROR);
                    onUnformattedNFC();
                }
                break;
            case STATUS_TRUE:
                isOrbConnected = true;
                ledRing.setPattern(LEDPatternId::ORB_CONNECTED, LEDPatternId::ORB_CONNECTED);
                readOrbInfo();
                setVisited(true);
                onOrbConnected();
                break;
        }
    }
}

bool OrbDock::isNFCPresent() {
    return nfcReader.isTagPresent();
}

// Check if an Orb NFC is connected by reading the orb page
bool OrbDock::isNFCActive() {
  // See if we can read the orb page
  int checkStatus = isOrb();
  if (checkStatus == STATUS_FAILED) {
    return false;
  }
  return true;
}

// Whether the connected NFC is formatted as an orb
int OrbDock::isOrb() {
    if (readPage(ORBS_PAGE) == STATUS_FAILED) {
        Serial.println(F("Failed to read data from NFC"));
        return STATUS_FAILED;
    }
    if (memcmp(page_buffer, ORBS_HEADER, 4) == 0) {
        return STATUS_TRUE;
    }
    Serial.println(F("ORBS header not found"));
    return STATUS_FALSE;
}

// Print station information
void OrbDock::printOrbInfo() {
    Serial.println(F("\n*************************************************"));
    Serial.print(F("Trait: "));
    Serial.println(getTraitName());
    Serial.print(F("Energy: "));
    Serial.println(orbInfo.energy);
    
    Serial.print(F("Visited:     "));
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (orbInfo.stations[i].visited) {
            Serial.print(STATION_NAMES[i]);
            Serial.print(F(" | "));
        }
    }
    Serial.println();
    Serial.print(F("Not visited: "));
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (!orbInfo.stations[i].visited) {
            Serial.print(STATION_NAMES[i]);
            Serial.print(F(" | "));
        }
    }
    Serial.println();
    Serial.println(F("*************************************************"));
    Serial.println();
}

void OrbDock::endOrbSession() {
    ledRing.setPattern(LEDPatternId::NO_ORB, LEDPatternId::NO_ORB);
    isOrbConnected = false;
    isNFCConnected = false;
    isUnformattedNFC = false;
    reInitializeStations();
    orbInfo.trait = TraitId::NONE;
    onOrbDisconnected();
}

int OrbDock::writeStation(int stationId) {
    // Prepare the page buffer with station data
    page_buffer[0] = orbInfo.stations[stationId].visited ? 1 : 0;
    page_buffer[1] = orbInfo.stations[stationId].custom;
    page_buffer[2] = 0;
    page_buffer[3] = 0;

    // Write the buffer to the NFC
    int writeDataStatus = writePage(STATIONS_PAGE_OFFSET + stationId, page_buffer);
    if (writeDataStatus == STATUS_FAILED) {
        Serial.println("Failed to write station");
        return STATUS_FAILED;
    }
    return STATUS_SUCCEEDED;
}

int OrbDock::writePage(int page, uint8_t* data) {
    if (nfcReader.writePage(page, data)) {
        return STATUS_SUCCEEDED;
    }
    return STATUS_FAILED;
}

int OrbDock::readPage(int page) {
    if (nfcReader.readPage(page, page_buffer)) {
        return STATUS_SUCCEEDED;
    }
    return STATUS_FAILED;
}

// Read and print the entire NFC storage
void OrbDock::printNFCStorage() {
    // Read the entire NFC storage
    for (int i = 0; i < 45; i++) {
        if (readPage(i) == STATUS_FAILED) {
            Serial.println(F("Failed to read page"));
            return;
        }
        Serial.print(F("Page "));
        Serial.print(i);
        Serial.print(F(": "));
        for (int j = 0; j < 4; j++) {
            Serial.print(page_buffer[j]);
            Serial.print(F(" "));
        }
        Serial.println();
    }
}

// Returns the trait name
const char* OrbDock::getTraitName() {
    int traitIndex = getTraitIndex();
    if (traitIndex < 0 || static_cast<size_t>(traitIndex) >= sizeof(TRAIT_NAMES)/sizeof(TRAIT_NAMES[0])) {
        Serial.print(F("ERROR: Invalid trait detected: "));
        Serial.println(traitIndex);
        ledRing.setPattern(LEDPatternId::ERROR, LEDPatternId::ERROR);
        return nullptr;
    }
    return TRAIT_NAMES[traitIndex];
}

int OrbDock::getTraitIndex() {
    return static_cast<int>(orbInfo.trait);
}

// Writes the trait to the orb
int OrbDock::setTrait(TraitId newTrait) {
    Serial.print(F("Setting trait to "));
    Serial.println(TRAIT_NAMES[static_cast<int>(newTrait)]);
    orbInfo.trait = newTrait;
    uint8_t traitBytes[4] = {static_cast<uint8_t>(newTrait), 0, 0, 0};  // Convert trait to bytes
    memcpy(page_buffer, traitBytes, 4);
    return writePage(TRAIT_PAGE, page_buffer);
}

int OrbDock::setVisited(bool visited) {
    Serial.print(F("Setting visited to "));
    Serial.print(visited ? "true" : "false");
    Serial.print(F(" for station "));
    Serial.println(STATION_NAMES[stationId]);
    orbInfo.stations[stationId].visited = visited;
    return writeStation(stationId);
}

int OrbDock::setEnergy(byte energy) {
    Serial.print(F("Setting energy to "));
    Serial.println(energy);
    orbInfo.energy = energy;
    byte energyBytes[4] = {energy, 0, 0, 0};  // Convert energy to bytes
    memcpy(page_buffer, energyBytes, 4);
    int result = writePage(ENERGY_PAGE, page_buffer);
    if (result == STATUS_SUCCEEDED) {
        LEDPatternId nextPattern = ledRing.getPattern();
        ledRing.setPattern(LEDPatternId::FLASH, nextPattern);
    }
    return result;
}

int OrbDock::addEnergy(byte amount) {
    byte newEnergy;
    if (amount > 250 - orbInfo.energy) {
        newEnergy = 250;
    } else {
        newEnergy = orbInfo.energy + amount;
    }
    Serial.print(F("Adding "));
    Serial.print(amount);
    Serial.println(F(" energy"));
    return setEnergy(newEnergy);
}

int OrbDock::removeEnergy(byte amount) {
    byte newEnergy;
    if (orbInfo.energy < amount) {
        newEnergy = 0;
    } else {
        newEnergy = orbInfo.energy - amount;
    }
    Serial.print(F("Removing "));
    Serial.print(amount);
    Serial.println(F(" energy"));
    return setEnergy(newEnergy);
}

int OrbDock::setCustom(byte value) {
    Serial.print(F("Setting custom to "));
    Serial.println(value);
    Serial.print(F(" for station "));
    Serial.println(STATION_NAMES[stationId]);
    orbInfo.stations[stationId].custom = value;
    int result = writeStation(stationId);
    return result;
}

Station OrbDock::getCurrentStationInfo() {
    return orbInfo.stations[stationId];
}

void OrbDock::handleError(const char* message) {
    Serial.println(message);
    onError(message);
}

// Formats the NFC with "ORBS" header, default station information and given trait
int OrbDock::formatNFC(TraitId trait) {
    Serial.println(F("Formatting NFC with ORBS header, default station information and given trait..."));
    
    // Store the trait we want to set
    orbInfo.trait = trait;
    
    // Write header
    memcpy(page_buffer, ORBS_HEADER, 4);
    if (writePage(ORBS_PAGE, page_buffer) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    
    // Write trait first
    if (setTrait(trait) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    
    // Then reset stations
    if (resetOrb() == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    
    // Write trait again to ensure it wasn't cleared
    if (setTrait(trait) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    
    // Write energy
    if (setEnergy(INIT_ENERGY) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    
    ledRing.setPattern(LEDPatternId::ORB_CONNECTED, LEDPatternId::ORB_CONNECTED);

    return STATUS_SUCCEEDED;
}

// Set the orb to default station information - zero energy, not visited
int OrbDock::resetOrb() {
    Serial.println("Initializing orb with default station information...");
    reInitializeStations();
    int status = writeStations();
    if (status == STATUS_FAILED) {
        Serial.println("Failed to reset orb");
        return STATUS_FAILED;
    }
    status = readOrbInfo();
    if (status == STATUS_FAILED) {
        Serial.println("Failed to reset orb");
        return STATUS_FAILED;
    }
    return STATUS_SUCCEEDED;
}

// Initialize stations information to default values
void OrbDock::reInitializeStations() {
    Serial.println(F("Initializing stations information to default values..."));
    for (int i = 0; i < NUM_STATIONS; i++) {
        orbInfo.stations[i] = {false, 0};
    }
}

// Read station information and trait from orb
int OrbDock::readOrbInfo() {
    Serial.println("Reading trait and station information from orb...");
    
    // Read stations
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (readPage(STATIONS_PAGE_OFFSET + i) == STATUS_FAILED) {
            Serial.println(F("Failed to read station information"));
            return STATUS_FAILED;
        }
        orbInfo.stations[i].visited = page_buffer[0] == 1;
        orbInfo.stations[i].custom = page_buffer[1];
    }

    // Read trait
    if (readPage(TRAIT_PAGE) == STATUS_FAILED) {
        Serial.println(F("Failed to read trait"));
        return STATUS_FAILED;
    }
    orbInfo.trait = static_cast<TraitId>(page_buffer[0]);

    // Read energy
    if (readPage(ENERGY_PAGE) == STATUS_FAILED) {
        Serial.println(F("Failed to read energy"));
        return STATUS_FAILED;
    }
    orbInfo.energy = page_buffer[0];

    printOrbInfo();
    return STATUS_SUCCEEDED;
}

// Write station information and trait to orb
int OrbDock::writeOrbInfo() {
    Serial.println("Writing stations to orb...");
    writeStations();
    setTrait(orbInfo.trait);
    setEnergy(orbInfo.energy);

    return STATUS_SUCCEEDED;
}

// Write station data
int OrbDock::writeStations() {
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (writeStation(i) == STATUS_FAILED) {
            return STATUS_FAILED;
        }
    }
    return STATUS_SUCCEEDED;
}

// Provide empty implementations for pure virtual functions to satisfy the linker
void OrbDock::onOrbConnected() {}
void OrbDock::onOrbDisconnected() {}
void OrbDock::onError(const char* errorMessage) {}
void OrbDock::onUnformattedNFC() {}
