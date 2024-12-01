#include "OrbDockHallSensor.h"

OrbDockHallSensor::OrbDockHallSensor(int hallSensorPin) : 
    ledRing(HALL_LED_PATTERNS),
    hallSensorPin(hallSensorPin),
    isOrbPresent(false),
    lastCheckTime(0)
{
    // KY-003 Hall Effect Sensor Wiring:
    // - VCC → 5V
    // - GND → GND
    // - SIGNAL → hallSensorPin (digital input)
}

void OrbDockHallSensor::begin() {
    // Initialize LED ring
    ledRing.begin();
    
    // Setup hall sensor pin
    pinMode(hallSensorPin, INPUT);
    
    // Start with NO_ORB pattern
    ledRing.setPattern(NO_ORB, NO_ORB);
    
    // Debug output
    Serial.println("Hall LED Pattern Intervals:");
    Serial.print("NO_ORB interval: ");
    Serial.println(HALL_LED_PATTERNS[NO_ORB].interval);
    Serial.print("ORB_CONNECTED interval: ");
    Serial.println(HALL_LED_PATTERNS[ORB_CONNECTED].interval);
}

void OrbDockHallSensor::loop() {
    unsigned long currentMillis = millis();
    
    // Check hall sensor periodically
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        
        // Read hall sensor (LOW when magnet is present)
        bool orbDetected = (digitalRead(hallSensorPin) == HIGH);
        Serial.print("Orb detected: ");
        Serial.println(orbDetected);
        
        // If orb state changed
        if (orbDetected != isOrbPresent) {
            isOrbPresent = orbDetected;
            
            if (isOrbPresent) {
                // Show orange/yellow pattern when orb is present
                ledRing.setPattern(FLASH, ORB_CONNECTED);
            } else {
                // Show default pattern when no orb
                ledRing.setPattern(NO_ORB, NO_ORB);
            }
        }
    }
    
    // Update LED pattern
    ledRing.update(isOrbPresent ? ORB_PRESENT_COLOR : 0xFF0000, 
                   50,  // Lower energy value for slower movement
                   255  // Max value
    );
}
