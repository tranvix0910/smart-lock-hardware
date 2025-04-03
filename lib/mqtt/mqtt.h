#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "secrets.h"
#include "lock.h"

extern String topicPublish;
extern String topicSubscribe;

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



