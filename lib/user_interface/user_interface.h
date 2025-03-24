#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <Adafruit_Fingerprint.h>

#include "wifi.h"
#include "api.h"
#include "button.h"
#include "motion_detect.h"
#include "fingerprint.h"

using namespace websockets;

extern WebsocketsServer server;
extern WebsocketsClient client;

#define GFXFF 1
#define FSB9 &FreeSerifBold9pt7b

#define SCREEN_TIMEOUT 30000  
#define BACKLIGHT_PIN 12      
#define BACKLIGHT_ON 255      
#define BACKLIGHT_OFF 10      

extern TFT_eSPI tft;

void websocketInit();
void websocketHandle();

void displayInit();
bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

void showingImage();
void handleImage();

bool faceAuthentication();

void displayJSONParsingFailed();
void displayJSONParsingResult(
    uint16_t color, 
    String message, 
    String livenessCheckResult, 
    String user_name, 
    String timestamp, 
    double similarity,
    uint8_t failedAttempts
);

void displaySetBrightness(uint8_t brightness);
void displayTurnOn();
void displayTurnOff();
void displayCheckMotion();

void displayClear();

void displayResult(String message, uint16_t color);

#endif
