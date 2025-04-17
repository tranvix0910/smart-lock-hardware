#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>

#include "secrets.h"
#include "lock.h"
#include "recentAccessLogs.h"
#include "eeprom_manager.h"

extern void deleteAllFingerprints();

extern String deviceId;
extern String macAddress;
extern String userId;

// Topic strings
extern String topicPublish;
extern String topicSubscribe;

extern String topicAddFingerprintPublish;
extern String topicAddFingerprintSubscribe;

extern String topicDeleteFingerprintPublish;
extern String topicDeleteFingerprintSubscribe;

extern String topicAddRFIDCardPublish;
extern String topicAddRFIDCardSubscribe;

extern String topicRemoveRFIDCardPublish;
extern String topicRemoveRFIDCardSubscribe;

extern String topicRecentAccessPublish;
extern String topicRecentAccessSubscribe;

// Fingerprint enrollment variables
extern bool pendingFingerprintEnroll;
extern String pendingFingerprintEnrollFaceId;

// Fingerprint deletion variables
extern bool pendingDeleteFingerprint;
extern String pendingDeleteFingerprintFaceId;
extern int pendingDeleteFingerprintId;

// RFID enrollment variables
extern bool pendingRFIDEnroll;
extern String pendingRFIDEnrollFaceId;
extern String pendingRFIDDeleteUIDLength;

// External functions
extern void displayResult(String message, uint16_t color);
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



