#include "lock.h"
#include "mqtt.h"
#include "freertos/FreeRTOS.h"
#include "alert.h"
#include "user_interface.h"
#include "eeprom_manager.h"

extern TaskHandle_t webSocketTask;
extern TaskHandle_t buttonTask;
extern TaskHandle_t rfidModeTask;
extern TaskHandle_t fingerprintModeTask;

bool isEmergencyLocked = false;

unsigned long emergencyLockStartTime = 0;
const unsigned long EMERGENCY_LOCK_DURATION = 20000;

unsigned long lockOpenTime = 0;
unsigned long lastDoorCheckTime = 0;
unsigned long lockOpenWithoutDoorOpenTime = 0;
bool isLockOpen = false;
bool isDoorAlertActive = false;
bool isSystemLocked = false;
uint8_t failedAttempts = 0;
bool doorHasBeenOpened = false;

extern String topicUnlockSystemPublish;
extern String deviceId;
extern String userId;

extern void displayUnlockScreen();
extern void displayEmergencyLockScreen();
extern void publishMessage(const char* topic, const char* message);

void lockInit() {
    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW);
    
    isLockOpen = false;
    isDoorAlertActive = false;
    isSystemLocked = false;
    failedAttempts = 0;
    doorHasBeenOpened = false;

    uint8_t lockStatus = EEPROM.read(EMERGENCY_LOCK_STATUS_ADDR);
    isEmergencyLocked = (lockStatus == EMERGENCY_LOCK_VALUE);
    
    if (isEmergencyLocked) {
        Serial.println("System was in emergency lock state before restart");
        emergencyLockStartTime = millis();
        displayEmergencyLockScreen();
    }
    
    Serial.printf("Emergency lock status read from EEPROM: %d (value: 0x%02X)\n", isEmergencyLocked, lockStatus);
}

void applyEmergencyLockState() {

    static bool applied = false;
    
    if (!applied && isEmergencyLocked) {

        applied = true;

        Serial.println("Applying emergency lock state - suspending tasks");
        
        if (webSocketTask != NULL) vTaskSuspend(webSocketTask);
        if (buttonTask != NULL) vTaskSuspend(buttonTask);
        if (rfidModeTask != NULL) vTaskSuspend(rfidModeTask);
        if (fingerprintModeTask != NULL) vTaskSuspend(fingerprintModeTask);
    }
}

void saveEmergencyLockStatus() {
    EEPROMManager::saveEmergencyLockStatus(true);
}

void clearEmergencyLockStatus() {
    EEPROMManager::clearEmergencyLockStatus(true);
}

void unlockSystem() {

    alertTurnOff();
    
    if (isEmergencyLocked) {
        if (webSocketTask != NULL) vTaskResume(webSocketTask);
        if (buttonTask != NULL) vTaskResume(buttonTask);
        if (rfidModeTask != NULL) vTaskResume(rfidModeTask);
        if (fingerprintModeTask != NULL) vTaskResume(fingerprintModeTask);
        clearEmergencyLockStatus();
    }
    
    isEmergencyLocked = false;
    emergencyLockStartTime = 0;
    isSystemLocked = false;
    failedAttempts = 0;
    
    displayUnlockScreen();
    
    isNormalMode = true;
    Serial.println("System unlocked via app");
}

void emergencyLockSystem() {

    Serial.println("Emergency Lock System");
    
    if (isEmergencyLocked) {
        return;
    }
    
    isEmergencyLocked = true;
    emergencyLockStartTime = millis();
    
    Serial.println("EMERGENCY SYSTEM LOCK ACTIVATED!");
    Serial.println("Too many failed attempts detected (5)");
    Serial.println("System will be locked until unlocked via app");

    saveEmergencyLockStatus();

    alertTurnOff();

    StaticJsonDocument<200> responseDoc;
    responseDoc["mode"] = "EMERGENCY LOCK SYSTEM";
    responseDoc["deviceId"] = deviceId;
    responseDoc["userId"] = userId;

    String responseJson;
    serializeJson(responseDoc, responseJson);
    publishMessage(topicUnlockSystemPublish.c_str(), responseJson.c_str());
}

void incrementFailedAttempt() {
    failedAttempts++;
    Serial.printf("Failed attempt %d of %d\n", failedAttempts, MAX_FAILED_ATTEMPTS);
    
    if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
        emergencyLockSystem();
    } else {
        alertTurnOn();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        alertTurnOff();
    }
}

void resetFailedAttempts() {
    failedAttempts = 0;
    Serial.println("Failed attempts reset");
}

void lockOpen() {
    digitalWrite(LOCK_PIN, HIGH);
    Serial.println("Lock opened");
    messageLock("UNLOCK");
    isLockOpen = true;
    isDoorAlertActive = false;
    doorHasBeenOpened = false;
    lockOpenWithoutDoorOpenTime = millis();
    resetFailedAttempts();
}

void lockClose() {
    digitalWrite(LOCK_PIN, LOW);
    Serial.println("Lock closed");
    messageLock("LOCK");
    isLockOpen = false;
    isDoorAlertActive = false;
    doorHasBeenOpened = false;
}

void checkDoorStatus() {
    if (millis() - lastDoorCheckTime >= DOOR_CHECK_TIME) {
        lastDoorCheckTime = millis();
        if (magneticHallCheck() && (millis() - lockOpenTime >= MAX_DOOR_OPEN_TIME)) {
            if (!isDoorAlertActive) {
                Serial.println("Warning: Door has been open too long!");
                isDoorAlertActive = true;
                alertTurnOn();
            }
        } else if (!magneticHallCheck() && isDoorAlertActive) {
            isDoorAlertActive = false;
            alertTurnOff();
            Serial.println("Alert deactivated: Door is now closed");
        }
    } 
}

void lockUpdate() {
    if (isLockOpen && !magneticHallCheck() && !doorHasBeenOpened && 
        (millis() - lockOpenWithoutDoorOpenTime >= UNUSED_OPEN_TIMEOUT)) {
        Serial.println("Lock closed: Door wasn't opened within 10 seconds");
        isNormalMode = true;
        lockClose();
        return;
    }
    
    if (isLockOpen && magneticHallCheck() && !doorHasBeenOpened) {
        lockOpenTime = millis();
        doorHasBeenOpened = true;
        Serial.println("Door is now open, starting timer");
    }
    
    checkDoorStatus();
    
    if (isLockOpen) {
        if (!magneticHallCheck() && doorHasBeenOpened) {
            Serial.println("Door closed by user, locking immediately");
            isNormalMode = true;
            lockClose();
        } else if (doorHasBeenOpened && (millis() - lockOpenTime >= LOCK_TIMEOUT)) {
            Serial.println("Lock timeout reached, closing lock");
            isNormalMode = true;
        }
    }
}

bool checkEmergencyLockStatus() {

    if (!isEmergencyLocked) {
        return false;
    }

    applyEmergencyLockState();
    
    static bool alertActivated = false;
    if (!alertActivated) {
        alertTurnOn();
        alertActivated = true;
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - emergencyLockStartTime;
    bool isAlertTimeOver = elapsedTime >= EMERGENCY_LOCK_DURATION;
    
    if (isAlertTimeOver && elapsedTime < EMERGENCY_LOCK_DURATION + 100) {
        Serial.println("Alert stopped after timeout, system remains locked");
        alertTurnOff();
        displayEmergencyLockScreen();
    }
    
    static unsigned long lastTimeUpdate = 0;
    if (!isAlertTimeOver && currentTime - lastTimeUpdate > 1000) {
        lastTimeUpdate = currentTime;
        
        tft.setRotation(0);
        
        unsigned long remainingTime = EMERGENCY_LOCK_DURATION - elapsedTime;
        int remainingSeconds = remainingTime / 1000;
        
        int screenWidth = tft.width();
        int screenHeight = tft.height();
        int centerX = screenWidth / 2;
        
        int timerY = screenHeight * 4 / 5;
        
        int barWidth = screenWidth * 0.75;
        int barHeight = screenHeight * 0.04;
        int barX = centerX - barWidth / 2;
        int barY = timerY;
        
        tft.fillRect(barX - 10, barY - barHeight * 2, barWidth + 20, barHeight * 4, TFT_RED);
        
        for (int i = 0; i < 3; i++) {
            uint16_t borderColor = tft.color565(220 - i*30, 220 - i*30, 220 - i*30);
            tft.drawRoundRect(barX - i, barY - i, barWidth + i*2, barHeight + i*2, barHeight/2, borderColor);
        }
        
        for (int i = 0; i < barHeight; i++) {
            uint16_t bgColor = tft.color565(60 + i, 60 + i, 70 + i);
            tft.drawRoundRect(barX, barY, barWidth, barHeight, barHeight/2, bgColor);
        }
        
        int progressWidth = map(remainingSeconds, EMERGENCY_LOCK_DURATION/1000, 0, 0, barWidth - 4);
        
        if (progressWidth > 0) {
            for (int i = 0; i < barHeight - 4; i++) {
                uint16_t fillColor = tft.color565(255, 200 - i*5, 0 + i*5);
                int fillY = barY + 2 + i;
                int radius = (barHeight - 4) / 2;
                
                if (progressWidth > radius * 2) {
                    tft.drawFastHLine(barX + 2 + radius, fillY, progressWidth - radius * 2, fillColor);
                    
                    tft.drawPixel(barX + 2 + progressWidth - radius, fillY, fillColor);
                    tft.drawPixel(barX + 2 + radius - 1, fillY, fillColor);
                } else if (progressWidth > 0) {
                    tft.drawFastHLine(barX + 2, fillY, progressWidth, fillColor);
                }
            }
        }
        
        int textY = barY - barHeight;
        
        tft.fillRoundRect(centerX - 50, textY - 15, 100, 30, 5, TFT_BLACK);
        tft.fillRoundRect(centerX - 48, textY - 13, 96, 26, 4, TFT_RED);
        
        char timeStr[20];
        sprintf(timeStr, "%d SEC", remainingSeconds);
        
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK);
        tft.drawString(timeStr, centerX + 1, textY + 1, 4);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(timeStr, centerX, textY, 4);
    }
    
    return true;
}
