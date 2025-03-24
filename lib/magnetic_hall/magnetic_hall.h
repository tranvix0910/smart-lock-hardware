#ifndef MAGNETIC_HALL_H
#define MAGNETIC_HALL_H

#include <Arduino.h>

#define MAGNETIC_HALL_PIN 25

void magneticHallInit();
bool magneticHallCheck();

#endif
