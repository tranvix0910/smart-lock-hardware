#include "alert.h"

void alertInit() {
    pinMode(ALERT_PIN, OUTPUT);
}

void alertTurnOn() {
    digitalWrite(ALERT_PIN, HIGH);
    Serial.println("Alert turned on");
}

void alertTurnOff() {
    digitalWrite(ALERT_PIN, LOW);
    Serial.println("Alert turned off");
}

