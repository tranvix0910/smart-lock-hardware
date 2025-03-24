#include "lock.h"
#include "user_interface.h"

unsigned long lockOpenTime = 0;
unsigned long lastDoorCheckTime = 0;
unsigned long lockOpenWithoutDoorOpenTime = 0;
bool isLockOpen = false;
bool isDoorAlertActive = false;
bool isSystemLocked = false;
uint8_t failedAttempts = 0;
bool doorHasBeenOpened = false;

void lockInit() {
    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW);
    
    isLockOpen = false;
    isDoorAlertActive = false;
    isSystemLocked = false;
    failedAttempts = 0;
    doorHasBeenOpened = false;
}

bool isSystemLockedOut() {
    if (isSystemLocked) {
        displayResult("System Locked!", TFT_RED);
        isNormalMode = false;
        for (int i = 0; i < 3; i++) {
            alertTurnOn(); 
            delay(200);
            alertTurnOff();
            delay(200);
        }
        return true;
    }
    return false;
}

void unlockSystem() {
    displayResult("System Unlocked!", TFT_GREEN);
    isNormalMode = true;
    isSystemLocked = false;
    failedAttempts = 0;
    Serial.println("System unlocked via React app");
}

void incrementFailedAttempt() {
    failedAttempts++;
    Serial.printf("Failed attempt %d of %d\n", failedAttempts, MAX_FAILED_ATTEMPTS);
    
    if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
        isSystemLocked = true;
        Serial.println("System is completely locked! All functions disabled. Use React app to unlock.");
        
        displayResult("System Locked!", TFT_RED);
        digitalWrite(ALERT_PIN, LOW);
        isLockOpen = false;
        isDoorAlertActive = false;
    } else {
        alertTurnOn();
        delay(500);
        alertTurnOff();
    }
}

void resetFailedAttempts() {
    failedAttempts = 0;
    Serial.println("Failed attempts reset");
}

void lockOpen() {
    if (isSystemLockedOut()) {
        return;
    }
    
    digitalWrite(LOCK_PIN, HIGH);
    Serial.println("Lock opened");
    
    isLockOpen = true;
    isDoorAlertActive = false;
    doorHasBeenOpened = false;
    lockOpenWithoutDoorOpenTime = millis();
    resetFailedAttempts();
}

void lockClose() {
    if (isSystemLockedOut()) {
        return;
    }
    digitalWrite(LOCK_PIN, LOW);
    Serial.println("Lock closed");

    isLockOpen = false;
    isDoorAlertActive = false;
    doorHasBeenOpened = false;
}

void checkDoorStatus() {
    if (isSystemLockedOut()) {
        return;
    }
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
    if (isSystemLockedOut()) {
        return;
    }
    
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


