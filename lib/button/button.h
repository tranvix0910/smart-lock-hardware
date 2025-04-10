#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include "user_interface.h"
#include "fingerprint.h"
#include "lock.h"
#include "mqtt.h"
#include "api.h"
#include "recentAccessLogs.h"

#define LONG_PRESS_TIME 3000
#define MIN_FINGER_ID 1
#define MAX_FINGER_ID 99

#define BUTTON_CAPTURE_PIN 26
#define BUTTON_RESET_PIN 13

// External declarations for fingerprint enrollment
extern bool pendingFingerprintEnroll;
extern String pendingFaceId;
extern bool isNormalMode;
extern uint8_t fingerprintMode;

typedef void (*HandleImageCallback)();
typedef void (*DisplayResultCallback)(String message, uint16_t color);
typedef void (*DisplayCornerTextCallback)(String message, uint16_t color, uint8_t fontSize);

void buttonInit();
bool buttonCaptureImageRead();
void buttonResetMode();
void buttonEvent(
    HandleImageCallback handleImageCallback, 
    DisplayResultCallback displayResultCallback,
    DisplayCornerTextCallback displayCornerText
);
void enrollFingerprint(DisplayResultCallback displayResultCallback);
void processDeleteFingerprint(uint8_t id, DisplayResultCallback displayResultCallback);
#endif
