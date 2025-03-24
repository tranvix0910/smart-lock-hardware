#include "smart_lock_system.h"
#include "motion_detect.h"
#include "fingerprint.h"
#include "rfid.h"
#include "magnetic_hall.h"
#include "button.h"
#include "user_interface.h"
#include "wifi.h"
#include "lock.h"
#include "alert.h"

void smartLockSystemInit() {

    Serial.begin(115200);
    delay(TIME_DELAY);

    // Initialize display
    displayInit();

    // Initialize WiFi AP
    wifi_ap_setup();

    // Initialize websocket
    websocketInit();

    // Initialize WiFi
    wifi_connect();

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

    // Streaming Camera
    websocketHandle();

    // Motion Detection
    displayCheckMotion();

    // Lock
    lockUpdate();

    // Check and process fingerprint if in scan mode
    checkFingerprintMode(displayResult);
}