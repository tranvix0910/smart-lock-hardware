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

// Khai báo task handles
TaskHandle_t rfidTask = NULL;
TaskHandle_t webSocketTask = NULL;

// Hàm xử lý task RFID
void rfidTaskFunction(void *parameter) {
    for(;;) {
        rfidRead();
        // Delay 100ms
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Hàm xử lý task WebSocket
void webSocketTaskFunction(void *parameter) {
    for(;;) {
        websocketHandle();
        // Delay 50ms
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

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
    rfidInit();
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
    
    // Tạo task cho RFID
    xTaskCreate(
        rfidTaskFunction,    // Hàm task
        "RFID Task",         // Tên task
        4096,                // Kích thước stack
        NULL,                // Tham số
        1,                   // Mức ưu tiên (1 là thấp nhất)
        &rfidTask            // Task handle
    );
    
    // Tạo task cho WebSocket
    xTaskCreate(
        webSocketTaskFunction, // Hàm task
        "WebSocket Task",      // Tên task
        8192,                  // Kích thước stack lớn hơn vì xử lý hình ảnh
        NULL,                  // Tham số
        2,                     // Mức ưu tiên cao hơn RFID
        &webSocketTask         // Task handle
    );
}

void smartLockSystemUpdate() {
    if (wifiMode != 1) {
        wifiConfigRun();
        return;
    }
    
    // Không cần gọi rfidRead() và websocketHandle() ở đây nữa
    // vì đã được xử lý trong các task riêng
    
    // Motion Detection
    displayCheckMotion();
    // Lock
    lockUpdate();
    // Check and process fingerprint if in scan mode
    checkFingerprintMode(displayResult);
    // check button reset mode
    buttonResetMode();
    // MQTT
    clientLoop();
}