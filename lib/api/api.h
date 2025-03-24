#ifndef API_H
#define API_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <ArduinoWebsockets.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>

#include "lock.h"
#include "button.h"
#include "user_interface.h"
#include "motion_detect.h"
#include "fingerprint.h"
#include "magnetic_hall.h"
using namespace websockets;

extern String livenessCheckResult;

void parsingJSONResult(
    String response, 
    uint16_t SCREEN_COLOR, 
    String message, 
    String livenessCheckResult,
    uint8_t failedAttempts
);
void uploadImageToS3(WebsocketsMessage msg);
bool livenessCheck(WebsocketsMessage msg);
void compareFace(WebsocketsMessage msg);
bool authenticateFace(WebsocketsMessage msg);

#endif

