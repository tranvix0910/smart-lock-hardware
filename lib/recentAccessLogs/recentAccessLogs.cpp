#include "recentAccessLogs.h"

void publishRecentAccessLogs(String method, String status, String userName, String notes) {
    Serial.println("Publishing recent access logs");

    const char* deviceIdPtr;
    const char* userIdPtr;
    
    getDeviceInfoValues(deviceIdPtr, userIdPtr);
    
    String deviceId = String(deviceIdPtr);
    String userId = String(userIdPtr);

    StaticJsonDocument<200> doc;
    doc["deviceId"] = deviceId;
    doc["userId"] = userId; 
    doc["userName"] = userName;
    doc["method"] = method;
    doc["status"] = status;
    doc["notes"] = notes;

    String jsonString;
    serializeJson(doc, jsonString);

    publishMessage(topicRecentAccessPublish.c_str(), jsonString.c_str());
}
