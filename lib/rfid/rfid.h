#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

void rfidInit();
void rfidRead();
void handleAddNewCard(uint8_t* uid, uint8_t uidLength);
bool addNewCard(uint8_t* uid, uint8_t uidLength);
bool removeCard(uint8_t* uid, uint8_t uidLength);
void clearAllCards();

#endif
