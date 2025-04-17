#include "button.h"

extern bool isAddingCard;

bool lastButtonStateCapture = HIGH;
bool lastButtonStateReset = HIGH;
unsigned long lastCheck = 0;
bool isFirstRun = true;
unsigned long buttonPressStartTime = 0;

bool isRemovalJustCompleted = false;
unsigned long lastRemovalTime = 0;

#define REMOVAL_COOLDOWN_TIME 2000

#define RESET_PRESS_TIME 5000

void buttonInit() {
    pinMode(BUTTON_CAPTURE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_RESET_PIN, INPUT_PULLUP);
}

bool buttonCaptureImageRead() {
    return digitalRead(BUTTON_CAPTURE_PIN);
}

bool buttonResetRead() {
    return digitalRead(BUTTON_RESET_PIN);
}

void buttonResetMode() {
    static unsigned long resetPressStartTime = 0;
    static bool isResetMode = false;
    bool statusButtonPinReset = buttonResetRead();
    
    if (statusButtonPinReset == LOW && lastButtonStateReset == HIGH) {
        resetPressStartTime = millis();
        isResetMode = true;
        lastButtonStateReset = LOW;
        Serial.println("Reset mode: Button pressed, waiting for 5 seconds...");
    } 
    else if (statusButtonPinReset == HIGH && lastButtonStateReset == LOW) {
        isResetMode = false;
        lastButtonStateReset = HIGH;
        Serial.println("Reset mode: Button released before 5 seconds");
    }
    else if (isResetMode && statusButtonPinReset == LOW) {
        unsigned long pressDuration = millis() - resetPressStartTime;
        
        if (pressDuration >= RESET_PRESS_TIME) {
            Serial.println("Reset mode: 5 seconds reached, sending delete request...");
            
            StaticJsonDocument<200> doc;
            doc["userId"] = userId;
            doc["deviceId"] = deviceId;
            doc["mode"] = "DELETE REQUEST APPCEPT FROM CLIENT";
            
            String topicDelete = "smartlock-delete/" + userId + "/" + deviceId;
            String jsonString;
            serializeJson(doc, jsonString);
            publishMessage(topicDelete.c_str(), jsonString.c_str());
            Serial.println("Delete request sent: " + jsonString);
            Serial.println("Waiting for server confirmation...");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void enrollFingerprint(DisplayResultCallback displayResultCallback) {
    isNormalMode = false;

    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before fingerprint enrollment");
        isNormalMode = true;
        return;
    }
    
    if (pendingFingerprintEnroll) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingFingerprintEnrollFaceId);
        
        if (faceId != pendingFingerprintEnrollFaceId) {
            Serial.println("Face ID mismatch! Cannot enroll fingerprint for different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingFingerprintEnrollFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["mode"] = "ADD FINGERPRINT FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            pendingFingerprintEnroll = false;
            pendingFingerprintEnrollFaceId = "";
            isNormalMode = true;
            return;
        }
        
        Serial.println("Face ID match confirmed, proceeding with fingerprint enrollment");
    }
    
    fingerprintMode = FINGERPRINT_ENROLL_MODE;
    
    uint8_t newFingerID = getNextFreeID();
    char message[50];
    snprintf(message, sizeof(message), "Add Fingerprint ID: %d", newFingerID);
    
    displayResultCallback(message, TFT_GREEN);
    
    Serial.printf("Ready to enroll fingerprint with ID: %d\n", newFingerID);
    
    bool success = getFingerprintEnroll(newFingerID, displayResultCallback);
    
    if (pendingFingerprintEnroll && success) {
        
        String fingerprintTemplate = getLatestFingerprintTemplateAsBase64(newFingerID);
        
        if (fingerprintTemplate.length() > 0) {
            Serial.println("Successfully retrieved fingerprint template");
            Serial.printf("Template length (base64): %d bytes\n", fingerprintTemplate.length());
            
            size_t jsonCapacity = fingerprintTemplate.length() + 300;
            DynamicJsonDocument resultDoc(jsonCapacity);
            
            resultDoc["fingerprintId"] = newFingerID;
            resultDoc["fingerprintTemplate"] = fingerprintTemplate;
            resultDoc["mode"] = "ADD FINGERPRINT SUCCESS";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent enrollment result with template data");
        } else {
            Serial.println("Failed to retrieve fingerprint template");
            
            StaticJsonDocument<200> resultDoc;
            resultDoc["fingerprintId"] = newFingerID;
            resultDoc["mode"] = "ADD FINGERPRINT SUCCESS (NO TEMPLATE)";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent enrollment result without template data");
        }
        
        pendingFingerprintEnroll = false;
        pendingFingerprintEnrollFaceId = "";
    } else if (pendingFingerprintEnroll && !success) {
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingFingerprintEnrollFaceId;
        resultDoc["mode"] = "ADD FINGERPRINT FAILED";
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
        Serial.println("Sent enrollment failure: " + resultJson);
        
        pendingFingerprintEnroll = false;
        pendingFingerprintEnrollFaceId = "";
    }
    
    isNormalMode = true;
}

void processDeleteFingerprint(uint8_t id, DisplayResultCallback displayResultCallback) {
    isNormalMode = false;

    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before fingerprint deletion");
        isNormalMode = true;
        return;
    }
    
    if (pendingDeleteFingerprint) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingDeleteFingerprintFaceId);
        
        if (faceId != pendingDeleteFingerprintFaceId) {
            Serial.println("Face ID mismatch! Cannot delete fingerprint for different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingDeleteFingerprintFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["fingerprintId"] = pendingDeleteFingerprintId;
            resultDoc["mode"] = "DELETE FINGERPRINT FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            publishMessage(topicDeleteFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            pendingDeleteFingerprint = false;
            pendingDeleteFingerprintFaceId = "";
            pendingDeleteFingerprintId = -1;
            isNormalMode = true;
            return;
        }
        Serial.println("Face ID match confirmed, proceeding with fingerprint deletion");
    }
    
    char message[50];
    snprintf(message, sizeof(message), "Deleting Fingerprint ID: %d", id);
    displayResultCallback(message, TFT_CYAN);
    
    Serial.printf("Deleting fingerprint with ID: %d\n", id);
    
    bool success = deleteFingerprint(id, displayResultCallback);

    if (pendingDeleteFingerprint) {

        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingDeleteFingerprintFaceId;
        resultDoc["fingerprintId"] = pendingDeleteFingerprintId;
        
        if (success) {
            resultDoc["mode"] = "DELETE FINGERPRINT SUCCESS";
            displayResultCallback("Fingerprint deleted!", TFT_GREEN);
        } else {
            resultDoc["mode"] = "DELETE FINGERPRINT FAILED";
            displayResultCallback("Deletion failed!", TFT_RED);
        }
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicDeleteFingerprintPublish.c_str(), resultJson.c_str());
        Serial.println("Sent deletion result: " + resultJson);
        
        pendingDeleteFingerprint = false;
        pendingDeleteFingerprintFaceId = "";
        pendingDeleteFingerprintId = -1;
    }
    
    isNormalMode = true;
}

void enrollRFID(DisplayResultCallback displayResultCallback) {
    isNormalMode = false;
    isAddingCard = true;
    
    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before RFID enrollment");
        isAddingCard = false;
        isNormalMode = true;
        return;
    }

    Serial.println("Face authenticated, checking if face IDs match");
    Serial.println("Authenticated Face ID: " + faceId);
    Serial.println("Requested Face ID: " + pendingRFIDEnrollFaceId);
    
    if (faceId != pendingRFIDEnrollFaceId) {
        Serial.println("Face ID mismatch! Cannot enroll RFID for different face");
        displayResultCallback("Face ID mismatch!", TFT_RED);
        
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingRFIDEnrollFaceId;
        resultDoc["authenticatedFaceId"] = faceId;
        resultDoc["mode"] = "ADD RFID CARD FAILED: FACE ID MISMATCH";
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicAddRFIDCardPublish.c_str(), resultJson.c_str());
        Serial.println("Sent face ID mismatch error: " + resultJson);
        
        pendingRFIDEnroll = false;
        pendingRFIDEnrollFaceId = "";
        isNormalMode = true;
        isAddingCard = false;
        return;
    }

    Serial.println("Face ID match confirmed, proceeding with RFID enrollment");

    uint8_t cardUID[7];
    uint8_t uidLength;
    
    bool success = handleAddNewCard(displayResultCallback, cardUID, &uidLength);

    Serial.println("RFID card enrollment result: " + String(success));
    
    if (success) {
        Serial.println("RFID card enrollment successful");
        
        StaticJsonDocument<300> resultDoc;
        resultDoc["faceId"] = pendingRFIDEnrollFaceId;
        resultDoc["cardUID"] = rfidUIDToString(cardUID, uidLength);
        resultDoc["uidLength"] = uidLength;
        resultDoc["mode"] = "ADD RFID CARD SUCCESS";
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        publishMessage(topicAddRFIDCardPublish.c_str(), resultJson.c_str());
        Serial.println("Sent RFID enrollment success: " + resultJson);
        
        displayResultCallback("Card added to account!", TFT_GREEN);
    } else {
        Serial.println("RFID card enrollment failed");

        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingRFIDEnrollFaceId;
        resultDoc["mode"] = failedRFIDEnroll;

        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicAddRFIDCardPublish.c_str(), resultJson.c_str());
        Serial.println("Sent RFID enrollment failure: " + resultJson);
    }
    
    Serial.println("Resetting pending RFID enrollment request");
    pendingRFIDEnroll = false;
    pendingRFIDEnrollFaceId = "";
    isNormalMode = true;
    isAddingCard = false;
}

void processRemoveRFIDCard(DisplayResultCallback displayResultCallback, uint8_t* targetUID, uint8_t targetUIDLength) {
    isNormalMode = false;
    
    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before RFID card removal");
        isNormalMode = true;
        return;
    }
    
    if (pendingRemoveRFIDCard) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingRemoveRFIDCardFaceId);
        
        if (faceId != pendingRemoveRFIDCardFaceId) {
            Serial.println("Face ID mismatch! Cannot remove RFID card for different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingRemoveRFIDCardFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["cardUID"] = pendingRemoveRFIDCardUID;
            resultDoc["mode"] = "REMOVE RFID CARD FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            publishMessage(topicRemoveRFIDCardPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            pendingRemoveRFIDCard = false;
            pendingRemoveRFIDCardFaceId = "";
            pendingRemoveRFIDCardUID = "";
            pendingRemoveRFIDCardUIDLength = "4 Bytes";
            isNormalMode = true;
            return;
        }
        Serial.println("Face ID match confirmed, proceeding with RFID card removal");
    }
    
    displayResultCallback("Removing card...", TFT_CYAN);
    
    Serial.printf("Removing RFID card with UID: %s\n", pendingRemoveRFIDCardUID.c_str());
    
    bool success = removeCard(targetUID, targetUIDLength);
    
    if (pendingRemoveRFIDCard) {
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingRemoveRFIDCardFaceId;
        resultDoc["cardUID"] = pendingRemoveRFIDCardUID;
        
        if (success) {
            resultDoc["mode"] = "REMOVE RFID CARD SUCCESS";
            displayResultCallback("Card removed!", TFT_GREEN);
            
            isRemovalJustCompleted = true;
            lastRemovalTime = millis();
            displayResultCallback("Returning to home...", TFT_BLUE);
        } else {
            resultDoc["mode"] = "REMOVE RFID CARD FAILED";
            displayResultCallback("Removal failed", TFT_RED);
        }
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicRemoveRFIDCardPublish.c_str(), resultJson.c_str());
        Serial.println("Sent removal result: " + resultJson);
        
        pendingRemoveRFIDCard = false;
        pendingRemoveRFIDCardFaceId = "";
        pendingRemoveRFIDCardUID = "";
        pendingRemoveRFIDCardUIDLength = "4 Bytes";
    }
    
    isNormalMode = true;
}

void processUnlockSystem(DisplayResultCallback displayResultCallback) {
    
    isNormalMode = false;
    
    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required for system unlock");
        isNormalMode = true;
        return;
    }
    
    if (pendingUnlockSystem) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingUnlockSystemFaceId);
        
        if (faceId != pendingUnlockSystemFaceId) {
            Serial.println("Face ID mismatch! Cannot unlock system with different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingUnlockSystemFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["mode"] = "UNLOCK SYSTEM FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            publishMessage(topicUnlockSystemPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            pendingUnlockSystem = false;
            pendingUnlockSystemFaceId = "";
            isNormalMode = true;
            return;
        }
        
        Serial.println("Face ID match confirmed, proceeding with system unlock");
        
        displayResultCallback("Unlocking system...", TFT_GREEN);
        
        // Mở khóa hệ thống
        unlockSystem();
        
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingUnlockSystemFaceId;
        resultDoc["mode"] = "UNLOCK SYSTEM SUCCESS";
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        publishMessage(topicUnlockSystemPublish.c_str(), resultJson.c_str());
        Serial.println("Sent unlock success: " + resultJson);
        
        pendingUnlockSystem = false;
        pendingUnlockSystemFaceId = "";
    }
    
    isNormalMode = true;
}

void buttonEvent(
    HandleImageCallback handleImageCallback, 
    DisplayResultCallback displayResultCallback,
    DisplayCornerTextCallback displayCornerText
) {

    static unsigned long lastMillis = 0;
    static unsigned long pressStartTime = 0;
    static bool isButtonPressed = false;
    unsigned long newMillis = millis();
    bool statusButtonPinCapture = buttonCaptureImageRead();

    if (millis() - lastCheck < 100) {
        return;
    }

    lastCheck = millis();
    
    if (isFirstRun) {
        lastButtonStateCapture = statusButtonPinCapture;
        isFirstRun = false;
        Serial.println("First run: Initializing button state to " + 
                      String(lastButtonStateCapture == HIGH ? "HIGH (not pressed)" : "LOW (pressed)"));
        lastMillis = newMillis;
        return;
    }

    if (isRemovalJustCompleted && (millis() - lastRemovalTime < REMOVAL_COOLDOWN_TIME)) {
        return;
    } else if (isRemovalJustCompleted) {
        isRemovalJustCompleted = false;
        displayResultCallback("Ready", TFT_BLACK);
    }

    if(pendingDeleteFingerprint){
        displayCornerText("Deleting Fingerprint", TFT_RED, 1);
    }

    if(pendingFingerprintEnroll){
        displayCornerText("Enrolling Fingerprint", TFT_GREEN, 1);
    }

    if(pendingRFIDEnroll){
        displayCornerText("Enrolling RFID", TFT_BLUE, 1);
    }

    if(pendingRemoveRFIDCard){
        displayCornerText("Removing RFID", TFT_RED, 1);
    }

    if(pendingUnlockSystem){
        displayCornerText("Emergency Unlock", TFT_GREEN, 1);
    }

    if (newMillis - lastMillis > 50) { 

        if (statusButtonPinCapture == LOW && lastButtonStateCapture == HIGH) {

            lastButtonStateCapture = LOW;
            pressStartTime = newMillis;
            isButtonPressed = true;
            Serial.println("Button pressed at: " + String(pressStartTime) + " ms");

        } else if (statusButtonPinCapture == HIGH && lastButtonStateCapture == LOW) {

            lastButtonStateCapture = HIGH;
            unsigned long pressDuration = newMillis - pressStartTime;
            
            Serial.println("Button released. Press duration: " + String(pressDuration) + " ms");
            
            if (pressDuration > 10000) {
                Serial.println("Unreasonable press duration, ignoring: " + String(pressDuration) + " ms");
                return;
            }

            if (pendingDeleteFingerprint) {
                Serial.println("Processing pending fingerprint deletion request");
                displayResultCallback("Authenticating face", TFT_ORANGE);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                
                processDeleteFingerprint(pendingDeleteFingerprintId, displayResultCallback);
                return;
            } else if (pendingFingerprintEnroll) {
                Serial.println("Processing pending fingerprint enrollment request");
                displayResultCallback("Authenticating face", TFT_ORANGE);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                
                enrollFingerprint(displayResultCallback);
                return;
            } else if (pendingRFIDEnroll) {
                Serial.println("Processing pending RFID enrollment request");
                displayResultCallback("Authenticating face", TFT_ORANGE);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                
                enrollRFID(displayResultCallback);
                return;
            } else if (pendingRemoveRFIDCard) {
                Serial.println("Processing pending RFID card removal request");
                displayResultCallback("Authenticating face", TFT_ORANGE);
                vTaskDelay(3000 / portTICK_PERIOD_MS);

                uint8_t cardUID[10];
                uint8_t uidLength = 0;
                
                String uidStr = pendingRemoveRFIDCardUID;
                int idx = 0;
                int pos = 0;
                
                while (pos < uidStr.length() && idx < 10) {
                    int delimPos = uidStr.indexOf(':', pos);
                    String byteStr = (delimPos > 0) ? uidStr.substring(pos, delimPos) : uidStr.substring(pos);
                    cardUID[idx++] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
                    pos = (delimPos > 0) ? delimPos + 1 : uidStr.length();
                }
                uidLength = idx;
                
                processRemoveRFIDCard(displayResultCallback, cardUID, uidLength);
                return;
            } else if (pendingUnlockSystem) {
                Serial.println("Processing pending system unlock request");
                displayResultCallback("Authenticating face", TFT_ORANGE);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                
                processUnlockSystem(displayResultCallback);
                return;
            }
            if (pressDuration >= LONG_PRESS_TIME) {
                Serial.println("Long press detected: Add Fingerprint");
                isNormalMode = false;
                enrollFingerprint(displayResultCallback);
                isNormalMode = true;
            } else {
                Serial.println("Short press detected: Capture Image");
                handleImageCallback();
            }
        }
        lastMillis = newMillis;
    }
}
