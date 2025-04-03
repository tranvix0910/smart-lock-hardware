#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include "mqtt.h"

#define EEPROM_SIZE 200

extern WebServer webServer;
extern String ssid;
extern String password;
extern String deviceId;
extern String macAddress;
extern String secretKey;
extern String userId;
extern int wifiMode;

void WiFiEvent(WiFiEvent_t event);
void createAccessPoint();
void setupWifi();
void setupWebServer();
void wifiConfigInit();
void wifiConfigRun();
bool checkWiFiStatus();
void wifiAPSetup();
void generateDeviceInfo();
void getDeviceInfoValues(const char* &deviceId, const char* &macAddress, const char* &userId);
void saveUserId(String id);
void clearAllData();
#endif 