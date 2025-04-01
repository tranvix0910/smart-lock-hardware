#include "smart_lock_system.h"
#include "wifi_config.h"
#include "user_interface.h"
#include "fingerprint.h"
#include "rfid.h"
#include "motion_detect.h"
#include "magnetic_hall.h"
#include "button.h"
#include "mqtt.h"
#include "alert.h"

void smartLockSystemInit() {
    Serial.begin(115200);
    delay(TIME_DELAY);

    wifiConfigInit();
    
    while (wifiMode != 1) {
        wifiConfigRun();
        delay(100);
    }
    
    Serial.println("WiFi connected successfully, initializing other modules...");

    // Initialize display
    displayInit();

    // Initialize WiFi AP
    wifiAPSetup();

    // Initialize websocket
    websocketInit();

    // Initialize MQTT
    connectToAWSIoTCore();

    // Initialize fingerprint
    fingerprintInit();

    // Initialize RFID
    // rfidInit();
    
    // Initialize motion detect
    motionDetectBegin();
    
    // Initialize magnetic hall
    magneticHallInit();

    // Initialize button capture image
    buttonInit();

    // Initialize lock
    lockInit();

    // Initialize buzzer
    alertInit();
}

void smartLockSystemUpdate() {

    if (wifiMode != 1) {
        wifiConfigRun();
        return;
    }

    websocketHandle();

    // Motion Detection
    displayCheckMotion();

    // Lock
    lockUpdate();

    // Check and process fingerprint if in scan mode
    checkFingerprintMode(displayResult);

    // MQTT
    clientLoop();
}