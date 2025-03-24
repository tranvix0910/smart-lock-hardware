#ifndef RFID_H
#define RFID_H

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21    // SDA
#define RST_PIN 27  // RST

void rfidInit();
void rfidRead();

#endif
