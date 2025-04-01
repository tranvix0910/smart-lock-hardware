#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_BLUE 0x001F
#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000

#define ALERT_PIN 33
#define MAGNETIC_HALL_PIN 25
#define LOCK_PIN 32

extern bool isLockOpen;
extern bool isNormalMode;

void displayResult(String message, uint16_t color);
void alertTurnOn();
void alertTurnOff();
bool magneticHallCheck();

#endif 