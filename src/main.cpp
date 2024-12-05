#include <Arduino.h>
#include <EEPROM.h>
#include "OrbDockHallSensor.h"
#include "FairyLights.h"

// EEPROM address to store dock type
const int DOCK_TYPE_ADDRESS = 0;
const uint8_t DOCK_TYPE_MARKER = 0xAA;  // Marker to check if value was set

// Function to write dock type to EEPROM
void writeDockType(DockType type) {
    EEPROM.write(DOCK_TYPE_ADDRESS, DOCK_TYPE_MARKER);
    EEPROM.write(DOCK_TYPE_ADDRESS + 1, type);
}

// Function to read dock type from EEPROM
DockType readDockType() {
    // Check if dock type was ever set
    if (EEPROM.read(DOCK_TYPE_ADDRESS) != DOCK_TYPE_MARKER) {
        // If not set, default to NEST_DOCK
        return NEST_DOCK;
    }
    
    // Read the stored dock type
    return (DockType)EEPROM.read(DOCK_TYPE_ADDRESS + 1);
}

// Initialize objects
FairyLights fairyLights;
OrbDockHallSensor orbDock{readDockType(), A0, &fairyLights};  // Pass fairyLights pointer

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    // Uncomment and upload once to set dock type, then comment out and upload again
    // writeDockType(NEST_DOCK);  // or NEST_DOCK
    
    Serial.print("Dock Type: ");
    Serial.println(readDockType() == POPCORN_DOCK ? "POPCORN" : "NEST");
    
    Serial.println("Starting OrbDock");
    orbDock.begin();
    Serial.println("OrbDock started");

    // Only initialize fairy lights for popcorn dock
    if (readDockType() == POPCORN_DOCK) {
        fairyLights.begin();
    }
}

void loop() {
    orbDock.loop();
    // Only update fairy lights for popcorn dock
    if (readDockType() == POPCORN_DOCK) {
        fairyLights.update();
    }
}
