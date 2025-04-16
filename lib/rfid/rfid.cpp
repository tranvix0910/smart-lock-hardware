#include "rfid.h"
#include "alert.h"

extern TaskHandle_t buttonTask;
extern TaskHandle_t rfidModeTask;
String failedRFIDEnroll = "";

Adafruit_PN532 nfcI2C(SDA_PIN, SCL_PIN);

void rfidInit() {
    Serial.println("Initializing RFID module...");
    
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);

    Wire.end();
    delay(100);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(50000);
    Wire.setTimeout(2000);
    
    nfcI2C.begin();
    
    uint32_t versiondata = nfcI2C.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("ERROR: Couldn't find PN53x board. Check connections!");
        return;
    }

    Serial.print("Found chip PN5"); 
    Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); 
    Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); 
    Serial.println((versiondata>>8) & 0xFF, DEC);

    nfcI2C.SAMConfig();
    Serial.println("RFID Reader Configured");
    
    int cardCount = 0;
    for (uint16_t i = 0; i < MAX_RFID_CARDS; i++) {
        uint16_t addr = RFID_START_ADDR + (i * RFID_CARD_SIZE);
        if (EEPROM.read(addr) != RFID_EMPTY_SLOT) {
            cardCount++;
        }
    }
    
    Serial.print("Found "); 
    Serial.print(cardCount); 
    Serial.println(" registered RFID cards in memory");
    Serial.println("RFID module initialization complete");
}

// void rfidRead() {
//     if (xSemaphoreTake(i2cMutex, 100 / portTICK_PERIOD_MS) != pdTRUE) {
//         return;
//     }
//     static bool cardDetected = false;
//     static unsigned long lastCardTime = 0;
//     uint8_t success;
//     uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
//     uint8_t uidLength;
//     success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
//     unsigned long currentTime = millis();
//     if (success) {
//         if (!cardDetected || (currentTime - lastCardTime > 3000)) {
//             Serial.println("Found an ISO14443A card");
//             Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
//             Serial.print("  UID Value: ");
//             nfcI2C.PrintHex(uid, uidLength);
//             alertBeep(100);
//             if (uidLength == 4){
//                 uint32_t cardid = uid[0];
//                 cardid <<= 8;
//                 cardid |= uid[1];
//                 cardid <<= 8;
//                 cardid |= uid[2];
//                 cardid <<= 8;
//                 cardid |= uid[3];
//                 Serial.print("Seems to be a Mifare Classic card #");
//                 Serial.println(cardid);
//                 if (EEPROMManager::isRFIDCardExists(uid, uidLength)) {
//                     Serial.println("Access granted - Card is registered");
//                     alertBeep(200);
//                 } else {
//                     Serial.println("Access denied - Card is not registered");
//                 }
//             }
//             Serial.println("");
//             cardDetected = true;
//             lastCardTime = currentTime;
//         }
//     } else {
//         if (currentTime - lastCardTime > 1000) {
//             cardDetected = false;
//         }
//     }
//     xSemaphoreGive(i2cMutex);
// }

bool handleAddNewCard(DisplayResultCallback displayResultCallback, uint8_t* uid, uint8_t* uidLength) {
    uint8_t success;
    uint8_t currentUID[7];
    uint8_t currentLength;

    displayResultCallback("Place RFID card", TFT_BLUE);
    Serial.println("Place RFID card to register");
    
    unsigned long startTime = millis();
    bool cardDetected = false;
    
    while (!cardDetected && millis() - startTime < 5000) {

        success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, currentUID, &currentLength);
        
        if (success) {
            Serial.println("Card detected");
            Serial.print("UID Length: "); Serial.print(currentLength, DEC); Serial.println(" bytes");
            Serial.print("UID Value: ");
            nfcI2C.PrintHex(currentUID, currentLength);
            
            alertBeep(100);
            
            if (EEPROMManager::isRFIDCardExists(currentUID, currentLength)) {
                failedRFIDEnroll = "ADD RFID CARD FAILED: CARD ALREADY EXISTS";
                displayResultCallback("Card already exists!", TFT_RED);
                Serial.println("Card already exists in the system");
                isAddingCard = false;
                return false;
            }

            memcpy(uid, currentUID, currentLength);
            *uidLength = currentLength;
            
            displayResultCallback("Card scanned", TFT_CYAN);
            cardDetected = true;
        }
        delay(100);
    }
    
    if (!cardDetected) {
        displayResultCallback("Timeout - No card", TFT_RED);
        Serial.println("Timeout - No card detected");
        isAddingCard = false;
        return false;
    }
    
    displayResultCallback("Saving card...", TFT_CYAN);
    
    if (addNewCard(currentUID, currentLength)) {
        displayResultCallback("Card registered!", TFT_GREEN);
        Serial.println("Card registered successfully!");
        alertBeep(200);
        isNormalMode = true;
        isAddingCard = false;
        return true;
    } else {
        displayResultCallback("Registration failed", TFT_RED);
        Serial.println("Failed to register card");
        isNormalMode = true;
        isAddingCard = false;
        return false;
    }
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

String rfidUIDToString(uint8_t* uid, uint8_t uidLength) {
    String result = "";
    char buf[3];
    
    for (uint8_t i = 0; i < uidLength; i++) {
        snprintf(buf, sizeof(buf), "%02X", uid[i]);
        result += buf;
        if (i < uidLength - 1) {
            result += ":";
        }
    }
    
    return result;
}

String createRFIDCardJSON(uint8_t* uid, uint8_t uidLength) {
    DynamicJsonDocument doc(256);
    
    doc["cardUID"] = rfidUIDToString(uid, uidLength);
    doc["uidLength"] = uidLength;
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

bool unlockWithRFID(DisplayResultCallback displayResultCallback) {

    Serial.println("Waiting for RFID card...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 5000;
    uint8_t success = 0;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    while (!success && (millis() - startTime < timeout)) {
        
        success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        
        if (success) {
            displayResultCallback("Card detected!", TFT_BLUE);
            Serial.println("RFID card detected!");
            alertBeep(100);
        }
        
        vTaskDelay(5);
        
        if (!success) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    
    if (!success) {
        displayResultCallback("Timeout", TFT_RED);
        Serial.println("Timeout - No RFID card detected");
        return false;
    }
    
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    nfcI2C.PrintHex(uid, uidLength);
    
    displayResultCallback("Processing...", TFT_CYAN);
    
    if (EEPROMManager::isRFIDCardExists(uid, uidLength)) {
        char message[50];
        snprintf(message, sizeof(message), "Welcome Card #%s!", rfidUIDToString(uid, uidLength).c_str());
        displayResultCallback(message, TFT_GREEN);
        Serial.print("RFID card matched! UID: ");
        Serial.println(rfidUIDToString(uid, uidLength));
        alertBeep(200);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        lockOpen();
        resetFailedAttempts();
        return true;
    } else {
        displayResultCallback("Access Denied!", TFT_RED);
        Serial.println("RFID card not verified!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        isNormalMode = true;
        incrementFailedAttempt();
        return false;
    }
}

void checkRFIDMode(DisplayResultCallback displayResultCallback) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        if (isNormalMode) {
            isNormalMode = false;
            displayResultCallback("RFID detected...", TFT_CYAN);
        }
        
        if (!isNormalMode) {
            Serial.println("Unlocking with RFID...");
            bool unlockSuccess = unlockWithRFID(displayResultCallback);
            if (!unlockSuccess) {
                isNormalMode = true;
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    } else {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

