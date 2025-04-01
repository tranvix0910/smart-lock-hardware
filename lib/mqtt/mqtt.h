#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "lock.h"

void createSubscribeTopic();
void connectToAWSIoTCore();
void handleMessage(char* topic, byte* payload, unsigned int length);
void clientLoop();
void processMessageByMode(const char* mode, const char* deviceId, const char* userId, const char* lockState);
void processChangeState(const char* deviceId, const char* userId, const char* lockState);
void reconnect();
void publishMessage(const char* topic, const char* message);
void messageLock(String lockState);

#endif



