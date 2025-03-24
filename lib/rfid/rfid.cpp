#include "rfid.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);   

void rfidInit() {
    SPI.begin(14, 12, 13, SS_PIN); // SCK, MISO, MOSI, SS
    mfrc522.PCD_Init();
    Serial.println("RFID reader initialized.");
}

void rfidRead() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    Serial.print("RFID card detected: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    mfrc522.PICC_HaltA();   
}