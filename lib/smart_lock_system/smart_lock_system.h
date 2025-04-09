#ifndef SMART_DOOR_SYSTEM_H
#define SMART_DOOR_SYSTEM_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TIME_DELAY 1000

extern TaskHandle_t rfidTask;
extern TaskHandle_t webSocketTask;
extern TaskHandle_t buttonTask;

void smartLockSystemInit();
void smartLockSystemUpdate();

#endif
