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

TaskHandle_t rfidTask = NULL;
TaskHandle_t webSocketTask = NULL;
TaskHandle_t buttonTask = NULL;
TaskHandle_t rfidModeTask = NULL;
TaskHandle_t fingerprintModeTask = NULL;

// Hàm xử lý task RFID
// void rfidTaskFunction(void *parameter) {
//     for(;;) {
//         rfidRead();
//         // Delay 100ms
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }

void webSocketTaskFunction(void *parameter) {
    for(;;) {
        websocketHandle();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void buttonTaskFunction(void *parameter) {
    ButtonEvent evt;
    for(;;) {
        if(xQueueReceive(buttonEventQueue, &evt, portMAX_DELAY) == pdTRUE) {
            if(xSemaphoreTake(wsMutex, portMAX_DELAY) == pdTRUE) {
                buttonEvent(evt.handleImg, evt.displayRes);
                xSemaphoreGive(wsMutex);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Hàm xử lý task RFID Mode
// void rfidModeTaskFunction(void *parameter) {
//     for(;;) {
//         checkRFIDMode(displayResult);
//         // Delay 100ms
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }

void fingerprintModeTaskFunction(void *parameter) {
    for(;;) {
        checkFingerprintMode(displayResult);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void smartLockSystemInit() {
    Serial.begin(115200);
    delay(TIME_DELAY);
    
    Serial.println("Initializing WiFi...");
    wifiConfigInit();
    while (wifiMode != 1) {
        wifiConfigRun();
        delay(100);
    }
    
    Serial.println("WiFi connected successfully, initializing other modules...");
    
    Serial.println("Initializing basic hardware...");
    displayInit();
    wifiAPSetup();
    websocketInit();
    
    Serial.println("Initializing cloud communication...");
    connectToAWSIoTCore();
    
    Serial.println("Initializing security sensors...");
    fingerprintInit();     
    // rfidInit();             // RFID
    motionDetectBegin();    
    magneticHallInit();     
    
    Serial.println("Initializing other components...");
    buttonInit();
    lockInit();
    alertInit();
    
    Serial.println("Creating RTOS tasks...");
    
    // Tạo task cho RFID đọc thẻ (ưu tiên thấp)
    // xTaskCreate(
    //     rfidTaskFunction,    
    //     "RFID Task",         
    //     4096,                
    //     NULL,                
    //     1,                   // Mức ưu tiên thấp nhất
    //     &rfidTask            
    // );

    xTaskCreate(
        webSocketTaskFunction,
        "WebSocket Task",     
        8192,
        NULL,                  
        2,
        &webSocketTask        
    );

    xTaskCreate(
        buttonTaskFunction,
        "Button Task",
        4096,
        NULL,
        3,
        &buttonTask
    );
    
    // Tạo task cho RFID Mode (nhận diện thẻ để mở khóa)
    // xTaskCreate(
    //     rfidModeTaskFunction,
    //     "RFID Mode Task",
    //     4096,
    //     NULL,
    //     2,                    // Ưu tiên trung bình
    //     &rfidModeTask
    // );
    

    xTaskCreate(
        fingerprintModeTaskFunction,
        "Fingerprint Mode Task",
        4096,
        NULL,
        2,
        &fingerprintModeTask
    );
    
    Serial.println("Smart Lock System initialization complete!");
}

void smartLockSystemUpdate() {
    if (wifiMode != 1) {
        wifiConfigRun();
        return;
    }
    
    // - rfidRead() -> rfidTaskFunction
    // - websocketHandle() -> webSocketTaskFunction
    // - checkFingerprintMode() -> fingerprintModeTaskFunction
    // - checkRFIDMode() -> rfidModeTaskFunction
    
    // Motion Detection
    displayCheckMotion();
    lockUpdate();
    buttonResetMode();
    clientLoop();
}