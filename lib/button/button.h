#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <freertos/FreeRTOS.h>

#include "user_interface.h"
#include "fingerprint.h"
#include "lock.h"
#include "mqtt.h"
#include "api.h"
#include "recentAccessLogs.h"
#include "rfid.h"

#define LONG_PRESS_TIME 3000
#define MIN_FINGER_ID 1
#define MAX_FINGER_ID 99

#define BUTTON_CAPTURE_PIN 26
#define BUTTON_RESET_PIN 13

extern bool pendingFingerprintEnroll;
extern String pendingFingerprintEnrollFaceId;
extern uint8_t fingerprintMode;

extern String deviceId;
extern String macAddress;
extern String userId;
extern String faceId;

extern String topicAddFingerprintPublish;
extern String topicDeleteFingerprintPublish;

extern String topicAddRFIDCardPublish;
extern String topicRemoveRFIDCardPublish;

extern String topicUnlockSystemPublish;

extern bool pendingFingerprintEnroll;
extern String pendingFingerprintEnrollFaceId;

extern bool pendingRFIDEnroll;
extern String pendingRFIDEnrollFaceId;
extern String failedRFIDEnroll;

extern bool pendingDeleteFingerprint;
extern String pendingDeleteFingerprintFaceId;
extern int pendingDeleteFingerprintId;

extern bool pendingRemoveRFIDCard;
extern String pendingRemoveRFIDCardFaceId;
extern String pendingRemoveRFIDCardUID;
extern String pendingRemoveRFIDCardUIDLength;

extern bool pendingUnlockSystem;
extern String pendingUnlockSystemFaceId;

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
void enrollRFID(DisplayResultCallback displayResultCallback);
void processRemoveRFIDCard(DisplayResultCallback displayResultCallback, uint8_t* targetUID, uint8_t targetUIDLength);
void processUnlockSystem(DisplayResultCallback displayResultCallback);

#endif
