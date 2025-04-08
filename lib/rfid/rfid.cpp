#include "rfid.h"
#include "eeprom_manager.h"

Adafruit_PN532 nfcI2C(SDA_PIN, SCL_PIN);

void rfidInit() {
  nfcI2C.begin();

  uint32_t versiondata = nfcI2C.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
}

void rfidRead() {
    static bool cardDetected = false;
    static unsigned long lastCardTime = 0;
    
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    unsigned long currentTime = millis();
    
    if (success) {
        if (!cardDetected || (currentTime - lastCardTime > 3000)) {
            Serial.println("Found an ISO14443A card");
            Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
            Serial.print("  UID Value: ");
            nfcI2C.PrintHex(uid, uidLength);

            if (uidLength == 4){
                uint32_t cardid = uid[0];
                cardid <<= 8;
                cardid |= uid[1];
                cardid <<= 8;
                cardid |= uid[2];
                cardid <<= 8;
                cardid |= uid[3];
                Serial.print("Seems to be a Mifare Classic card #");
                Serial.println(cardid);

                // Kiểm tra quyền truy cập
                if (EEPROMManager::isRFIDCardExists(uid, uidLength)) {
                    Serial.println("Access granted - Card is registered");
                    // TODO: Thêm code xử lý khi thẻ hợp lệ
                } else {
                    Serial.println("Access denied - Card is not registered");
                    // TODO: Thêm code xử lý khi thẻ không hợp lệ
                }
            }
            Serial.println("");
            
            cardDetected = true;
            lastCardTime = currentTime;
        }
    } else {
        if (currentTime - lastCardTime > 1000) {
            cardDetected = false;
        }
    }
}

void handleAddNewCard(uint8_t* uid, uint8_t uidLength) {
    
}

bool addNewCard(uint8_t* uid, uint8_t uidLength) {
    return EEPROMManager::addRFIDCard(uid, uidLength, true);
}

bool removeCard(uint8_t* uid, uint8_t uidLength) {
    return EEPROMManager::deleteRFIDCard(uid, uidLength, true);
}

void clearAllCards() {
    EEPROMManager::clearAllRFIDCards(true);
}

