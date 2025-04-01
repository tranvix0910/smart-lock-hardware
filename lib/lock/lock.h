#ifndef LOCK_H
#define LOCK_H

#include "common.h"

#define LOCK_TIMEOUT 30000    
#define DOOR_CHECK_TIME 1000  
#define MAX_DOOR_OPEN_TIME 35000
#define MAX_FAILED_ATTEMPTS 5     
#define UNUSED_OPEN_TIMEOUT 10000

extern unsigned long lockOpenTime;
extern unsigned long lastDoorCheckTime;
extern unsigned long lockOpenWithoutDoorOpenTime;
extern bool isDoorAlertActive;
extern bool isSystemLocked;
extern uint8_t failedAttempts;
extern bool doorHasBeenOpened;

void lockInit();
void lockOpen();
void lockClose();
void lockUpdate();
void checkDoorStatus();
void incrementFailedAttempt();
void resetFailedAttempts();
bool isSystemLockedOut();
void unlockSystem();

#endif
