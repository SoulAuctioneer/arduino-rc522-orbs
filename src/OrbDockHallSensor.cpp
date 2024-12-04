#include "OrbDockHallSensor.h"

const CHSV OrbDockHallSensor::ORB_PRESENT_COLOR = CHSV(32, 255, 255);
const CHSV OrbDockHallSensor::NO_ORB_COLOR = CHSV(32, 255, 100);  // Golden color with lower value

OrbDockHallSensor::OrbDockHallSensor(DockType type, int hallSensorPin) : 
    ledRing(type == NEST_DOCK ? NEST_LED_PATTERNS : POPCORN_LED_PATTERNS),
    hallSensorPin(hallSensorPin),
    isOrbPresent(false),
    lastCheckTime(0),
    dockType(type)
{
    // Constructor should only initialize member variables
}

void OrbDockHallSensor::begin() {
    ledRing.begin();
    ledRing.setPattern(PULSE);
    
    // Initialize moving average and set baseline
    Serial.println("Reading hall sensor baseline values:");
    int sum = 0;
    for (int i = 0; i < MOVING_AVG_SIZE; i++) {
        readings[i] = analogRead(hallSensorPin);
        sum += readings[i];
        Serial.print("Reading ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(readings[i]);
        delay(10);
    }
    baselineValue = sum / MOVING_AVG_SIZE;
    movingAverage = baselineValue;
    Serial.print("Baseline value: ");
    Serial.println(baselineValue);
}

void OrbDockHallSensor::loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        
        // Get new reading
        int hallValue = analogRead(hallSensorPin);
        
        // Update moving average
        readings[readIndex] = hallValue;
        readIndex = (readIndex + 1) % MOVING_AVG_SIZE;
        
        // Calculate average
        long sum = 0;
        for (int i = 0; i < MOVING_AVG_SIZE; i++) {
            sum += readings[i];
        }
        int newAverage = sum / MOVING_AVG_SIZE;
        
        // Calculate change from baseline instead of previous average
        int change = abs(newAverage - baselineValue);
        
        // Use appropriate detection margin
        int detectionMargin = (dockType == POPCORN_DOCK) ? 
                             POPCORN_DETECTION_MARGIN : 
                             NEST_DETECTION_MARGIN;
        
        // Check if change exceeds threshold
        bool orbDetected = (change > detectionMargin);
        
        Serial.print("Value: ");
        Serial.print(hallValue);
        Serial.print(" Avg: ");
        Serial.print(movingAverage);
        Serial.print(" New Avg: ");
        Serial.print(newAverage);
        Serial.print(" Change: ");
        Serial.print(change);
        Serial.print(" Detected: ");
        Serial.println(orbDetected ? "true" : "false");
        
        // Update state if changed
        if (orbDetected != isOrbPresent) {
            isOrbPresent = orbDetected;
            
            // Update LED patterns
            if (isOrbPresent) {
                ledRing.setPattern(SPARKLE_OUTWARD);
                if (dockType == NEST_DOCK) {
                    ledRing.queuePattern(COLOR_CHASE);
                } else {
                    ledRing.queuePattern(SPARKLE_OUTWARD);
                }
            } else {
                ledRing.setPattern(PULSE);
            }
        }
        
        // Update moving average
        movingAverage = newAverage;
        
        // Update LED pattern
        ledRing.update(isOrbPresent ? ORB_PRESENT_COLOR : NO_ORB_COLOR, 
                      isOrbPresent ? 50 : 30,
                      255);
    }
}
