#include "mqtt.h"

extern String deviceId;
extern String macAddress;
extern String userId;

// Variables for server-requested fingerprint enrollment - defined here
bool pendingFingerprintEnroll = false;
String pendingFaceId = "";

// Variables for server-requested fingerprint deletion
bool pendingDeleteFingerprint = false;
String pendingDeleteFaceId = "";
int pendingDeleteFingerprintId = -1;

// Variables for server-requested RFID enrollment
String pendingRFIDFaceId = "";
bool pendingRFIDEnroll = false;

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

String topicDeleteRFIDCardPublish;
String topicDeleteRFIDCardSubscribe;

String topicRecentAccessPublish;
String topicRecentAccessSubscribe;

bool subscribeTopic(const char* topic) {
    if (AWSIoTClient.subscribe(topic)) {
        Serial.print("Subscribed to topic: ");
        Serial.println(topic);
        return true;
    }
    return false;
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

    // Phân tích JSON
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
                
            for(int i = 0; i < EEPROM.length(); i++) {
                EEPROM.write(i, 0);
            }
            
            if (!EEPROM.commit()) {
                Serial.println("Error committing to EEPROM!");
                return;
            }
                
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

            delay(100);
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
                pendingFaceId = faceIdReceived;
                
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
                
                // Yêu cầu xác thực khuôn mặt
                Serial.println("Face authentication required before fingerprint deletion");
                delay(3000);
                
                // Lưu thông tin yêu cầu để xử lý sau khi xác thực khuôn mặt
                pendingDeleteFingerprint = true;
                pendingDeleteFaceId = requestedFaceId;
                pendingDeleteFingerprintId = fingerprintIdStr.toInt();
                
                // Gửi phản hồi cho server biết hệ thống đang đợi xác thực
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
                
                // Set the pending flag and store the face ID
                pendingRFIDEnroll = true;
                pendingRFIDFaceId = receivedFaceId;
                
                // Send acknowledgment
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
        delay(100);
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

    topicDeleteRFIDCardPublish = "deleteRFIDCard-smartlock/" + String(userId) + "/" + String(deviceId);
    topicDeleteRFIDCardSubscribe = "deleteRFIDCard-server/" + String(userId) + "/" + String(deviceId);

    topicRecentAccessPublish = "recentAccess-smartlock/" + String(userId) + "/" + String(deviceId);
    topicRecentAccessSubscribe = "recentAccess-server/" + String(userId) + "/" + String(deviceId);

    subscribeTopic(topicSubscribe.c_str());
    subscribeTopic(topicAddFingerprintSubscribe.c_str());
    subscribeTopic(topicDeleteFingerprintSubscribe.c_str());
    subscribeTopic(topicAddRFIDCardSubscribe.c_str());
    subscribeTopic(topicDeleteRFIDCardSubscribe.c_str());
    subscribeTopic(topicRecentAccessSubscribe.c_str());
    subscribeTopic(topicDeleteSubscribe.c_str());
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
            subscribeTopic(topicDeleteRFIDCardSubscribe.c_str());
            subscribeTopic(topicRecentAccessSubscribe.c_str());

            Serial.println("AWS IoT Connected!");
        } else {
            Serial.print("failed, rc=");
            Serial.print(AWSIoTClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void clientLoop() {
    if (!AWSIoTClient.connected()) {
        reconnect();
    }
    AWSIoTClient.loop();
}

