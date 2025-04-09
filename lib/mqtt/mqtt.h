#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "secrets.h"
#include "lock.h"

// Topic strings
extern String topicPublish;
extern String topicSubscribe;

extern String topicAddFingerprintPublish;
extern String topicAddFingerprintSubscribe;

extern String topicDeleteFingerprintPublish;
extern String topicDeleteFingerprintSubscribe;

extern String topicAddRFIDCardPublish;
extern String topicAddRFIDCardSubscribe;

extern String topicDeleteRFIDCardPublish;
extern String topicDeleteRFIDCardSubscribe;

// Fingerprint enrollment variables
extern bool pendingFingerprintEnroll;
extern String pendingFaceId;

// Fingerprint deletion variables
extern bool pendingDeleteFingerprint;
extern String pendingDeleteFaceId;
extern int pendingDeleteFingerprintId;

// External functions
extern void displayResult(String message, uint16_t color);
extern bool deleteFingerprintProcess(uint8_t id, void (*displayCallback)(String, uint16_t));

bool connectToAWSIoTCore();
bool subscribeTopic(const char* topic);
void handleMessage(char* topic, byte* payload, unsigned int length);
void clientLoop();
void processChangeState(const char* deviceIdParam, const char* userIdParam, const char* lockState);
void reconnect();
void publishMessage(const char* topic, const char* message);
void messageLock(String lockState);
bool isDeviceVerified();

#endif



