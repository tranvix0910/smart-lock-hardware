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
#include "esp_task_wdt.h"

// TaskHandle_t rfidTask = NULL;                                       
TaskHandle_t webSocketTask = NULL;
TaskHandle_t buttonTask = NULL;
TaskHandle_t rfidModeTask = NULL;
TaskHandle_t fingerprintModeTask = NULL;

<<<<<<< HEAD
bool isAddingCard = false;

=======
>>>>>>> 866cf2b8f8c32aa2c680b131271d449053364145
// void rfidTaskFunction(void *parameter) {
//     for(;;) {
//         rfidRead();
//         vTaskDelay(200 / portTICK_PERIOD_MS);
//     }
// }

void webSocketTaskFunction(void *parameter) {
    for(;;) {
        websocketHandle();
<<<<<<< HEAD
        vTaskDelay(10 / portTICK_PERIOD_MS);
=======
        vTaskDelay(100 / portTICK_PERIOD_MS);
>>>>>>> 866cf2b8f8c32aa2c680b131271d449053364145
    }
}

void buttonTaskFunction(void *parameter) {
    ButtonEvent evt;
    for(;;) {
        if(xQueueReceive(buttonEventQueue, &evt, portMAX_DELAY) == pdTRUE) {
            if(xSemaphoreTake(wsMutex, portMAX_DELAY) == pdTRUE) {
                buttonEvent(evt.handleImg, evt.displayRes, evt.displayCornerText);
                xSemaphoreGive(wsMutex);
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void rfidModeTaskFunction(void *parameter) {
    for(;;) {
<<<<<<< HEAD
        if (!isAddingCard) {
            checkRFIDMode(displayResult);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
=======
        checkRFIDMode(displayResult);
        vTaskDelay(10 / portTICK_PERIOD_MS);
>>>>>>> 866cf2b8f8c32aa2c680b131271d449053364145
    }
}

void fingerprintModeTaskFunction(void *parameter) {
    for(;;) {
        checkFingerprintMode(displayResult);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void smartLockSystemInit() {
    Serial.begin(115200);
    
    delay(TIME_DELAY);
    
    displayInit();
    wifiConfigInit();
    
    Serial.println("Initializing WiFi...");
    while (wifiMode != 1) {
        wifiConfigRun();
        delay(100);
    }
    
    Serial.println("WiFi connected successfully, initializing other modules...");
    
    Serial.println("Initializing basic hardware...");
    wifiAPSetup();
    websocketInit();
    
    Serial.println("Initializing cloud communication...");
    connectToAWSIoTCore();
    
    Serial.println("Initializing security sensors...");
    fingerprintInit();     
    rfidInit();
    motionDetectBegin();    
    magneticHallInit();     
    
    Serial.println("Initializing other components...");
    buttonInit();
    lockInit();
    alertInit();
    
    Serial.println("Creating RTOS tasks...");
    
    // xTaskCreate(
    //     rfidTaskFunction,    
    //     "RFID Task",         
    //     4096,                
    //     NULL,                
    //     1,
    //     &rfidTask            
    // );

    xTaskCreate(
        buttonTaskFunction,
        "Button Task",
        4096,
        NULL,
        4,
        &buttonTask
    );
<<<<<<< HEAD
=======
    
    xTaskCreatePinnedToCore(
        webSocketTaskFunction,
        "WebSocket Task",     
        16384,
        NULL,                  
        3,
        &webSocketTask,
        1  
    );
    
    xTaskCreatePinnedToCore(
        rfidModeTaskFunction,
        "RFID Mode Task",
        4096,
        NULL,
        tskIDLE_PRIORITY + 1,                    
        &rfidModeTask,
        0
    );
>>>>>>> 866cf2b8f8c32aa2c680b131271d449053364145

    xTaskCreate(
        fingerprintModeTaskFunction,
        "Fingerprint Mode Task",
        4096,
        NULL,
        2,
        &fingerprintModeTask
    );
    
    xTaskCreatePinnedToCore(
        webSocketTaskFunction,
        "WebSocket Task",     
        4096,
        NULL,                  
        4,
        &webSocketTask,
        1  
    );
    
    xTaskCreatePinnedToCore(
        rfidModeTaskFunction,
        "RFID Mode Task",
        4096,
        NULL,
        2,                    
        &rfidModeTask,
        0
    );
    
    Serial.println("Smart Lock System initialization complete!");
}

void smartLockSystemUpdate() {

    if (wifiMode != 1) {
        wifiConfigRun();
        return;
    }
    
    // Kiểm tra trạng thái khóa khẩn cấp trước khi làm bất cứ điều gì khác
    if (checkEmergencyLockStatus()) {
        return;
    }
    
    // - rfidRead() -> rfidTaskFunction
    // - websocketHandle() -> webSocketTaskFunction
    // - checkFingerprintMode() -> fingerprintModeTaskFunction
    // - checkRFIDMode() -> rfidModeTaskFunction

    static uint32_t lastStatusTime = 0;
    uint32_t currentTime = millis();
    
    if (currentTime - lastStatusTime > 30000) {
        Serial.println("System running normally. Memory free: " + String(ESP.getFreeHeap()) + " bytes");
        lastStatusTime = currentTime;
    }
    
    // Motion Detection
    displayCheckMotion();
    lockUpdate();
    buttonResetMode();
    clientLoop();
}