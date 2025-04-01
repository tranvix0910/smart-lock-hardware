#include "mqtt.h"

WiFiClientSecure net;
PubSubClient AWSIoTClient(net);
const char* CLIENT_ID = "LOCK-001";
const char* USER_ID = "39da459c-4001-704e-8a18-531c910c5e4b";
char TOPIC_SUBSCRIBE[100];
char TOPIC_PUBLISH[100];

void createSubscribeTopic() {
    snprintf(TOPIC_SUBSCRIBE, sizeof(TOPIC_SUBSCRIBE), "server/%s/%s", USER_ID, CLIENT_ID);
    Serial.print("Created subscribe topic: ");
    Serial.println(TOPIC_SUBSCRIBE);
    snprintf(TOPIC_PUBLISH, sizeof(TOPIC_PUBLISH), "smartlock/%s/%s", USER_ID, CLIENT_ID);
    Serial.print("Created publish topic: ");
    Serial.println(TOPIC_PUBLISH);
}

void messageLock(String lockState) {
    StaticJsonDocument<512> doc;
    char output[512];
    
    time_t now;
    time(&now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.000Z", gmtime(&now));
 
    doc["deviceId"] = "LOCK-001";
    doc["userId"] = "39da459c-4001-704e-8a18-531c910c5e4b";
    doc["lockState"] = lockState;
    doc["timestamp"] = timestamp;
    
    serializeJson(doc, output);
    
    Serial.println("Generated lock message:");
    Serial.println(output);

    publishMessage(TOPIC_PUBLISH, output);
}

void processChangeState(const char* deviceId, const char* userId, const char* lockState) {
    if (strcmp(deviceId, CLIENT_ID) != 0 || strcmp(userId, USER_ID) != 0) {
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
    Serial.print("Raw payload: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }
    
    const char* deviceId = doc["deviceId"];
    const char* userId = doc["userId"];
    
    if (!deviceId || !userId) {
        Serial.println("Missing required fields in JSON");
        return;
    }
    
    const char* lockState = doc["lockState"];
    
    processChangeState(deviceId, userId, lockState);
}

void connectToAWSIoTCore() {
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
        return;
    }
    
    createSubscribeTopic();
    if (AWSIoTClient.subscribe(TOPIC_SUBSCRIBE)) {
        Serial.print("Subscribed to topic: ");
        Serial.println(TOPIC_SUBSCRIBE);
    } else {
        Serial.println("Failed to subscribe to topic!");
    }
    
    Serial.println("AWS IoT Connected!");
}

void reconnect() {
    while (!AWSIoTClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (AWSIoTClient.connect(THINGNAME)) {
            Serial.println("connected");
            createSubscribeTopic();
            if (AWSIoTClient.subscribe(TOPIC_SUBSCRIBE)) {
                Serial.print("Subscribed to topic: ");
                Serial.println(TOPIC_SUBSCRIBE);
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
    AWSIoTClient.publish(topic, message);
}

void clientLoop() {
    if (!AWSIoTClient.connected()) {
        reconnect();
    }
    AWSIoTClient.loop();
}

