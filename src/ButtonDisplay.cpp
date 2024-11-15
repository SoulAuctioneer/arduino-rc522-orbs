/**
 * Controller for the button display module 
 * 
 * I2C pins:
 * SDA: A4 (Pin 27)
 * SCL: A5 (Pin 28)
 * 
 * Button Pins:
 * S1: D8
 * S2: D9
 * S3: D10  
 * S4: D11
 */

#include "ButtonDisplay.h"
#include <Wire.h>
#include <U8glib.h>
#include <Arduino.h>

#define DISPLAY_WIDTH 128  // Display width in pixels
#define DISPLAY_HEIGHT 64  // Display height in pixels
#define SCREEN_ADDRESS 0x3C

ButtonDisplay::ButtonDisplay(const uint8_t* font) : display(U8G_I2C_OPT_NONE), defaultFont(font) {
    buttonsInitialized = false;
    displayInitialized = false;
    cursorX = 0;
    cursorY = 0;
    charHeight = 8;
    needsUpdate = false;
    lastText[0] = '\0';
    defaultFont = font;
    textLines[0][0] = '\0';
    numLines = 0;
}

void ButtonDisplay::initButtons() {
    if (!buttonsInitialized) {
        pinMode(BTN1_PIN, INPUT_PULLUP);
        pinMode(BTN2_PIN, INPUT_PULLUP);
        pinMode(BTN3_PIN, INPUT_PULLUP);
        pinMode(BTN4_PIN, INPUT_PULLUP);
        buttonsInitialized = true;
    }
}

void ButtonDisplay::initDisplay() {
    if (!displayInitialized) {
        Serial.println(F("Initializing display"));
        Wire.begin();
        
        Wire.beginTransmission(SCREEN_ADDRESS);
        byte error = Wire.endTransmission();
        if (error != 0) {
            Serial.print(F("I2C device not found at address 0x"));
            Serial.println(SCREEN_ADDRESS, HEX);
            return;
        }

        display.begin();
        display.setFont(defaultFont);  // Use constructor-provided font
        display.setFontRefHeightExtendedText();
        display.setDefaultForegroundColor();
        display.setFontPosTop();
        
        // Calculate initial character height
        charHeight = display.getFontAscent() - display.getFontDescent();
        cursorX = 0;
        cursorY = 0;
        displayInitialized = true;
        numLines = 0;
        lastText[0] = '\0';
    }
}

void ButtonDisplay::begin() {
    initButtons();
    initDisplay();
}

void ButtonDisplay::clearDisplay() {
    cursorX = 0;
    cursorY = 0;
    numLines = 0;  // Reset line counter
    display.firstPage();
    do {
        // Empty draw - clears display
    } while(display.nextPage());
}

void ButtonDisplay::updateDisplay() {
    if (needsUpdate) {
        display.firstPage();
        do {
            // Calculate total height of all lines
            uint8_t totalHeight = numLines * charHeight;
            
            // Calculate starting Y position to center vertically
            uint8_t startY = (DISPLAY_HEIGHT - totalHeight) / 2;
            uint8_t y = startY;
            
            // Draw all stored lines
            for (uint8_t i = 0; i < numLines; i++) {
                // Calculate width of text and center position horizontally
                uint8_t strWidth = display.getStrWidth(textLines[i]);
                uint8_t x = (DISPLAY_WIDTH - strWidth) / 2;
                display.drawStr(x, y, textLines[i]);
                y += charHeight;
            }
        } while(display.nextPage());
        needsUpdate = false;
    }
}

void ButtonDisplay::setCursor(uint8_t x, uint8_t y) {
    if (x < DISPLAY_WIDTH) cursorX = x;  // Added bounds checking
    if (y < DISPLAY_HEIGHT) cursorY = y;     // Added bounds checking
}

void ButtonDisplay::print(const char* text) {
    // Store position and text for next update
    strncpy(lastText, text, sizeof(lastText)-1);
    lastText[sizeof(lastText)-1] = '\0';
    display.drawStr(cursorX, cursorY, text);
    cursorX += display.getStrWidth(text);
    needsUpdate = true;
}

void ButtonDisplay::print(int number) {
    char buf[12];
    itoa(number, buf, 10);
    print(buf);
}

void ButtonDisplay::print(byte number) {
    print((int)number);
}

void ButtonDisplay::println(const char* text) {
    if (text != nullptr && numLines < MAX_LINES) {
        strncpy(textLines[numLines], text, sizeof(textLines[0])-1);
        textLines[numLines][sizeof(textLines[0])-1] = '\0';
        numLines++;
    }
    cursorX = 0;
    cursorY += charHeight;
    if (cursorY >= DISPLAY_HEIGHT) {
        cursorY = 0;
    }
    needsUpdate = true;
}

void ButtonDisplay::setFont(const uint8_t* font) {
    display.setFont(font);
}

bool ButtonDisplay::isButton1Pressed() {
    return !digitalRead(BTN1_PIN);
}

bool ButtonDisplay::isButton2Pressed() {
    return !digitalRead(BTN2_PIN);
}

bool ButtonDisplay::isButton3Pressed() {
    return !digitalRead(BTN3_PIN);
}

bool ButtonDisplay::isButton4Pressed() {
    return !digitalRead(BTN4_PIN);
}

void ButtonDisplay::showMessage(const char* message, uint16_t duration) {
    clearDisplay();
    setCursor(0, 0);
    print(message);
    updateDisplay();  // Add explicit update
    if (duration > 0) {
        delay(duration);
    }
}

void ButtonDisplay::showError(const char* errorMessage) {
    clearDisplay();
    setCursor(0, 0);
    println(errorMessage);
}

U8GLIB_SSD1306_128X64* ButtonDisplay::getDisplay() {
    return &display;
}
