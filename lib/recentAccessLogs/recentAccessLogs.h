#ifndef RECENT_ACCESS_LOGS_H
#define RECENT_ACCESS_LOGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

extern void publishMessage(const char* topic, const char* message);
extern void getDeviceInfoValues(const char* &deviceIdPtr, const char* &userIdPtr);

extern String topicRecentAccessPublish;
extern String topicRecentAccessSubscribe;

void publishRecentAccessLogs(String method, String status, String userName, String livenessCheckResult);

#endif
