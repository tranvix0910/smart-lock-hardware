#ifndef ALERT_H
#define ALERT_H

#include <Arduino.h>

#define ALERT_PIN 33

void alertInit();
void alertTurnOn();
void alertTurnOff();
void alertBeep(int duration = 200);

#endif

