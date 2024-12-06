#include "OrbDockHallSensor.h"

const CHSV OrbDockHallSensor::ORB_PRESENT_COLOR = CHSV(32, 255, 255);
const CHSV OrbDockHallSensor::NO_ORB_COLOR = CHSV(32, 255, 100);  // Golden color with lower value

OrbDockHallSensor::OrbDockHallSensor(DockType type, int hallSensorPin, FairyLights* fairyLights) : 
    ledRing(type == NEST_DOCK ? NEST_LED_PATTERNS : POPCORN_LED_PATTERNS),
    hallSensorPin(hallSensorPin),
    lastCheckTime(0),
    dockType(type),
    fairyLights(fairyLights)
{
    // Constructor is now cleaner
}

void OrbDockHallSensor::begin() {
    pinMode(ORB_PRESENT_PIN, OUTPUT);
    digitalWrite(ORB_PRESENT_PIN, LOW);  // Initialize as LOW (no orb)
    
    ledRing.begin();
    ledRing.setPattern(WARM_GOLD_ROTATE);
    
    // Calibrate baseline with samples
    Serial.println("Calibrating baseline...");
    long sum = 0;
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        sum += analogRead(hallSensorPin);
        delay(10);
    }
    baselineValue = sum / CALIBRATION_SAMPLES;
    
    Serial.print("Baseline value: ");
    Serial.println(baselineValue);
}

void OrbDockHallSensor::loop() {
    unsigned long currentMillis = millis();
    
    // Check hall sensor at slower interval
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        
        // Get new reading
        int hallValue = analogRead(hallSensorPin);
        
        // Compare value to baseline
        int change = abs(hallValue - baselineValue);
        
        // Use appropriate detection margin
        int detectionMargin = (dockType == POPCORN_DOCK) ? 
                             POPCORN_DETECTION_MARGIN : 
                             NEST_DETECTION_MARGIN;
        
        // Simple threshold detection
        bool orbDetected = (change > detectionMargin);
        
        // Update state and baseline if changed
        if (orbDetected != isOrbPresent) {
            isOrbPresent = orbDetected;
            
            // Update outputs
            digitalWrite(ORB_PRESENT_PIN, isOrbPresent ? HIGH : LOW);
            
            if (dockType == POPCORN_DOCK && fairyLights != nullptr) {
                fairyLights->setActive(isOrbPresent);
            }
            
            if (isOrbPresent) {
                ledRing.setPattern(SPARKLE_OUTWARD);
                ledRing.queuePattern(COLOR_CHASE);
            } else {
                ledRing.setPattern(WARM_GOLD_ROTATE);
            }
        }
        
        // Debug output
        Serial.print("Value: ");
        Serial.print(hallValue);
        Serial.print(" Baseline: ");
        Serial.print(baselineValue);
        Serial.print(" Change: ");
        Serial.print(change);
        Serial.print(" Detected: ");
        Serial.println(isOrbPresent ? "true" : "false");
    }
    
    // Update LEDs
    ledRing.update(isOrbPresent ? ORB_PRESENT_COLOR : NO_ORB_COLOR, 
                  isOrbPresent ? 50 : 30,
                  255);
}
