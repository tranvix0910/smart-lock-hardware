#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <Adafruit_PN532.h>
#include "common.h"
#include "lock.h"
#include "eeprom_manager.h"
#include "smart_lock_system.h"
#include "freertos/FreeRTOS.h"

#define SDA_PIN 21
#define SCL_PIN 22

extern String failedRFIDEnroll;
extern bool isAddingCard;

extern void publishRecentAccessLogs(String type, String status, String uid, String message);

typedef void (*DisplayResultCallback)(String message, uint16_t color);

void rfidInit();
// void rfidRead();

bool addNewCard(uint8_t* uid, uint8_t uidLength);
bool removeCard(uint8_t* uid, uint8_t uidLength);
void clearAllCards();

bool handleAddNewCard(
    DisplayResultCallback displayResultCallback,
    uint8_t* uid,
    uint8_t* uidLength
);

String rfidUIDToString(uint8_t* uid, uint8_t uidLength);
String createRFIDCardJSON(uint8_t* uid, uint8_t uidLength);

bool unlockWithRFID(DisplayResultCallback displayResultCallback);
void checkRFIDMode(DisplayResultCallback displayResultCallback);

#endif
