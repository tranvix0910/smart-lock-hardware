#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include "user_interface.h"
#include "fingerprint.h"
#include "lock.h"

extern bool isNormalMode;
extern bool lastButtonState;

#define LONG_PRESS_TIME 3000
#define MIN_FINGER_ID 1
#define MAX_FINGER_ID 99

#define BUTTON_CAPTURE_PIN 26
#define FINGERPRINT_SCAN_MODE 0
#define FINGERPRINT_ENROLL_MODE 1

typedef void (*HandleImageCallback)();
typedef void (*DisplayResultCallback)(String message, uint16_t color);

void buttonInit();
bool buttonCaptureImageRead();
void checkFingerprintMode(DisplayResultCallback displayResultCallback);
void buttonEvent(
    HandleImageCallback handleImageCallback, 
    DisplayResultCallback displayResultCallback
);

#endif
