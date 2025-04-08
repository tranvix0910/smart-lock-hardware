#include "button.h"

extern String deviceId;
extern String macAddress;
extern String userId;
extern String faceId;

bool isNormalMode = true;
bool lastButtonStateCapture = HIGH;
bool lastButtonStateReset = HIGH;
unsigned long lastCheck = 0;
bool isFirstRun = true;

// Variables for server-requested fingerprint enrollment are declared in mqtt.cpp
extern bool pendingFingerprintEnroll;
extern String pendingFaceId;

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
    bool statusButtonPinReset = buttonResetRead();
    
    if (statusButtonPinReset == LOW && lastButtonStateReset == HIGH) {
        resetPressStartTime = millis();
        isResetMode = true;
        lastButtonStateReset = LOW;
        Serial.println("Reset mode: Button pressed, waiting for 5 seconds...");
        
        String topicDeleteSubscribe = "server-delete/" + userId + "/" + deviceId;
        if(subscribeTopic(topicDeleteSubscribe.c_str())) {
            Serial.println("Subscribed to topic: " + topicDeleteSubscribe);
        } else {
            Serial.println("Failed to subscribe to topic: " + topicDeleteSubscribe);
        }
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

void enrollFingerprint(DisplayResultCallback displayResultCallback) {
    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before fingerprint enrollment");
        return;
    }
    
    if (pendingFingerprintEnroll) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingFaceId);
        
        if (faceId != pendingFaceId) {
            Serial.println("Face ID mismatch! Cannot enroll fingerprint for different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            // Send failure to server
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["mode"] = "ADD FINGERPRINT FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            extern void publishMessage(const char* topic, const char* message);
            extern String topicAddFingerprintPublish;
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            // Reset the pending state
            pendingFingerprintEnroll = false;
            pendingFaceId = "";
            return;
        }
        
        Serial.println("Face ID match confirmed, proceeding with fingerprint enrollment");
    }
    
    isNormalMode = false;
    fingerprintMode = FINGERPRINT_ENROLL_MODE;
    
    uint8_t newFingerID = getNextFreeID();
    char message[50];
    snprintf(message, sizeof(message), "Add Fingerprint ID: %d", newFingerID);
    
    displayResultCallback(message, TFT_GREEN);
    
    Serial.printf("Ready to enroll fingerprint with ID: %d\n", newFingerID);
    
    bool success = getFingerprintEnroll(newFingerID, displayResultCallback);
    
    // If this is a server-requested enrollment, send the result back
    if (pendingFingerprintEnroll && success) {
        // Get fingerprint template as base64
        String fingerprintTemplate = getLatestFingerprintTemplateAsBase64(newFingerID);
        
        if (fingerprintTemplate.length() > 0) {
            Serial.println("Successfully retrieved fingerprint template");
            Serial.printf("Template length (base64): %d bytes\n", fingerprintTemplate.length());
            
            // Create a document with dynamic size based on template size
            // Add extra space for metadata
            size_t jsonCapacity = fingerprintTemplate.length() + 300;
            DynamicJsonDocument resultDoc(jsonCapacity);
            
            resultDoc["fingerprintId"] = newFingerID;
            resultDoc["fingerprintTemplate"] = fingerprintTemplate;
            resultDoc["mode"] = "ADD FINGERPRINT SUCCESS";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            extern void publishMessage(const char* topic, const char* message);
            extern String topicAddFingerprintPublish;
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent enrollment result with template data");
        } else {
            Serial.println("Failed to retrieve fingerprint template");
            
            // Send basic success message without template
            StaticJsonDocument<200> resultDoc;
            resultDoc["fingerprintId"] = newFingerID;
            resultDoc["mode"] = "ADD FINGERPRINT SUCCESS (NO TEMPLATE)";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            extern void publishMessage(const char* topic, const char* message);
            extern String topicAddFingerprintPublish;
            publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent enrollment result without template data");
        }
        
        // Reset the pending state
        pendingFingerprintEnroll = false;
        pendingFaceId = "";
    } else if (pendingFingerprintEnroll && !success) {
        // Send failure to server
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingFaceId;
        resultDoc["mode"] = "ADD FINGERPRINT FAILED";
        
        String resultJson;
        serializeJson(resultDoc, resultJson);
        
        extern void publishMessage(const char* topic, const char* message);
        extern String topicAddFingerprintPublish;
        publishMessage(topicAddFingerprintPublish.c_str(), resultJson.c_str());
        Serial.println("Sent enrollment failure: " + resultJson);
        
        // Reset the pending state
        pendingFingerprintEnroll = false;
        pendingFaceId = "";
    }
}

void processDeleteFingerprint(uint8_t id, DisplayResultCallback displayResultCallback) {
    bool faceAuthenticated = faceAuthentication();

    if (!faceAuthenticated) {
        displayResultCallback("Face auth required!", TFT_ORANGE);
        Serial.println("Face authentication required before fingerprint deletion");
        return;
    }
    
    if (pendingDeleteFingerprint) {
        Serial.println("Face authenticated, checking if face IDs match");
        Serial.println("Authenticated Face ID: " + faceId);
        Serial.println("Requested Face ID: " + pendingDeleteFaceId);
        
        if (faceId != pendingDeleteFaceId) {
            Serial.println("Face ID mismatch! Cannot delete fingerprint for different face");
            displayResultCallback("Face ID mismatch!", TFT_RED);
            
            // Send failure to server
            StaticJsonDocument<200> resultDoc;
            resultDoc["faceId"] = pendingDeleteFaceId;
            resultDoc["authenticatedFaceId"] = faceId;
            resultDoc["fingerprintId"] = pendingDeleteFingerprintId;
            resultDoc["mode"] = "DELETE FINGERPRINT FAILED: FACE ID MISMATCH";
            
            String resultJson;
            serializeJson(resultDoc, resultJson);
            
            extern void publishMessage(const char* topic, const char* message);
            String topicDeleteFingerprintPublish = "deleteFingerprint-smartlock/" + String(userId) + "/" + String(deviceId);
            publishMessage(topicDeleteFingerprintPublish.c_str(), resultJson.c_str());
            Serial.println("Sent face ID mismatch error: " + resultJson);
            
            // Reset the pending state
            pendingDeleteFingerprint = false;
            pendingDeleteFaceId = "";
            pendingDeleteFingerprintId = -1;
            return;
        }
        
        Serial.println("Face ID match confirmed, proceeding with fingerprint deletion");
    }
    
    // Proceed with fingerprint deletion
    char message[50];
    snprintf(message, sizeof(message), "Deleting Fingerprint ID: %d", id);
    displayResultCallback(message, TFT_CYAN);
    
    Serial.printf("Deleting fingerprint with ID: %d\n", id);
    
    bool success = deleteFingerprint(id, displayResultCallback);
    
    // If this was a server-requested deletion, send the result back
    if (pendingDeleteFingerprint) {
        // Send result to server
        StaticJsonDocument<200> resultDoc;
        resultDoc["faceId"] = pendingDeleteFaceId;
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
        
        extern void publishMessage(const char* topic, const char* message);
        String topicDeleteFingerprintPublish = "deleteFingerprint-smartlock/" + String(userId) + "/" + String(deviceId);
        publishMessage(topicDeleteFingerprintPublish.c_str(), resultJson.c_str());
        Serial.println("Sent deletion result: " + resultJson);
        
        // Reset the pending state
        pendingDeleteFingerprint = false;
        pendingDeleteFaceId = "";
        pendingDeleteFingerprintId = -1;
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

            if (isSystemLockedOut()) {
                displayResultCallback("System Locked!", TFT_RED);
                return;
            }

            // Check for pending fingerprint enrollment request
            extern bool pendingFingerprintEnroll;
            extern String pendingFaceId;
            
            // Check for pending fingerprint deletion request
            extern bool pendingDeleteFingerprint;
            extern String pendingDeleteFaceId;
            extern int pendingDeleteFingerprintId;
            
            if (pendingDeleteFingerprint) {
                Serial.println("Processing pending fingerprint deletion request");
                displayResultCallback("Authenticating face...", TFT_ORANGE);
                
                // Gọi hàm xử lý xóa vân tay
                processDeleteFingerprint(pendingDeleteFingerprintId, displayResultCallback);
                
                return;
            } else if (pendingFingerprintEnroll) {
                Serial.println("Processing pending fingerprint enrollment request");
                displayResultCallback("Authenticating face...", TFT_ORANGE);
                
                // This will trigger face authentication
                // enrollFingerprint handles the face authentication internally
                enrollFingerprint(displayResultCallback);
                
                // After face authentication and fingerprint enrollment, reset the pending state
                pendingFingerprintEnroll = false;
                pendingFaceId = "";
                return;
            }

            // Long press to add fingerprint
            if (pressDuration >= LONG_PRESS_TIME) {
                Serial.println("Long press detected: Add Fingerprint");
                enrollFingerprint(displayResultCallback);
            } else {
                Serial.println("Short press detected: Capture Image");
                handleImageCallback();
            }
        }
        lastMillis = newMillis;
    }
}
