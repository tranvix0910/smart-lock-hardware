#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>

extern WebServer webServer;

extern String ssid;
extern String password;
extern int wifiMode; // 0: Chế độ cấu hình, 1: Chế độ kết nối, 2: Mất wifi

// Khai báo các hàm
void WiFiEvent(WiFiEvent_t event);
void setupWifi();
void createAccessPoint();
void setupWebServer();
void wifiConfigInit();
void wifiConfigRun();
bool checkWiFiStatus();
void wifiAPSetup();
#endif 