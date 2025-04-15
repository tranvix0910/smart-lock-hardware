#include "alert.h"
#include "freertos/FreeRTOS.h"
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

void alertBeep(int duration) {
    digitalWrite(ALERT_PIN, HIGH);
    vTaskDelay(duration / portTICK_PERIOD_MS);
    digitalWrite(ALERT_PIN, LOW);
    Serial.println("Alert beep completed");
}

