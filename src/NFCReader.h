#ifndef NFC_READER_H
#define NFC_READER_H

#include <Adafruit_PN532.h>

// PN532 pins - latest design
#define PN532_SCK   (5)
#define PN532_MISO  (4)
#define PN532_MOSI  (3)
#define PN532_SS    (2)

// V2 PN532 pins - for later dock designs
#define PN532_SCK2  (2)
#define PN532_MISO2 (3)
#define PN532_MOSI2 (4)
#define PN532_SS2   (5)

// V1 PN532 pins - for early dock designs
#define PN532_SCK1  (2)
#define PN532_MISO1 (5)
#define PN532_MOSI1 (3)
#define PN532_SS1   (4)

// Communication constants
#define MAX_RETRIES      4
#define RETRY_DELAY      10
#define NFC_TIMEOUT      1000

// Abstract base class for NFC readers
class NFCReaderBase {
public:
    virtual ~NFCReaderBase() = default;
    virtual bool begin() = 0;
    virtual bool isTagPresent() = 0;
    virtual bool readPage(int page, uint8_t* buffer) = 0;
    virtual bool writePage(int page, uint8_t* data) = 0;
};

// PN532 implementation
class PN532Reader : public NFCReaderBase {
public:
    PN532Reader();
    bool begin() override;
    bool isTagPresent() override;
    bool readPage(int page, uint8_t* buffer) override;
    bool writePage(int page, uint8_t* data) override;

private:
    Adafruit_PN532 nfc;
    bool tryPinConfiguration(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss);
};

#endif
