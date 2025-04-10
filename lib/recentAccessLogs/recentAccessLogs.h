#ifndef RECENT_ACCESS_LOGS_H
#define RECENT_ACCESS_LOGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <mqtt.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "wifi_config.h"

extern String topicRecentAccessPublish;
extern String topicRecentAccessSubscribe;

void publishRecentAccessLogs(String method, String status, String userName, String livenessCheckResult);

#endif
