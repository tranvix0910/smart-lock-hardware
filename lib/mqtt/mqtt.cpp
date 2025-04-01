#include "mqtt.h"

// Khai báo extern cho các biến toàn cục từ wifi_config.cpp
extern String deviceId;
extern String macAddress;
extern String userId;

WiFiClientSecure net;
PubSubClient AWSIoTClient(net);
bool deviceVerified = false;

String topicPublish;
String topicSubscribe;

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

    // Connect device message
    if (doc.containsKey("deviceId") && doc.containsKey("macAddress") && 
        doc.containsKey("secretKey") && doc.containsKey("userId")) {
        
        // Lấy thông tin từ message
        String receivedDeviceId = doc["deviceId"].as<String>();
        String receivedMacAddress = doc["macAddress"].as<String>();
        String receivedSecretKey = doc["secretKey"].as<String>();
        String receivedUserId = doc["userId"].as<String>();
        
        // Lấy thông tin thiết bị hiện tại
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
    
    // Change state message
    if (doc.containsKey("deviceId") && doc.containsKey("userId") && doc.containsKey("lockState")) {
        const char* receivedDeviceId = doc["deviceId"];
        const char* receivedUserId = doc["userId"];
        const char* lockState = doc["lockState"];
        
        processChangeState(receivedDeviceId, receivedUserId, lockState);
    }
}

// Hàm kiểm tra xem thiết bị đã được xác thực chưa
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

    subscribeTopic(topicSubscribe.c_str());
    
    Serial.println("AWS IoT Connected!");
    return true;
    
    // createSubscribeTopic();
    // if (AWSIoTClient.subscribe(TOPIC_SUBSCRIBE)) {
    //     Serial.print("Subscribed to topic: ");
    //     Serial.println(TOPIC_SUBSCRIBE);
    // } else {
    //     Serial.println("Failed to subscribe to topic!");
    // }
    
}

void reconnect() {
    while (!AWSIoTClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (AWSIoTClient.connect(THINGNAME)) {
            Serial.println("connected");
            subscribeTopic(topicSubscribe.c_str());
            if (AWSIoTClient.subscribe(topicSubscribe.c_str())) {
                Serial.print("Subscribed to topic: ");
                Serial.println(topicSubscribe.c_str());
            } else {
                Serial.println("Failed to subscribe to topic!");
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(AWSIoTClient.state());
            Serial.println(" try again in 2 seconds");
            delay(2000);
        }
    }
}

void publishMessage(const char* topic, const char* message) {
    Serial.print("Publishing message to topic: ");
    Serial.println(topic);
    Serial.println(message);
    AWSIoTClient.publish(topic, message);
}

void clientLoop() {
    if (!AWSIoTClient.connected()) {
        reconnect();
    }
    AWSIoTClient.loop();
}

