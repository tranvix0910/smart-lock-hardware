#include "button.h"

extern String deviceId;
extern String macAddress;
extern String userId;

bool isNormalMode = true;
bool lastButtonState = HIGH;
unsigned long lastCheck = 0;
bool isFirstRun = true;

unsigned long buttonPressStartTime = 0;
uint8_t fingerprintMode = FINGERPRINT_SCAN_MODE;

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
    bool statusButtonPin = buttonResetRead();
    
    if (statusButtonPin == LOW && lastButtonState == HIGH) {
        resetPressStartTime = millis();
        isResetMode = true;
        lastButtonState = LOW;
        Serial.println("Reset mode: Button pressed, waiting for 5 seconds...");
        
        String topicDeleteSubscribe = "server-delete/" + userId + "/" + deviceId;
        if(subscribeTopic(topicDeleteSubscribe.c_str())) {
            Serial.println("Subscribed to topic: " + topicDeleteSubscribe);
        } else {
            Serial.println("Failed to subscribe to topic: " + topicDeleteSubscribe);
        }
    } 
    else if (statusButtonPin == HIGH && lastButtonState == LOW) {
        isResetMode = false;
        lastButtonState = HIGH;
        Serial.println("Reset mode: Button released before 5 seconds");
    }
    else if (isResetMode && statusButtonPin == LOW) {
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
            delay(2000);
        }
    }
}

void checkFingerprintMode(DisplayResultCallback displayResultCallback) {
    lastCheck = millis();
    uint8_t p = finger.getImage();
    
    if (p == FINGERPRINT_OK) {
        if (isNormalMode) {
            isNormalMode = false;
            fingerprintMode = FINGERPRINT_SCAN_MODE;
        }
        if (!isNormalMode && fingerprintMode == FINGERPRINT_SCAN_MODE) {
            bool unlockSuccess = unlockWithFingerprint(displayResultCallback);
        }
    }
}

void buttonEvent(
    HandleImageCallback handleImageCallback, 
    DisplayResultCallback displayResultCallback
) {
    static unsigned long lastMillis = 0;
    static unsigned long pressStartTime = 0;
    static bool isButtonPressed = false;
    unsigned long newMillis = millis();
    bool statusButtonPin = buttonCaptureImageRead();

    if (millis() - lastCheck < 100) {
        return;
    }
    lastCheck = millis();
    
    if (isFirstRun) {
        lastButtonState = statusButtonPin;
        isFirstRun = false;
        Serial.println("First run: Initializing button state to " + 
                      String(lastButtonState == HIGH ? "HIGH (not pressed)" : "LOW (pressed)"));
        lastMillis = newMillis;
        return;
    }

    if (newMillis - lastMillis > 50) { 

        if (statusButtonPin == LOW && lastButtonState == HIGH) {

            lastButtonState = LOW;
            pressStartTime = newMillis;
            isButtonPressed = true;
            Serial.println("Button pressed at: " + String(pressStartTime) + " ms");


        } else if (statusButtonPin == HIGH && lastButtonState == LOW) {

            lastButtonState = HIGH;
            unsigned long pressDuration = newMillis - pressStartTime;
            
            Serial.println("Button released. Press duration: " + String(pressDuration) + " ms");
            
            if (pressDuration > 10000) {
                Serial.println("Unreasonable press duration, ignoring: " + String(pressDuration) + " ms");
                return;
            }

            if (isSystemLockedOut()) {
                displayResultCallback("System Locked!", TFT_RED);
                return;
            }

            if (pressDuration >= LONG_PRESS_TIME) {

                Serial.println("Long press detected: Add Fingerprint");

                bool faceAuthenticated = faceAuthentication();

                if (!faceAuthenticated) {
                    displayResultCallback("Face auth required!", TFT_ORANGE);
                    Serial.println("Face authentication required before fingerprint enrollment");
                    return;
                }
                
                isNormalMode = false;
                fingerprintMode = FINGERPRINT_ENROLL_MODE;
                
                uint8_t newFingerID = getNextFreeID();
                char message[50];
                snprintf(message, sizeof(message), "Add Fingerprint ID: %d", newFingerID);
                
                displayResultCallback(message, TFT_GREEN);
                
                Serial.printf("Ready to enroll fingerprint with ID: %d\n", newFingerID);
                
                getFingerprintEnroll(newFingerID, displayResultCallback);
            } else {
                Serial.println("Short press detected: Capture Image");
                handleImageCallback();
            }
        }
        lastMillis = newMillis;
    }
}
