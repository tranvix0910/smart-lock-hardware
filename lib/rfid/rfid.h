#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include "common.h"
#include "lock.h"
#include "eeprom_manager.h"
#include "smart_lock_system.h"

#define SDA_PIN 21
#define SCL_PIN 22

extern String failedRFIDEnroll;

typedef void (*DisplayResultCallback)(String message, uint16_t color);

void rfidInit();
void rfidRead();

// Các hàm quản lý thẻ RFID
bool addNewCard(uint8_t* uid, uint8_t uidLength);
bool removeCard(uint8_t* uid, uint8_t uidLength);
void clearAllCards();

bool handleAddNewCard(
    DisplayResultCallback displayResultCallback,
    uint8_t* uid,
    uint8_t* uidLength
);

// Các hàm xử lý UID cho MQTT
String rfidUIDToString(uint8_t* uid, uint8_t uidLength);
String createRFIDCardJSON(uint8_t* uid, uint8_t uidLength);

// Unlocking with RFID
bool unlockWithRFID(DisplayResultCallback displayResultCallback);

void checkRFIDMode(DisplayResultCallback displayResultCallback);

#endif
