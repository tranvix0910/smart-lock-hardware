#include "wifi.h"

WiFiMulti wifiMulti;
const char* ssid_ap = "Tran Vix";
const char* password_ap = "09102004";

bool wifi_connect() {
    wifiMulti.addAP("PTIT.HCM_CanBo", "");
    wifiMulti.addAP("PTIT.HCM_SV", "");
    wifiMulti.addAP("Thai Bao", "0869334749");
    if (wifiMulti.run() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
        Serial.print("Connected to: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("Failed to connect to any WiFi network.");
        return false;
    }
}

void wifi_ap_setup() {
    WiFi.softAP(ssid_ap, password_ap);
    Serial.println("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
}


