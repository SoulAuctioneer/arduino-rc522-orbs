#ifndef ORB_STATION_H
#define ORB_STATION_H

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

// NeoPixel pin 
#define NEOPIXEL_PIN (6)

// PN532 pins
#define PN532_SCK  (5)
#define PN532_MISO (4)
#define PN532_MOSI (3)
#define PN532_SS   (2)

// Old PN532 pins - for later dock designs
// #define PN532_SCK  (2)
// #define PN532_MISO (5)
// #define PN532_MOSI (3)
// #define PN532_SS   (4)

// Old PN532 pins - for early dock designs
// #define PN532_SCK  (2)
// #define PN532_MISO (5)
// #define PN532_MOSI (3)
// #define PN532_SS   (4)

// Status constants
#define STATUS_FAILED    0
#define STATUS_SUCCEEDED 1
#define STATUS_FALSE     2
#define STATUS_TRUE      3

// Communication constants
#define MAX_RETRIES      4
#define RETRY_DELAY      10
#define NFC_TIMEOUT      1000
#define DELAY_AFTER_CARD_PRESENT 50
#define NFC_CHECK_INTERVAL 300

// NFC constants
#define PAGE_OFFSET 4
#define ORBS_PAGE (PAGE_OFFSET + 0)
#define TRAIT_PAGE (PAGE_OFFSET + 1) 
#define STATIONS_PAGE_OFFSET (PAGE_OFFSET + 2)
#define ORBS_HEADER "ORBS"

// LED constants
#define NEOPIXEL_COUNT  24

// Station constants
#define NUM_STATIONS 14
#define NUM_TRAITS 6

enum TraitId {
    NONE, RUMINATE, SHAME, DOUBT, DISCONTENT, HOPELESS
};

const char* const TRAIT_NAMES[] = {
    "NONE",
    "RUMINATE",
    "SHAME",
    "DOUBT",
    "DISCONTENT",
    "HOPELESS"
};

const uint32_t TRAIT_COLORS[] = {
    0xFF0000,  // None
    0xFF2800,  // Orange for RUMINATE (Rumination)
    0xFF4600,  // Yellow for SHAME (Shame Spiral)
    0x20FF00,  // Green for DOUBT (Self Doubt)
    0xFF00D2,  // Pink/Magenta for DISCONTENT (Discontentment)
    0x1400FF   // Blue for HOPELESS (Hopelessness)
};

const char* const TRAIT_COLOR_NAMES[] = {
    "red",     // None
    "orange",  // Rumination
    "yellow",  // Shame Spiral
    "green",   // Self Doubt
    "pink",    // Discontentment
    "blue"     // Hopelessness
};

enum StationId {
    GENERIC, CONFIGURE, CONSOLE, DISTILLER, CASINO, FOREST,
    ALCHEMY, PIPES, CHECKER, SLERP, RETOXIFY,
    GENERATOR, STRING, CHILL, HUNT
};

const char* const STATION_NAMES[] = {
    "GENERIC", "CONFIGURE", "CONSOLE", "DISTILLER", "CASINO", "FOREST",
    "ALCHEMY", "PIPES", "CHECKER", "SLERP", "RETOXIFY",
    "GENERATOR", "STRING", "CHILL", "HUNT"
};

// Station struct
struct Station {
    bool visited;
    uint16_t energy;
    byte custom;
};

enum LEDPatternId {
    LED_PATTERN_NO_ORB,
    LED_PATTERN_ORB_CONNECTED,
    LED_PATTERN_FLASH
};

struct LEDPatternConfig {
    int id;
    uint8_t brightness;
    uint16_t interval;
    float brightnessInterval;
};

const LEDPatternConfig LED_PATTERNS[] = {
    {
        .id = LED_PATTERN_NO_ORB,
        .brightness = 200,
        .interval = 15,
        .brightnessInterval = 5.0f
    },
    {
        .id = LED_PATTERN_ORB_CONNECTED,
        .brightness = 255,
        .interval = 80,
        .brightnessInterval = 5.0f
    },
    {
        .id = LED_PATTERN_FLASH,
        .brightness = 255,
        .interval = 10,
        .brightnessInterval = 5.0f
    }
};

// Additional helper structs/enums
struct OrbInfo {
    TraitId trait;
    Station stations[NUM_STATIONS];
};

class OrbDock {
public:
    OrbDock(StationId id);
    virtual ~OrbDock();
    
    void begin();
    virtual void loop();

protected:
    // State variables
    StationId stationId;
    OrbInfo orbInfo;
    bool isNFCConnected;
    bool isOrbConnected;
    bool isUnformattedNFC;
    
    // Timing variables
    unsigned long currentMillis;

    // Virtual methods for child classes to implement
    virtual void onOrbConnected() = 0;
    virtual void onOrbDisconnected() = 0;
    virtual void onError(const char* errorMessage) = 0;
    virtual void onUnformattedNFC() = 0;

    // Helper methods that child classes can use
    Station getCurrentStationInfo();
    // Returns the total energy of the orb across all stations
    uint16_t getTotalEnergy();
    // Returns the trait name
    const char* getTraitName();
    // Resets the station information, but keeps the trait
    int resetOrb();
    // Resets the orb with a new trait
    int formatNFC(TraitId newTrait);
    // Writes the trait to the orb
    int setTrait(TraitId newTrait);
    // Adds energy to the current station
    int addEnergy(uint16_t amount);
    // Removes energy from the current station
    int removeEnergy(uint16_t amount);
    // Sets the energy of the current station
    int setEnergy(uint16_t amount);
    // Gets the energy of the current station
    uint16_t getEnergy();
    // Sets the visited status of the current station
    int setVisited(bool visited);
    // Sets the custom value of the current station
    int setCustom(byte value);
    // Sets the LED pattern
    void setLEDPattern(LEDPatternId patternId);
    // Reads and prints the entire NFC storage
    void printNFCStorage();

private:
    // NFC helper methods
    int writeStation(int stationID);
    int writeStations();
    int writePage(int page, uint8_t* data);
    int readPage(int page);
    int readOrbInfo();
    int writeOrbInfo();
    void reInitializeStations();
    bool isNFCPresent();
    bool isNFCActive();
    int isOrb();
    void printOrbInfo();
    void endOrbSession();

    // LED pattern methods
    void runLEDPatterns();
    void led_rainbow();
    void led_trait_chase();
    void led_flash();
    uint32_t dimColor(uint32_t color, uint8_t intensity);
    float lerp(float start, float end, float t);

    // Additional helper methods
    void handleError(const char* message);
    
    // Hardware objects
    Adafruit_NeoPixel strip;
    Adafruit_PN532 nfc;
    
    // LED variables
    LEDPatternConfig ledPatternConfig;
    
    // NFC
    byte page_buffer[4];
};

#endif
