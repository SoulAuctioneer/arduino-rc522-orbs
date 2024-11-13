#include "OrbDock.h"


// Constructor
OrbDock::OrbDock(StationId id) :
    strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
    nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS) {
    // Initialize member variables
    stationId = id;
    isNFCConnected = false;
    isOrbConnected = false;
    isUnformattedNFC = false;
    currentMillis = 0;
    setLEDPattern(LED_PATTERN_NO_ORB);
}

// Destructor
OrbDock::~OrbDock() {
    // Cleanup code if needed
}

void OrbDock::begin() {
    // Initialize NeoPixel strip
    strip.begin();
    strip.setBrightness(0);
    strip.show();

    // Try to initialize NFC with default pins
    Serial.println(F("Initializing PN532 NFC reader with latest dock pins..."));
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    
    // If default pins don't work, try later dock design pins
    if (!versiondata) {
        Serial.println(F("Latest dock pins failed, trying V2 dock pins..."));
        nfc = Adafruit_PN532(PN532_SCK2, PN532_MISO2, PN532_MOSI2, PN532_SS2); // Later dock design pins
        nfc.begin();
        versiondata = nfc.getFirmwareVersion();
        
        // If that fails too, try newest pins
        if (!versiondata) {
            Serial.println(F("V2 dock pins failed, trying v1 dock pins..."));
            nfc = Adafruit_PN532(PN532_SCK1, PN532_MISO1, PN532_MOSI1, PN532_SS1);
            nfc.begin();
            versiondata = nfc.getFirmwareVersion();
            
            // If all pin configurations fail
            if (!versiondata) {
                Serial.println(F("Didn't find PN53x board with any pin configuration"));
                // Flash red LED to indicate error
                while (1) {
                    strip.setPixelColor(0, 255, 0, 0); // Red
                    strip.show();
                    delay(1000);
                    strip.setPixelColor(0, 0, 0, 0); // Off
                    strip.show(); 
                    delay(1000);
                }
            }
        }
    }

    nfc.SAMConfig();                        // Configure the PN532 to read RFID tags
    nfc.setPassiveActivationRetries(0x11);  // Set the max number of retry attempts to read from a card

    Serial.print(F("Station: "));
    Serial.println(STATION_NAMES[stationId]);
    Serial.println(F("Put your orbs in me!"));
}

void OrbDock::loop() {
    currentMillis = millis();

    // Run LED patterns
    runLEDPatterns();

    // Check for NFC / Orb presence periodically
    static unsigned long lastNFCCheckTime = 0;
    if (currentMillis - lastNFCCheckTime < NFC_CHECK_INTERVAL) {
        return;
    }
    lastNFCCheckTime = currentMillis;

    // While orb is connected, check if it's still connected
    if (isNFCConnected && isOrbConnected) {
        if (!isNFCActive()) {
            // Orb has disconnected
            endOrbSession();
        }
        return;
    }

    // Check for NFC presence
    if(isNFCPresent()) {
        isNFCConnected = true;
        // NFC is present! Check if it's an orb
        int orbStatus = isOrb();
        switch (orbStatus) {
            case STATUS_FAILED:
                handleError("Failed to check orb header");
                return;
            case STATUS_FALSE:
                if (!isUnformattedNFC) {
                    Serial.println(F("Unformatted NFC connected"));
                    isUnformattedNFC = true;
                    onUnformattedNFC();
                }
                break;
            case STATUS_TRUE:
                isOrbConnected = true;
                setLEDPattern(LED_PATTERN_ORB_CONNECTED);
                readOrbInfo();
                setVisited(true);
                onOrbConnected();
                break;
        }
    }
}

bool OrbDock::isNFCPresent() {
    uint8_t uid[7];  // Buffer to store the returned UID
    uint8_t uidLength;
    if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 30)) {
        return false;
    }
    if (uidLength != 7) {
        Serial.println(F("Detected non-NTAG203 tag (UUID length != 7 bytes)!"));
        return false;
    }
    Serial.println(F("NFC tag read successfully"));
    return true;
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
    setLEDPattern(LED_PATTERN_NO_ORB);
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
    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
        Serial.print(F("Writing to page "));
        Serial.println(page);
        
        if (nfc.ntag2xx_WritePage(page, data)) {
            Serial.println(F("Write succeeded"));
            return STATUS_SUCCEEDED;
        }
        
        retryCount++;
        if (retryCount < MAX_RETRIES) {
            Serial.println(F("Retrying write"));
            //delay(RETRY_DELAY);
            nfc.inListPassiveTarget();
        }
    }

    Serial.println(F("Write failed after retries"));
    return STATUS_FAILED;
}

int OrbDock::readPage(int page) {
    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
        if (nfc.ntag2xx_ReadPage(page, page_buffer)) {
            return STATUS_SUCCEEDED;
        }
        
        retryCount++;
        if (retryCount < MAX_RETRIES) {
            Serial.println(F("Retrying read"));
            delay(RETRY_DELAY);
            nfc.inListPassiveTarget();
        }
    }

    Serial.println(F("Read failed after retries"));
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
    int traitIndex = static_cast<int>(orbInfo.trait);
    if (traitIndex < 0 || static_cast<size_t>(traitIndex) >= sizeof(TRAIT_NAMES)/sizeof(TRAIT_NAMES[0])) {
        Serial.print(F("ERROR: Invalid trait detected: "));
        Serial.println(static_cast<int>(orbInfo.trait));
        setLEDPattern(LED_PATTERN_ERROR);
        return nullptr;
    }
    return TRAIT_NAMES[traitIndex];
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
        setLEDPattern(LED_PATTERN_FLASH);
    }
    return result;
}

int OrbDock::addEnergy(byte amount) {
    byte newEnergy = orbInfo.energy + amount;
    if (newEnergy > 250) newEnergy = 250;
    Serial.print(F("Adding "));
    Serial.print(amount);
    Serial.println(F(" energy"));
    return setEnergy(newEnergy);
}

int OrbDock::removeEnergy(byte amount) {
    byte newEnergy = orbInfo.energy - amount;
    if (newEnergy < 0) newEnergy = 0;
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
    // Write header
    memcpy(page_buffer, ORBS_HEADER, 4);
    if (writePage(ORBS_PAGE, page_buffer) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    // Write default stations
    if (resetOrb() == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    // Write trait
    if (setTrait(trait) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
    // Write energy
    if (setEnergy(INIT_ENERGY) == STATUS_FAILED) {
        return STATUS_FAILED;
    }
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

/********************** LED FUNCTIONS *****************************/

void OrbDock::setLEDPattern(LEDPatternId patternId) {
    ledPatternConfig = LED_PATTERNS[patternId];
}

void OrbDock::runLEDPatterns() {
    static unsigned long ledPreviousMillis;
    static uint8_t ledBrightness;
    //static unsigned long ledBrightnessPreviousMillis;
    static unsigned int ledPatternInterval;

    // If the LED pattern is orb_connected, set the LED speed based on energy level
    if (ledPatternConfig.id == LED_PATTERN_ORB_CONNECTED) {
        ledPatternInterval = map(MAX_ENERGY - orbInfo.energy, 0, MAX_ENERGY, 20, 100);
    }
    else {
        ledPatternInterval = ledPatternConfig.interval;
    }

    if (currentMillis - ledPreviousMillis >= ledPatternInterval) {
        ledPreviousMillis = currentMillis;

        switch (ledPatternConfig.id) {
            case LED_PATTERN_NO_ORB: {
                led_rainbow();
                break;
            }
            case LED_PATTERN_ORB_CONNECTED: {
                led_trait_chase();
                break;
            }
            case LED_PATTERN_FLASH: {
                led_flash();
                break;
            }
            case LED_PATTERN_ERROR: {
                led_error();
                break;
            }
            default:
                break;
        }

        // Set brightness
        if (ledBrightness != ledPatternConfig.brightness) {
            ledBrightness = ledPatternConfig.brightness;
            strip.setBrightness(ledBrightness);
        }
        // Smooth brightness transitions
        // TODO: This causes a bunch of flickering jank. Figure out why.
        // if (ledBrightness != ledPatternConfig.brightness && currentMillis - ledBrightnessPreviousMillis >= ledPatternConfig.brightnessInterval) {
        //     ledBrightnessPreviousMillis = currentMillis;
        //     ledBrightness = lerp(ledBrightness, ledPatternConfig.brightness, ledPatternConfig.brightnessInterval);
        //     strip.setBrightness(ledBrightness);
        // }

        strip.show();
    }

}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void OrbDock::led_rainbow() {
    static long firstPixelHue = 0;
    if (firstPixelHue < 5*65536) {
      strip.rainbow(firstPixelHue, 1, 255, 255, true);
      firstPixelHue += 256;
    } else {
      firstPixelHue = 0; // Reset for next cycle
    }
}

// Rotates a weakening dot around the NeoPixel ring using the trait color
void OrbDock::led_trait_chase() {
    static uint16_t currentPixel = 0;
    static uint8_t intensity = 255;
    static uint8_t globalIntensity = 0;
    static int8_t globalDirection = 1;
    
    // Update global intensity
    globalIntensity += globalDirection * 9;  // Adjust 2 to change global fade speed
    if (globalIntensity >= 255 || globalIntensity <= 30) {
        globalDirection *= -1;
        globalIntensity = constrain(globalIntensity, 30, 255);
    }

    // Find the trait color
    uint32_t traitColor = TRAIT_COLORS[static_cast<int>(orbInfo.trait)];

    // Calculate opposite pixel position
    uint16_t oppositePixel = (currentPixel + (NEOPIXEL_COUNT / 2)) % NEOPIXEL_COUNT;
    
    // Set both bright dots
    uint8_t adjustedIntensity = (uint16_t)intensity * globalIntensity / 255;
    strip.setPixelColor(currentPixel, dimColor(traitColor, adjustedIntensity));
    strip.setPixelColor(oppositePixel, dimColor(traitColor, adjustedIntensity));
    
    // Set pixels between the dots with decreasing intensity
    for (int i = 1; i < NEOPIXEL_COUNT/2; i++) {
        // Calculate pixels on both sides
        uint16_t pixel1 = (currentPixel + i) % NEOPIXEL_COUNT;
        uint16_t pixel2 = (currentPixel - i + NEOPIXEL_COUNT) % NEOPIXEL_COUNT;
        
        // Calculate fade based on distance to nearest bright dot
        float fadeRatio = pow(float(NEOPIXEL_COUNT/4 - abs(i - NEOPIXEL_COUNT/4)) / (NEOPIXEL_COUNT/4), 2);
        uint8_t fadeIntensity = round(intensity * fadeRatio);
        adjustedIntensity = (uint16_t)fadeIntensity * globalIntensity / 255;
        
        if (adjustedIntensity > 0) {
            strip.setPixelColor(pixel1, dimColor(traitColor, adjustedIntensity));
            strip.setPixelColor(pixel2, dimColor(traitColor, adjustedIntensity));
        }
    }
    
    // Move to next pixel
    currentPixel = (currentPixel + 1) % NEOPIXEL_COUNT;
}

void OrbDock::led_flash() {
    static uint8_t intensity = 255;
    static int8_t intensityDirection = -1;
    static uint16_t hueOffset = 0;
    static bool cycleComplete = false;

    // If cycle is complete, switch to appropriate pattern
    if (cycleComplete) {
        intensity = 255;
        intensityDirection = -1;
        hueOffset = 0;
        cycleComplete = false;
        if (isOrbConnected) {
            setLEDPattern(LED_PATTERN_ORB_CONNECTED);
        } else {
            setLEDPattern(LED_PATTERN_NO_ORB);
        }
        return;
    }

    // Get base trait color and extract hue
    uint32_t traitColor = TRAIT_COLORS[static_cast<int>(orbInfo.trait)];
    uint8_t r = (uint8_t)(traitColor >> 16);
    uint8_t g = (uint8_t)(traitColor >> 8); 
    uint8_t b = (uint8_t)traitColor;

    // Fast fade intensity
    intensity += intensityDirection * 12;
    if (intensity <= 30 || intensity >= 255) {
        intensityDirection *= -1;
        intensity = constrain(intensity, 30, 255);
        if (intensity <= 30) {
            cycleComplete = true;
        }
    }

    // Rotate hue offset in opposite direction
    hueOffset = (hueOffset - 8 + 360) % 360;

    // Fill strip with hue-shifted colors
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Calculate hue offset for this pixel
        uint16_t pixelHue = (hueOffset + (360 * i / NEOPIXEL_COUNT)) % 360;
        
        // Create color with similar hue to trait color but varying
        float hueShift = sin(pixelHue * PI / 180.0) * 30; // +/- 30 degree hue shift
        uint32_t shiftedColor = strip.Color(
            r + (r * hueShift/360),
            g + (g * hueShift/360),
            b + (b * hueShift/360)
        );

        strip.setPixelColor(i, dimColor(shiftedColor, intensity));
    }
}

void OrbDock::led_error() {
    static uint8_t r = 0;
    static uint8_t b = 255;
    static bool toRed = true;

    if (toRed) {
        r = min(255, r + 1);
        b = max(0, b - 1);
        if (r >= 255 && b <= 0) {
            toRed = false;
        }
    } else {
        r = max(0, r - 1); 
        b = min(255, b + 1);
        if (r <= 0 && b >= 255) {
            toRed = true;
        }
    }

    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, r, 0, b);
    }
}

// Helper function to dim a 32-bit color value by a certain intensity (0-255)
uint32_t OrbDock::dimColor(uint32_t color, uint8_t intensity) {
  uint8_t r = (uint8_t)(color >> 16);
  uint8_t g = (uint8_t)(color >> 8);
  uint8_t b = (uint8_t)color;
  
  r = (r * intensity) >> 8;
  g = (g * intensity) >> 8;
  b = (b * intensity) >> 8;
  
  return strip.Color(r, g, b);
}

float OrbDock::lerp(float start, float end, float t) {
    return start + t * (end - start);
}

// Provide empty implementations for pure virtual functions to satisfy the linker
void OrbDock::onOrbConnected() {}
void OrbDock::onOrbDisconnected() {}
void OrbDock::onError(const char* errorMessage) {}
void OrbDock::onUnformattedNFC() {}
/********************** MISC FUNCTIONS *****************************/
