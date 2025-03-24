#include "magnetic_hall.h"
#include "alert.h"

void magneticHallInit() {
    pinMode(MAGNETIC_HALL_PIN, INPUT);
}

bool magneticHallCheck() {
    if (digitalRead(MAGNETIC_HALL_PIN) == LOW) {
        return false;
    } else {
        return true;
    }
}

