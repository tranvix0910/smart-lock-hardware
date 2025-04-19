#include "mqtt.h"

// Fingerprint Enroll
bool pendingFingerprintEnroll = false;
String pendingFingerprintEnrollFaceId = "";

// Fingerprint Delete
bool pendingDeleteFingerprint = false;
String pendingDeleteFingerprintFaceId = "";
int pendingDeleteFingerprintId = -1;

// RFID Enroll
String pendingRFIDEnrollFaceId = "";
bool pendingRFIDEnroll = false;

// RFID Delete
bool pendingRemoveRFIDCard = false;
String pendingRemoveRFIDCardFaceId = "";
String pendingRemoveRFIDCardUID = "";
String pendingRemoveRFIDCardUIDLength = "4 Bytes";

// Unlock System
bool pendingUnlockSystem = false;
String pendingUnlockSystemFaceId = "";

WiFiClientSecure net;
PubSubClient AWSIoTClient(net);
bool deviceVerified = false;

String topicPublish;
String topicSubscribe;

String topicDeleteSubscribe;

String topicAddFingerprintPublish;
String topicAddFingerprintSubscribe;

String topicDeleteFingerprintPublish;
String topicDeleteFingerprintSubscribe;

String topicAddRFIDCardPublish;
String topicAddRFIDCardSubscribe;

String topicRemoveRFIDCardPublish;
String topicRemoveRFIDCardSubscribe;

String topicRecentAccessPublish;
String topicRecentAccessSubscribe;

String topicUnlockSystemPublish;
String topicUnlockSystemSubscribe;

bool subscribeTopic(const char* topic) {
    if (AWSIoTClient.subscribe(topic)) {
        Serial.print("Subscribed to topic: ");
        Serial.println(topic);
        return true;
    }
    return false;
}

void publishMessage(const char* topic, const char* message) {
    if (AWSIoTClient.publish(topic, message)) {
        Serial.print("Message published to topic: ");
        Serial.println(topic);
    } else {
        Serial.print("Failed to publish message to topic: ");
        Serial.println(topic);
    }
}

void messageLock(String lockState) {
    StaticJsonDocument<512> doc;
    char output[512];
    
    time_t now;
    time(&now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.000Z", gmtime(&now));
 
    doc["deviceId"] = deviceId;
    doc["userId"] = userId;
    doc["lockState"] = lockState;
    doc["timestamp"] = timestamp;
    
    serializeJson(doc, output);
    
    Serial.println("Generated lock message:");
    Serial.println(output);

    publishMessage(topicPublish.c_str(), output);
}

void processChangeState(const char* deviceIdParam, const char* userIdParam, const char* lockState) {
    if (strcmp(deviceIdParam, deviceId.c_str()) != 0 || strcmp(userIdParam, userId.c_str()) != 0) {
        Serial.println("Message not for this device");
        return;
    }
    
    if (strcmp(lockState, "UNLOCK") == 0) {
        Serial.println("Command: UNLOCK");
        lockOpen();
    } else if (strcmp(lockState, "LOCK") == 0) {
        Serial.println("Command: LOCK");
        lockClose();
    } else {
        Serial.print("Unknown lock state: ");
        Serial.println(lockState);
    }
}

void handleMessage(char* topic, byte* payload, unsigned int length) {
    Serial.println("\n=== Received MQTT Message ===");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload length: ");
    Serial.println(length);
    
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Raw payload: ");
    Serial.println(message);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    String topicString = String(topic);

    if (topicString.startsWith("connect/") && doc.containsKey("deviceId") && doc.containsKey("macAddress") && 
        doc.containsKey("secretKey") && doc.containsKey("userId")) {
        
        // Lấy thông tin từ message
        String receivedDeviceId = doc["deviceId"].as<String>();
        String receivedMacAddress = doc["macAddress"].as<String>();
        String receivedSecretKey = doc["secretKey"].as<String>();
        String receivedUserId = doc["userId"].as<String>();
        extern String deviceId;
        extern String macAddress;
        extern String secretKey;
        extern String userId;
        
        Serial.println("\n=== Comparing Device Information ===");
        Serial.println("Received DeviceID: " + receivedDeviceId + " | Local: " + deviceId);
        Serial.println("Received MAC: " + receivedMacAddress + " | Local: " + macAddress);
        Serial.println("Received SecretKey: " + receivedSecretKey + " | Local: " + secretKey);
        
        // So sánh thông tin
        if (receivedDeviceId == deviceId && 
            receivedMacAddress == macAddress && 
            receivedSecretKey == secretKey) {
            
            Serial.println("Device verification SUCCESS!");
            deviceVerified = true;
            
            // Lưu userId
            userId = receivedUserId;
            
            // Lưu vào EEPROM
            extern void saveUserId(String id);
            saveUserId(userId);
            
            return;
        } else {
            Serial.println("Device verification FAILED!");
            deviceVerified = false;
        }
    }

    if(topicString.startsWith("server-delete/")) {
        Serial.println("Delete device message");
        String modeReceived = doc["mode"].as<String>();
        if(modeReceived == "DELETED DEVICE FROM SERVER") {
            Serial.println("Delete request accepted");
            deviceVerified = false;
            Serial.println("Server confirmed deletion, clearing EEPROM...");
            
            EEPROMManager::beginBatchWrite();
            
            EEPROMManager::clearConfig(false);
            
            EEPROMManager::clearAllRFIDCards(false);

            deleteAllFingerprints();

            EEPROMManager::endBatchWrite();
            
            Serial.println("EEPROM cleared successfully!");
            Serial.println("Device will restart in 2 seconds...");
            
            StaticJsonDocument<200> confirmDoc;
            confirmDoc["userId"] = userId;
            confirmDoc["deviceId"] = deviceId;
            confirmDoc["mode"] = "DEVICE DELETED";
            
            String confirmJson;
            serializeJson(confirmDoc, confirmJson);
            
            String topicDelete = "smartlock-delete/" + userId + "/" + deviceId;
            publishMessage(topicDelete.c_str(), confirmJson.c_str());
            Serial.println("Deletion confirmation sent: " + confirmJson);

            vTaskDelay(100 / portTICK_PERIOD_MS);
            ESP.restart();
        } else {
            Serial.println("No confirmation received from server");
        }
    }

    if(topicString.startsWith("addFingerprint-server/")) {
        Serial.println("Received fingerprint addition request");
        
        if (doc.containsKey("mode") && 
            doc.containsKey("userId") && 
            doc.containsKey("deviceId") && 
            doc.containsKey("faceId")) {
            
            String modeReceived = doc["mode"].as<String>();
            String receivedUserId = doc["userId"].as<String>();
            String receivedDeviceId = doc["deviceId"].as<String>();
            String faceIdReceived = doc["faceId"].as<String>();
            
            if (modeReceived == "ADD FINGERPRINT REQUEST FROM SERVER" && 
                receivedUserId == userId && 
                receivedDeviceId == deviceId) {
                
                Serial.println("Valid fingerprint addition request received");
                Serial.println("Face ID: " + faceIdReceived);
                
                // Set the pending flag and store the face ID
                pendingFingerprintEnroll = true;
                pendingFingerprintEnrollFaceId = faceIdReceived;
                
                // Send acknowledgment
                StaticJsonDocument<200> responseDoc;
                responseDoc["userId"] = userId;
                responseDoc["deviceId"] = deviceId;
                responseDoc["faceId"] = faceIdReceived;
                responseDoc["mode"] = "ADD FINGERPRINT REQUEST ACCEPTED";
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                publishMessage(topicAddFingerprintPublish.c_str(), responseJson.c_str());
                Serial.println("Sent acknowledgment: " + responseJson);
            } else {
                Serial.println("Invalid request or not for this device");
            }
        } else {
            Serial.println("Missing required fields in request");
        }
    }

    if(topicString.startsWith("deleteFingerprint-server/")) {
        Serial.println("Received fingerprint deletion request");
        
        if (doc.containsKey("mode") && 
            doc.containsKey("faceId") &&
            doc.containsKey("fingerprintId")) {
            
            String modeReceived = doc["mode"].as<String>();
            String requestedFaceId = doc["faceId"].as<String>();
            String fingerprintIdStr = doc["fingerprintId"].as<String>();
            
            if (modeReceived == "DELETE FINGERPRINT REQUEST FROM SERVER") {
                Serial.printf("Processing delete request for fingerprint ID: %s, Face ID: %s\n", 
                             fingerprintIdStr.c_str(), requestedFaceId.c_str());
                
                Serial.println("Face authentication required before fingerprint deletion");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                
                pendingDeleteFingerprint = true;
                pendingDeleteFingerprintFaceId = requestedFaceId;
                pendingDeleteFingerprintId = fingerprintIdStr.toInt();
                
                StaticJsonDocument<200> responseDoc;
                responseDoc["faceId"] = requestedFaceId;
                responseDoc["fingerprintId"] = fingerprintIdStr;
                responseDoc["mode"] = "DELETE FINGERPRINT ACCEPTED";
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                publishMessage(topicDeleteFingerprintPublish.c_str(), responseJson.c_str());
                Serial.println("Sent waiting for authentication response: " + responseJson);
            } else {
                Serial.println("Invalid mode received");
            }
        } else {
            Serial.println("Missing required fields in fingerprint deletion request");
        }
    }

    if(topicString.startsWith("addRFIDCard-server/")) {
        if(doc.containsKey("mode") && 
            doc.containsKey("faceId") && 
            doc.containsKey("deviceId") && 
            doc.containsKey("userId")
        ) {
            String modeReceived = doc["mode"].as<String>();
            String receivedFaceId = doc["faceId"].as<String>();
            String receivedDeviceId = doc["deviceId"].as<String>();
            String receivedUserId = doc["userId"].as<String>();
            
            if(modeReceived == "ADD RFID CARD REQUEST FROM SERVER" && 
                receivedUserId == userId && 
                receivedDeviceId == deviceId) {
                
                Serial.println("Received RFID card addition request");
                Serial.println("Face ID: " + receivedFaceId);
                
                pendingRFIDEnroll = true;
                pendingRFIDEnrollFaceId = receivedFaceId;
                
                StaticJsonDocument<200> responseDoc;
                responseDoc["faceId"] = receivedFaceId;
                responseDoc["mode"] = "ADD RFID CARD REQUEST ACCEPTED";
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                String topicAddRFIDCardPublish = "addRFIDCard-smartlock/" + String(userId) + "/" + String(deviceId);
                publishMessage(topicAddRFIDCardPublish.c_str(), responseJson.c_str());
                Serial.println("Sent acknowledgment: " + responseJson);
            } else {
                Serial.println("Invalid request or not for this device");
            }
        } else {
            Serial.println("Missing required fields in request");
        }
    }

    if(topicString.startsWith("deleteRFIDCard-server/")) {
        Serial.println("Received RFID card deletion request");
        if(doc.containsKey("mode") && doc.containsKey("faceId") && doc.containsKey("rfidId")) {
            
            String modeReceived = doc["mode"].as<String>();
            String receivedFaceId = doc["faceId"].as<String>();
            String receivedUID = doc["rfidId"].as<String>();
            String receivedUIDLength = doc["rfidIdLength"].as<String>();

            if(modeReceived == "DELETE RFID CARD REQUEST FROM SERVER") {
                
                Serial.println("Valid RFID card deletion request received");
                Serial.println("Face ID: " + receivedFaceId);
                
                pendingRemoveRFIDCard = true;
                pendingRemoveRFIDCardFaceId = receivedFaceId;
                pendingRemoveRFIDCardUID = receivedUID;
                pendingRemoveRFIDCardUIDLength = receivedUIDLength;
                
                StaticJsonDocument<200> responseDoc;
                responseDoc["faceId"] = receivedFaceId;
                responseDoc["rfidId"] = receivedUID;
                responseDoc["rfidIdLength"] = receivedUIDLength;
                responseDoc["mode"] = "DELETE RFID CARD ACCEPTED";
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                String topicRemoveRFIDCardPublish = "deleteRFIDCard-smartlock/" + String(userId) + "/" + String(deviceId);
                publishMessage(topicRemoveRFIDCardPublish.c_str(), responseJson.c_str());
                Serial.println("Sent acknowledgment: " + responseJson);
            } else {
                Serial.println("Invalid request or not for this device");
            }
        } else {
            Serial.println("Missing required fields in request");
        }
    }

    if(topicString.startsWith("unlockSystem-server/")) {
        Serial.println("Received unlock system request");
        if(doc.containsKey("mode") && doc.containsKey("faceId")) {

            String modeReceived = doc["mode"].as<String>();
            String receivedFaceId = doc["faceId"].as<String>();

            if(modeReceived == "UNLOCK SYSTEM REQUEST FROM SERVER") {
                pendingUnlockSystem = true;
                pendingUnlockSystemFaceId = receivedFaceId;

                if (isEmergencyLocked) {
                    Serial.println("Resuming tasks for face authentication");
                    if (webSocketTask != NULL) vTaskResume(webSocketTask);
                    if (buttonTask != NULL) vTaskResume(buttonTask);
                }

                StaticJsonDocument<200> responseDoc;
                responseDoc["userId"] = userId;
                responseDoc["deviceId"] = deviceId;
                responseDoc["faceId"] = receivedFaceId;
                responseDoc["mode"] = "UNLOCK SYSTEM ACCEPTED";
                
                String responseJson;
                serializeJson(responseDoc, responseJson);
                
                publishMessage(topicUnlockSystemPublish.c_str(), responseJson.c_str());
                Serial.println("Sent acknowledgment: " + responseJson);
            } else {
                Serial.println("Invalid request or not for this device");
            }
        } else {
            Serial.println("Missing required fields in request");
        }
    }

    if (doc.containsKey("deviceId") && doc.containsKey("userId") && doc.containsKey("lockState")) {
        const char* receivedDeviceId = doc["deviceId"];
        const char* receivedUserId = doc["userId"];
        const char* lockState = doc["lockState"];
        
        processChangeState(receivedDeviceId, receivedUserId, lockState);

        publishRecentAccessLogs("WEB_APP", "SUCCESS", "ACCOUNT USER", "Accessed Via Web App");
    }
}

bool isDeviceVerified() {
    return deviceVerified;
}

bool connectToAWSIoTCore() {
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);
    
    AWSIoTClient.setServer(AWS_IOT_ENDPOINT, 8883);
    AWSIoTClient.setCallback(handleMessage);
    
    Serial.println("Connecting to AWS IOT");
    
    while (!AWSIoTClient.connect(THINGNAME))
    {
        Serial.print(".");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    if (!AWSIoTClient.connected())
    {
        Serial.println("AWS IoT Timeout!");
        return false;
    }

    topicPublish = "smartlock/" + String(userId) + "/" + String(deviceId);
    topicSubscribe = "server/" + String(userId) + "/" + String(deviceId);

    topicDeleteSubscribe = "server-delete/" + userId + "/" + deviceId;

    topicAddFingerprintPublish = "addFingerprint-smartlock/" + String(userId) + "/" + String(deviceId);
    topicAddFingerprintSubscribe = "addFingerprint-server/" + String(userId) + "/" + String(deviceId);

    topicDeleteFingerprintPublish = "deleteFingerprint-smartlock/" + String(userId) + "/" + String(deviceId);
    topicDeleteFingerprintSubscribe = "deleteFingerprint-server/" + String(userId) + "/" + String(deviceId);

    topicAddRFIDCardPublish = "addRFIDCard-smartlock/" + String(userId) + "/" + String(deviceId);
    topicAddRFIDCardSubscribe = "addRFIDCard-server/" + String(userId) + "/" + String(deviceId);

    topicRemoveRFIDCardPublish = "deleteRFIDCard-smartlock/" + String(userId) + "/" + String(deviceId);
    topicRemoveRFIDCardSubscribe = "deleteRFIDCard-server/" + String(userId) + "/" + String(deviceId);

    topicRecentAccessPublish = "recentAccess-smartlock/" + String(userId) + "/" + String(deviceId);
    topicRecentAccessSubscribe = "recentAccess-server/" + String(userId) + "/" + String(deviceId);

    topicUnlockSystemPublish = "unlockSystem-smartlock/" + String(userId) + "/" + String(deviceId);
    topicUnlockSystemSubscribe = "unlockSystem-server/" + String(userId) + "/" + String(deviceId);

    subscribeTopic(topicSubscribe.c_str());
    subscribeTopic(topicAddFingerprintSubscribe.c_str());
    subscribeTopic(topicDeleteFingerprintSubscribe.c_str());
    subscribeTopic(topicAddRFIDCardSubscribe.c_str());
    subscribeTopic(topicRemoveRFIDCardSubscribe.c_str());
    subscribeTopic(topicRecentAccessSubscribe.c_str());
    subscribeTopic(topicDeleteSubscribe.c_str());
    subscribeTopic(topicUnlockSystemSubscribe.c_str());
    Serial.println("AWS IoT Connected!");
    return true;
}

void reconnect() {
    while (!AWSIoTClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (AWSIoTClient.connect(THINGNAME)) {
            Serial.println("connected");
            subscribeTopic(topicSubscribe.c_str());
            subscribeTopic(topicAddFingerprintSubscribe.c_str());
            subscribeTopic(topicDeleteSubscribe.c_str());
            subscribeTopic(topicDeleteFingerprintSubscribe.c_str());
            subscribeTopic(topicAddRFIDCardSubscribe.c_str());
            subscribeTopic(topicRemoveRFIDCardSubscribe.c_str());
            subscribeTopic(topicRecentAccessSubscribe.c_str());
            subscribeTopic(topicUnlockSystemSubscribe.c_str());

            Serial.println("AWS IoT Connected!");
        } else {
            Serial.print("failed, rc=");
            Serial.print(AWSIoTClient.state());
            Serial.println(" try again in 5 seconds");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }
}

void clientLoop() {
    if (!AWSIoTClient.connected()) {
        reconnect();
    }
    AWSIoTClient.loop();
}