#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <Arduino.h>

// Motion detect
#define MOTION_DETECT_PIN 14

void motionDetectBegin();
bool motionDetectCheck();

#endif
