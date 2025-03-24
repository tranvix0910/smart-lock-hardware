#include "motion_detect.h"

void motionDetectBegin() {
    pinMode(MOTION_DETECT_PIN, INPUT);
}

bool motionDetectCheck() {
    if (digitalRead(MOTION_DETECT_PIN) == HIGH) {
        return true;
    } else {
        return false;
    }
}