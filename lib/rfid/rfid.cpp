#include "rfid.h"
#include "alert.h"

// Khai báo biến buttonTask
extern TaskHandle_t buttonTask;
SemaphoreHandle_t i2cMutex = NULL;
String failedRFIDEnroll = "";

Adafruit_PN532 nfcI2C(SDA_PIN, SCL_PIN);

void rfidInit() {
    Serial.println("Initializing RFID module...");
    
    // Tạo mutex cho I2C
    if (i2cMutex == NULL) {
        i2cMutex = xSemaphoreCreateMutex();
        if (i2cMutex == NULL) {
            Serial.println("ERROR: Could not create I2C mutex!");
        }
    }
    
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);

    // Reset I2C bus trước khi bắt đầu
    Wire.end();
    delay(100);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(50000);  // Giảm tốc độ để tăng ổn định
    Wire.setTimeout(2000); // Tăng timeout
    
    delay(100);
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        nfcI2C.begin();
        delay(50);
        
        uint32_t versiondata = nfcI2C.getFirmwareVersion();
        if (!versiondata) {
            Serial.println("ERROR: Couldn't find PN53x board. Check connections!");
            xSemaphoreGive(i2cMutex);
            return;
        }

        // Hiển thị thông tin của chip
        Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
        Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
        Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

        // Cấu hình RFID reader
        nfcI2C.SAMConfig();
        Serial.println("RFID Reader Configured");
        
        // Đọc số lượng thẻ đã đăng ký trong EEPROM
        int cardCount = 0;
        for (uint16_t i = 0; i < MAX_RFID_CARDS; i++) {
            uint16_t addr = RFID_START_ADDR + (i * RFID_CARD_SIZE);
            if (EEPROM.read(addr) != RFID_EMPTY_SLOT) {
                cardCount++;
            }
        }
        Serial.print("Found "); Serial.print(cardCount); Serial.println(" registered RFID cards in memory");
        
        // Khởi tạo biến toàn cục cho RFID
        isNormalMode = true;
        
        Serial.println("RFID module initialization complete");
        xSemaphoreGive(i2cMutex);
    }
}

void rfidRead() {
    if (xSemaphoreTake(i2cMutex, 100 / portTICK_PERIOD_MS) != pdTRUE) {
        return;
    }
    
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

            // Phát tiếng bíp khi phát hiện thẻ
            alertBeep(100);

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
                    // Phát tiếng bíp khi thẻ hợp lệ
                    alertBeep(200);
                } else {
                    Serial.println("Access denied - Card is not registered");
                    // Không cần tiếng bíp khi thẻ không hợp lệ
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
    
    xSemaphoreGive(i2cMutex);
}

bool handleAddNewCard(DisplayResultCallback displayResultCallback, uint8_t* uid, uint8_t* uidLength) {
    uint8_t success;
    uint8_t currentUID[7];
    uint8_t currentLength;
    
    // Hiển thị thông báo
    displayResultCallback("Place RFID card", TFT_BLUE);
    Serial.println("Place RFID card to register");
    
    // Chờ quét thẻ
    unsigned long startTime = millis();
    bool cardDetected = false;
    
    while (!cardDetected && millis() - startTime < 10000) { // Timeout 10 giây
        success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, currentUID, &currentLength);
        
        if (success) {
            Serial.println("Card detected");
            Serial.print("UID Length: "); Serial.print(currentLength, DEC); Serial.println(" bytes");
            Serial.print("UID Value: ");
            nfcI2C.PrintHex(currentUID, currentLength);
            
            // Phát tiếng bíp khi phát hiện thẻ
            alertBeep(100);
            
            // Kiểm tra xem thẻ đã tồn tại chưa
            if (EEPROMManager::isRFIDCardExists(currentUID, currentLength)) {
                failedRFIDEnroll = "ADD RFID CARD FAILED: CARD ALREADY EXISTS";
                displayResultCallback("Card already exists!", TFT_RED);
                Serial.println("Card already exists in the system");
                return false;
            }

            memcpy(uid, currentUID, currentLength);
            *uidLength = currentLength;
            
            displayResultCallback("Card scanned", TFT_CYAN);
            cardDetected = true;
        }
        delay(100); // Đợi một chút giữa các lần quét
    }
    
    if (!cardDetected) {
        displayResultCallback("Timeout - No card", TFT_RED);
        Serial.println("Timeout - No card detected");
        return false;
    }
    
    // Lưu vào EEPROM
    displayResultCallback("Saving card...", TFT_CYAN);
    
    if (addNewCard(currentUID, currentLength)) {
        displayResultCallback("Card registered!", TFT_GREEN);
        Serial.println("Card registered successfully!");
        // Phát tiếng bíp thông báo khi đăng ký thẻ thành công
        alertBeep(200);
        return true;
    } else {
        displayResultCallback("Registration failed", TFT_RED);
        Serial.println("Failed to register card");
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
    if (isSystemLockedOut()) {
        displayResultCallback("System Locked!", TFT_RED);
        Serial.println("System locked out!");
        return false;
    }

    Serial.println("Waiting for RFID card...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 5000;
    uint8_t success = 0;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    while (!success && (millis() - startTime < timeout)) {
        if (xSemaphoreTake(i2cMutex, 100 / portTICK_PERIOD_MS) != pdTRUE) {
            vTaskDelay(1);
            continue;
        }
        
        success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        
        if (success) {
            displayResultCallback("Card detected!", TFT_BLUE);
            Serial.println("RFID card detected!");
            alertBeep(100);
        }
        
        xSemaphoreGive(i2cMutex);
        vTaskDelay(5);
        
        if (!success) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
    
    if (!success) {
        displayResultCallback("Timeout", TFT_RED);
        Serial.println("Timeout - No RFID card detected");
        return false;
    }
    
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        nfcI2C.PrintHex(uid, uidLength);
        xSemaphoreGive(i2cMutex);
    }
    
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
    if (xSemaphoreTake(i2cMutex, 50 / portTICK_PERIOD_MS) == pdTRUE) {

        uint8_t success;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        uint8_t uidLength;

        success = nfcI2C.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            if (isNormalMode) {
                isNormalMode = false;
                displayResultCallback("RFID detected...", TFT_CYAN);
            }
            
            xSemaphoreGive(i2cMutex);
            
            if (!isNormalMode) {
                Serial.println("Unlocking with RFID...");
                bool unlockSuccess = unlockWithRFID(displayResultCallback);
                if (!unlockSuccess) {
                    isNormalMode = true;
                }
            }
            vTaskDelay(1);
        } else {
            xSemaphoreGive(i2cMutex);
            vTaskDelay(1);
        }
    } else {
        return;
    }
}

