#include "NFCReader.h"

PN532Reader::PN532Reader() : 
    nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS) {
}

bool PN532Reader::begin() {
    // Try each pin configuration until one works
    Serial.println(F("Initializing PN532 NFC reader with latest dock pins..."));
    if (tryPinConfiguration(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS)) {
        return true;
    }

    Serial.println(F("Latest dock pins failed, trying V2 dock pins..."));
    if (tryPinConfiguration(PN532_SCK2, PN532_MISO2, PN532_MOSI2, PN532_SS2)) {
        return true;
    }

    Serial.println(F("V2 dock pins failed, trying v1 dock pins..."));
    if (tryPinConfiguration(PN532_SCK1, PN532_MISO1, PN532_MOSI1, PN532_SS1)) {
        return true;
    }

    Serial.println(F("Didn't find PN53x board with any pin configuration"));
    return false;
}

bool PN532Reader::tryPinConfiguration(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss) {
    nfc = Adafruit_PN532(sck, miso, mosi, ss);
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    
    if (versiondata) {
        nfc.SAMConfig();
        nfc.setPassiveActivationRetries(0x11);
        return true;
    }
    return false;
}

bool PN532Reader::isTagPresent() {
    uint8_t uid[7];
    uint8_t uidLength;
    if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 30)) {
        return false;
    }
    return uidLength == 7;
}

bool PN532Reader::readPage(int page, uint8_t* buffer) {
    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
        if (nfc.ntag2xx_ReadPage(page, buffer)) {
            return true;
        }
        
        retryCount++;
        if (retryCount < MAX_RETRIES) {
            Serial.println(F("Retrying read"));
            delay(RETRY_DELAY);
            nfc.inListPassiveTarget();
        }
    }
    return false;
}

bool PN532Reader::writePage(int page, uint8_t* data) {
    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
        if (nfc.ntag2xx_WritePage(page, data)) {
            return true;
        }
        
        retryCount++;
        if (retryCount < MAX_RETRIES) {
            Serial.println(F("Retrying write"));
            nfc.inListPassiveTarget();
        }
    }
    return false;
} 