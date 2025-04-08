#include "api.h"

// Define global variables
String livenessCheckResult = "";
extern String userId;
extern String deviceId;
String faceId = "";
const char* user_name = "Undefined";

String convertToVietnamTime(String isoTimestamp) {
    // Trích xuất năm, tháng, ngày, giờ, phút, giây từ chuỗi timestamp
    int year = isoTimestamp.substring(0, 4).toInt();
    int month = isoTimestamp.substring(5, 7).toInt();
    int day = isoTimestamp.substring(8, 10).toInt();
    int hour = isoTimestamp.substring(11, 13).toInt();
    int minute = isoTimestamp.substring(14, 16).toInt();
    int second = isoTimestamp.substring(17, 19).toInt();

    // Cộng 7 giờ để chuyển sang giờ Việt Nam
    hour += 7;

    // Điều chỉnh ngày nếu vượt quá 24 giờ
    if (hour >= 24) {
        hour -= 24;
        day += 1;
        
        // Xử lý nếu ngày vượt quá số ngày của tháng
        int daysInMonth;
        switch (month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                daysInMonth = 31;
                break;
            case 4: case 6: case 9: case 11:
                daysInMonth = 30;
                break;
            case 2:
                // Kiểm tra năm nhuận
                daysInMonth = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
                break;
            default:
                daysInMonth = 30;  // Mặc định an toàn
        }

        if (day > daysInMonth) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }

    // Định dạng lại chuỗi ngày giờ
    char vietnamTime[25];
    sprintf(vietnamTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    return String(vietnamTime);
}

void parsingJSONResult(String response, uint16_t SCREEN_COLOR, String message, String livenessCheckResult, uint8_t failedAttempts) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.print("JSON Parsing failed: ");
        Serial.println(error.c_str());
        displayJSONParsingFailed();
        return;
    }

    user_name = doc["user_name"];
    double similarity = doc["similarity"];
    faceId = doc["faceId"].as<String>();
    String timestamp = doc["timestamp"];
    String vietnamTime = convertToVietnamTime(timestamp);
    displayJSONParsingResult(SCREEN_COLOR, message, livenessCheckResult, user_name, vietnamTime, similarity, failedAttempts);
}

void uploadImageToS3(WebsocketsMessage msg){

    String bucketName = "smart-door-system";
    String fileName = "image.jpeg";

    HTTPClient http;

    String apiUrl = "https://rxmieh048b.execute-api.ap-southeast-1.amazonaws.com/upload/" + bucketName + "/" + fileName;

    Serial.println("API URL: " + apiUrl);

    http.begin(apiUrl);
    http.addHeader("Content-Type", "image/jpeg");

    int httpResponseCode = http.POST((uint8_t*)msg.c_str(), msg.length());

    if(httpResponseCode == 200){
        String response = http.getString();
        Serial.println("Response: " + response);
        displayResult("Upload Success!", TFT_GREEN);
    } else {
        Serial.printf("HTTP Request Failed: %s\n", http.errorToString(httpResponseCode).c_str());
        displayResult("Upload Failed!", TFT_RED);
    }
    http.end();
}

void compareFace(WebsocketsMessage msg){

    String bucketName = "smart-door-system";
    String fileName = "image.jpeg";

    HTTPClient http;

    String rekognitionCollectionId = "smartlock-" + userId + "-" + deviceId;
    String apiUrl = "https://rxmieh048b.execute-api.ap-southeast-1.amazonaws.com/compare/" + bucketName + "/" + fileName + "?collection_id=" + rekognitionCollectionId;
    Serial.println("API URL: " + apiUrl);
    Serial.println("Rekognition Collection ID: " + rekognitionCollectionId);

    http.begin(apiUrl);
    http.addHeader("Content-Type", "image/jpeg");

    bool isLiveness = livenessCheck(msg);

    if(!isLiveness) {
        Serial.println("Liveness check failed");
        incrementFailedAttempt();
        displayJSONParsingResult(TFT_RED, livenessCheckResult, "Not found!", "N/A", "N/A", 0, failedAttempts);
    } else {
        Serial.println("Liveness check passed");
        int httpResponseCode = http.POST((uint8_t*)msg.c_str(), msg.length());
        String response = http.getString();

        DynamicJsonDocument doc(1024);  
        DeserializationError error = deserializeJson(doc, response);

        String successMessage = "Face comparison completed successfully";
        String errorMessage = "No matching face found";

        if (httpResponseCode == 200) {

            const char* message = doc["message"];

            if (strcmp(message, successMessage.c_str()) == 0) {
                Serial.println("Response: " + response);
                parsingJSONResult(response, TFT_GREEN, "Welcome!", livenessCheckResult, failedAttempts);
                resetFailedAttempts();
                lockOpen();
                Serial.println("Door opened after successful authentication");
                isNormalMode = false;
            } else if (strcmp(message, errorMessage.c_str()) == 0) {
                Serial.println("Response: " + response);
                parsingJSONResult(response, TFT_RED, "No matching face found!", livenessCheckResult, failedAttempts);
                incrementFailedAttempt();
            } else {
                Serial.println("Unknown response: " + String(message));
            }
        } else {
            Serial.printf("HTTP Request Failed: %s\n", http.errorToString(httpResponseCode).c_str());
            displayResult("Upload Failed!", TFT_RED);
        }
    }
    http.end();
}

bool livenessCheck(WebsocketsMessage msg){

    // String apiUrl = "http://10.252.9.240:8000/predict"; // School
    String apiUrl = "http://192.168.1.119:8000/predict"; // Home
    // String apiUrl = "http://172.16.10.134:8000/predict"; // Homies
    // String apiUrl = "http://192.168.43.56:8000/predict"; // Quynh

    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Content-Type", "image/jpeg");

    Serial.println("Liveness Check");
    int httpResponseCode = http.POST((uint8_t*)msg.c_str(), msg.length());
    String response = http.getString();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (httpResponseCode == 200) {
        if (!error) {
            const char* prediction = doc["prediction"];
            float real_prob = doc["real_prob"];
            float fake_prob = doc["fake_prob"];

            if (strcmp(prediction, "real") == 0) {
                livenessCheckResult = "Passed";
                return true;
            } else {
                livenessCheckResult = "Failed";
                return false;
            }

            Serial.printf("Prediction: %s\n", prediction);
            Serial.printf("Real Probability: %.2f%%\n", real_prob * 100);
            Serial.printf("Fake Probability: %.2f%%\n", fake_prob * 100);
        } else {
            Serial.println("JSON parsing failed");
            livenessCheckResult = "Error: JSON parsing failed";
            return false;
        }
    } else {
        livenessCheckResult = "Error: " + http.errorToString(httpResponseCode);
        Serial.printf("HTTP Request Failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();    
    return false;
}

bool authenticateFace(WebsocketsMessage msg) {
    String bucketName = "smart-door-system";
    String fileName = "image.jpeg";

    HTTPClient http;

    String rekognitionCollectionId = "smartlock-" + userId + "-" + deviceId;
    String apiUrl = "https://rxmieh048b.execute-api.ap-southeast-1.amazonaws.com/compare/" + bucketName + "/" + fileName + "?collection_id=" + rekognitionCollectionId;
    Serial.println("API URL: " + apiUrl);

    http.begin(apiUrl);
    http.addHeader("Content-Type", "image/jpeg");

    bool isLiveness = livenessCheck(msg);

    if(!isLiveness) {
        Serial.println("Liveness check failed");
        incrementFailedAttempt();
        displayJSONParsingResult(TFT_RED, livenessCheckResult, "Not found!", "N/A", "N/A", 0, failedAttempts);
        return false;
    } else {
        Serial.println("Liveness check passed");
        int httpResponseCode = http.POST((uint8_t*)msg.c_str(), msg.length());
        String response = http.getString();

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);

        String successMessage = "Face comparison completed successfully";
        String errorMessage = "No matching face found";

        if (httpResponseCode == 200) {
            const char* message = doc["message"];

            if (strcmp(message, successMessage.c_str()) == 0) {
                Serial.println("Response: " + response);
                if (doc.containsKey("faceId")) {
                    faceId = doc["faceId"].as<String>();
                    Serial.println("Face ID extracted: " + faceId);
                } else {
                    Serial.println("Warning: Response doesn't contain faceId");
                    faceId = "unknown";
                }
                
                parsingJSONResult(response, TFT_GREEN, "Authentication Success!", livenessCheckResult, failedAttempts);
                resetFailedAttempts();
                Serial.println("Face authentication successful");
                
                return true;
            } else if (strcmp(message, errorMessage.c_str()) == 0) {
                Serial.println("Response: " + response);
                faceId = ""; // Clear faceId on failed authentication
                parsingJSONResult(response, TFT_RED, "No matching face found!", livenessCheckResult, failedAttempts);
                incrementFailedAttempt();
                return false;
            } else {
                Serial.println("Unknown response: " + String(message));
                faceId = ""; // Clear faceId on unknown response
                return false;
            }
        } else {
            Serial.printf("HTTP Request Failed: %s\n", http.errorToString(httpResponseCode).c_str());
            faceId = ""; // Clear faceId on HTTP error
            displayResult("Authentication Failed!", TFT_RED);
            return false;
        }
    }
    http.end();
    return false;
}