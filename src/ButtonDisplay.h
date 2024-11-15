#ifndef BUTTON_DISPLAY_H
#define BUTTON_DISPLAY_H

#include <Wire.h>
#include <U8glib.h>
#include <Arduino.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define BTN1_PIN 7
#define BTN2_PIN 8
#define BTN3_PIN 9
#define BTN4_PIN 10


class ButtonDisplay {
private:
    U8GLIB_SSD1306_128X64 display;
    bool buttonsInitialized;
    bool displayInitialized;
    uint8_t cursorX;
    uint8_t cursorY;
    uint8_t charHeight;
    bool needsUpdate;
    char lastText[16];
    const uint8_t* defaultFont;
    static const int MAX_LINES = 8;  // Maximum number of lines to store
    char textLines[MAX_LINES][16];
    uint8_t numLines;

    void initButtons();
    void initDisplay();

public:
    ButtonDisplay(const uint8_t* font);
    void begin();
    void clearDisplay();
    void updateDisplay();
    void setCursor(uint8_t x, uint8_t y);
    void print(const char* text);
    void print(int number);
    void print(byte number);
    void println(const char* text = nullptr);
    void setFont(const uint8_t* font);
    bool isButton1Pressed();
    bool isButton2Pressed();
    bool isButton3Pressed();
    bool isButton4Pressed();
    void showMessage(const char* message, uint16_t duration = 2000);
    void showError(const char* errorMessage);
    U8GLIB_SSD1306_128X64* getDisplay();
};

#endif