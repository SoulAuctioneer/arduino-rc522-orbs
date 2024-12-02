#include "OrbDockHallSensor.h"

const CHSV OrbDockHallSensor::ORB_PRESENT_COLOR = CHSV(32, 255, 255);
const CHSV OrbDockHallSensor::NO_ORB_COLOR = CHSV(32, 255, 100);  // Golden color with lower value

OrbDockHallSensor::OrbDockHallSensor(int hallSensorPin) : 
    ledRing(HALL_LED_PATTERNS),
    hallSensorPin(hallSensorPin),
    isOrbPresent(false),
    lastCheckTime(0)
{
    // OH49E Linear Hall Effect Sensor Wiring:
    // - VCC → 5V
    // - GND → GND
    // - OUT → hallSensorPin (analog input)
}

void OrbDockHallSensor::begin() {
    ledRing.begin();
    ledRing.setPattern(COLOR_CHASE);
    
    // Initialize baseline by taking several readings
    int sum = 0;
    for (int i = 0; i < MOVING_AVG_SIZE; i++) {
        sum += analogRead(hallSensorPin);
        readings[i] = analogRead(hallSensorPin);
        delay(10);  // Small delay between readings
    }
    baselineValue = sum / MOVING_AVG_SIZE;
    
    Serial.print("Initial baseline value: ");
    Serial.println(baselineValue);
}

void OrbDockHallSensor::loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        
        int hallValue = analogRead(hallSensorPin);
        readings[readIndex] = hallValue;
        
        int sum = 0;
        for (int i = 0; i < MOVING_AVG_SIZE; i++) {
            sum += readings[i];
        }
        movingAverage = sum / MOVING_AVG_SIZE;
        
        readIndex = (readIndex + 1) % MOVING_AVG_SIZE;

        // Check for significant deviation from baseline
        int deviation = movingAverage - baselineValue;
        bool orbDetected = abs(deviation) > DETECTION_MARGIN;

        Serial.print("Hall: ");
        Serial.print(hallValue);
        Serial.print(" Avg: ");
        Serial.print(movingAverage);
        Serial.print(" Base: ");
        Serial.print(baselineValue);
        Serial.print(" Deviation: ");
        Serial.print(deviation);
        Serial.print(" Orb detected: ");
        Serial.println(orbDetected ? "true" : "false");
        
        if (orbDetected != isOrbPresent) {
            isOrbPresent = orbDetected;
            
            if (isOrbPresent) {
                ledRing.setPattern(SPARKLE_OUTWARD);
                ledRing.queuePattern(COLOR_CHASE);
            } else {
                ledRing.setPattern(COLOR_CHASE);  // Changed from RAINBOW_IDLE
            }
        }
    }
    
    // Update LED pattern with appropriate color and energy based on orb presence
    ledRing.update(isOrbPresent ? ORB_PRESENT_COLOR : NO_ORB_COLOR, 
                   isOrbPresent ? 50 : 1,  // Lower energy when no orb
                   255  // Max value
    );
}
