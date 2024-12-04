#include <Arduino.h>

//#include "OrbDockConfigurizer.cpp"
//OrbDockConfigurizer orbDock{};

//#include "OrbDockBasic.cpp"
//OrbDockBasic orbDock{};

//#include "OrbDockCasino.cpp"
//OrbDockCasino orbDock{};

//#include "OrbDockComms.h"
//OrbDockComms orbDock(10, 11, 9, 13);

//#include "OrbDockLedDistiller.cpp"
//OrbDockLedDistiller orbDock{};

#include "OrbDockHallSensor.h"
//OrbDockHallSensor orbDock{POPCORN_DOCK};
OrbDockHallSensor orbDock{NEST_DOCK};

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    Serial.println("Starting OrbDock");
    orbDock.begin();
    Serial.println("OrbDock started");
}

void loop() {
    orbDock.loop();
}
