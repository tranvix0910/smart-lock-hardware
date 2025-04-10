#include "user_interface.h"
#include "mqtt.h"
#include "rfid.h"

SemaphoreHandle_t wsMutex = NULL;
QueueHandle_t buttonEventQueue = NULL;

TFT_eSPI tft = TFT_eSPI();

WebsocketsServer WebSocketServer;
WebsocketsClient WebSocketClient;

static unsigned long lastMotionTime = 0;

void websocketInit() {
    wsMutex = xSemaphoreCreateMutex();
    buttonEventQueue = xQueueCreate(5, sizeof(ButtonEvent));
    WebSocketServer.listen(8888);
    Serial.println("WebSocket server started on port 8888");
}

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
    if ( y >= tft.height() ) return 0;
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

void displaySetBrightness(uint8_t brightness) {
    analogWrite(BACKLIGHT_PIN, brightness);
}

void displayTurnOn() {
    displaySetBrightness(BACKLIGHT_ON);
    lastMotionTime = millis();
}

void displayTurnOff() {
    displaySetBrightness(BACKLIGHT_OFF);
}

void displayCheckMotion() {
    if (motionDetectCheck()) {
        displayTurnOn();
    } else {
        if (millis() - lastMotionTime > SCREEN_TIMEOUT) {
            displayTurnOff();
        }
    }
}

void displayResult(String message, uint16_t color) {
    tft.setRotation(0);
    tft.fillScreen(color);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, color);
    tft.drawString(message, 120, 120, GFXFF);
}

void displayInit() {
    tft.begin();
    tft.setRotation(0);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillScreen(TFT_SKYBLUE);
    tft.setSwapBytes(true);
    tft.setFreeFont(FSB9);
    
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tftOutput);

    tft.setTextDatum(TC_DATUM);
    tft.drawString("Smart Door System", 120, 80, GFXFF);
    tft.drawString("Initializing...", 120, 120, GFXFF);
    delay(2000);
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
}

void showingImage() {
    tft.setRotation(1);
    WebsocketsMessage msg = WebSocketClient.readBlocking();
    TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());
}

bool faceAuthentication() {
    Serial.println("Face Authentication Start");
    WebsocketsMessage msg = WebSocketClient.readBlocking();
    tft.setRotation(0);
    TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());
    return authenticateFace(msg);
}

void handleImage() {
    WebsocketsMessage msg = WebSocketClient.readBlocking();
    TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());
    uploadImageToS3(msg);
    compareFace(msg);
}

void displayCornerText(String message, uint16_t color, uint8_t fontSize) {
    tft.setRotation(0);
    tft.setTextSize(fontSize);
    tft.setTextColor(color);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(message, 5, 5, GFXFF);
}

void websocketHandle() {
    if(xSemaphoreTake(wsMutex, portMAX_DELAY) == pdTRUE) {
        if(WebSocketServer.poll()) {
            WebSocketClient = WebSocketServer.accept();
            Serial.println("Client connected");
        }
        if(WebSocketClient.available()) {
            WebSocketClient.poll();
            ButtonEvent evt = {handleImage, displayResult, displayCornerText};
            xQueueSend(buttonEventQueue, &evt, 0);
            if(isNormalMode) {
                showingImage();
            }
        }
        xSemaphoreGive(wsMutex);
    }
}

void displayJSONParsingFailed() {
    tft.fillScreen(TFT_RED);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("JSON Parsing Error!", 160, 120, GFXFF);
}

void displayJSONParsingResult(uint16_t color, String message, String livenessCheckResult, String user_name, String timestamp, double similarity, uint8_t failedAttempts) {
    tft.setRotation(0);
    tft.fillScreen(color);

    int centerX = 120;  
    int startY = 80;    

    tft.setTextDatum(TC_DATUM); 
    tft.setTextColor(TFT_BLACK, color);

    tft.drawString(message, centerX, startY, GFXFF);
    tft.drawString("Liveness Check: " + livenessCheckResult, centerX, startY + 30, GFXFF);
    tft.drawString("User Name: " + user_name, centerX, startY + 60, GFXFF);
    tft.drawString("Time: " + timestamp, centerX, startY + 90, GFXFF);
    tft.drawString("Similarity: " + String(similarity) + "%", centerX, startY + 120, GFXFF);
    tft.drawString("Failed Attempts: " + String(failedAttempts), centerX, startY + 150, GFXFF);
}


