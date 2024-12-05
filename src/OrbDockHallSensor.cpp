#include "OrbDockHallSensor.h"

const CHSV OrbDockHallSensor::ORB_PRESENT_COLOR = CHSV(32, 255, 255);
const CHSV OrbDockHallSensor::NO_ORB_COLOR = CHSV(32, 255, 100);  // Golden color with lower value

OrbDockHallSensor::OrbDockHallSensor(DockType type, int hallSensorPin, FairyLights* fairyLights) : 
    ledRing(type == NEST_DOCK ? NEST_LED_PATTERNS : POPCORN_LED_PATTERNS),
    hallSensorPin(hallSensorPin),
    isOrbPresent(false),
    lastCheckTime(0),
    dockType(type),
    fairyLights(fairyLights)
{
    // Constructor should only initialize member variables
}

void OrbDockHallSensor::begin() {
    pinMode(ORB_PRESENT_PIN, OUTPUT);
    digitalWrite(ORB_PRESENT_PIN, LOW);  // Initialize as LOW (no orb)
    
    ledRing.begin();
    ledRing.setPattern(WARM_GOLD_ROTATE);
    
    // Calibrate baseline with more samples
    Serial.println("Calibrating baseline...");
    long sum = 0;
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        sum += analogRead(hallSensorPin);
        delay(10);
    }
    baselineValue = sum / CALIBRATION_SAMPLES;
    
    // Initialize moving average buffer with baseline
    for (int i = 0; i < MOVING_AVG_SIZE; i++) {
        readings[i] = baselineValue;
    }
    movingAverage = baselineValue;
    
    Serial.print("Baseline value: ");
    Serial.println(baselineValue);
}

void OrbDockHallSensor::loop() {
    unsigned long currentMillis = millis();
    
    // Check hall sensor at slower interval
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        
        // Get new reading and update moving average
        int hallValue = analogRead(hallSensorPin);
        movingAverage -= readings[readIndex] / MOVING_AVG_SIZE;
        readings[readIndex] = hallValue;
        movingAverage += hallValue / MOVING_AVG_SIZE;
        readIndex = (readIndex + 1) % MOVING_AVG_SIZE;
        
        // Compare moving average to baseline
        int change = abs(movingAverage - baselineValue);
        
        // Use appropriate detection margin
        int detectionMargin = (dockType == POPCORN_DOCK) ? 
                             POPCORN_DETECTION_MARGIN : 
                             NEST_DETECTION_MARGIN;
        
        // Check if change exceeds threshold
        bool orbDetected = (change > detectionMargin);
        
        Serial.print("Value: ");
        Serial.print(hallValue);
        Serial.print(" MovAvg: ");
        Serial.print(movingAverage);
        Serial.print(" Baseline: ");
        Serial.print(baselineValue);
        Serial.print(" Change: ");
        Serial.print(change);
        Serial.print(" Detected: ");
        Serial.println(orbDetected ? "true" : "false");
        
        // Update state if changed
        if (orbDetected != isOrbPresent) {
            isOrbPresent = orbDetected;
            
            // Add this line to control D2
            digitalWrite(ORB_PRESENT_PIN, isOrbPresent ? HIGH : LOW);
            
            // Trigger fairy lights if this is a popcorn dock and an orb was just detected
            if (dockType == POPCORN_DOCK && fairyLights != nullptr) {
                fairyLights->setActive(isOrbPresent);
            }
            
            // Update LED patterns
            if (isOrbPresent) {
                ledRing.setPattern(SPARKLE_OUTWARD);
                ledRing.queuePattern(COLOR_CHASE);
            } else {
                ledRing.setPattern(WARM_GOLD_ROTATE);
            }
        }
    }
    
    // Update LEDs at faster interval (independent of sensor checks)
    ledRing.update(isOrbPresent ? ORB_PRESENT_COLOR : NO_ORB_COLOR, 
                  isOrbPresent ? 50 : 30,
                  255);
}
