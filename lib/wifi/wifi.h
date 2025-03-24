#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>

extern WiFiMulti wifiMulti;
extern const char* ssid_ap;
extern const char* password_ap;

bool wifi_connect();
void wifi_ap_setup();

#endif
