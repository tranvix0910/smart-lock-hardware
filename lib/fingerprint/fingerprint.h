#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <TFT_eSPI.h>
#include "lock.h"
#include "user_interface.h"

#define RX_PIN 16 // GREEN
#define TX_PIN 17 // WHITE
#define BAUD_RATE 57600

extern HardwareSerial mySerial;
extern Adafruit_Fingerprint finger;

typedef void (*DisplayResultCallback)(String message, uint16_t color);

void fingerprintInit();
uint8_t getFingerprintEnroll(int id, DisplayResultCallback displayResultCallback);
int getFingerprintId();
bool unlockWithFingerprint(DisplayResultCallback displayResultCallback);
bool deleteAllFingerprints(DisplayResultCallback displayResultCallback);
bool deleteFingerprint(uint8_t id, DisplayResultCallback displayResultCallback);
bool isFingerIDFree(uint8_t id);
int getNextFreeID();

#endif