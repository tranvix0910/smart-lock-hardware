#include "user_interface.h"

SemaphoreHandle_t wsMutex = NULL;
QueueHandle_t buttonEventQueue = NULL;

TFT_eSPI tft = TFT_eSPI();

WebsocketsServer WebSocketServer;
WebsocketsClient WebSocketClient;

bool isNormalMode = true;

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
    displayTurnOn();
    
    tft.setRotation(0);
    tft.setSwapBytes(true);

    uint8_t primaryR = 36, primaryG = 48, primaryB = 63;
    
    for (int i = 0; i < 10; i++) {
        uint8_t r = i * primaryR / 10;
        uint8_t g = i * primaryG / 10;
        uint8_t b = i * primaryB / 10;
        tft.fillScreen(tft.color565(r, g, b));
        delay(80);
    }
    
    int width = tft.width();
    int height = tft.height();
    int centerX = width / 2;
    int centerY = height / 2;
   
    uint8_t accentR = 235, accentG = 244, accentB = 93;
    
    for (int y = 0; y < height; y++) {
        float factor = y / (float)height;
        uint8_t r = primaryR + (factor * 25);
        uint8_t g = primaryG + (factor * 20);
        uint8_t b = primaryB + (factor * 30);
        tft.drawFastHLine(0, y, width, tft.color565(r, g, b));
    }
    
    for (int i = 0; i < 3; i++) {
        tft.drawRoundRect(
            5 + i, 5 + i, 
            width - 10 - i*2, height - 10 - i*2, 
            10, 
            tft.color565(accentR - i*30, accentG - i*30, accentB)
        );
        delay(50);
    }
    
    for (int r = 80; r > 0; r -= 4) {
        uint8_t alpha = map(r, 80, 0, 20, 150);
        uint16_t color = tft.color565(
            accentR * alpha / 255, 
            accentG * alpha / 255, 
            accentB * alpha / 255
        );
        tft.drawCircle(centerX, height / 3, r, color);
        if (r % 10 == 0) delay(30);
    }
    
    int logoY = height / 3;
    int lockSize = width / 3;
    
    uint8_t blueR = 37, blueG = 99, blueB = 235;
    
    tft.fillRoundRect(
        centerX - lockSize/2 + 5, 
        logoY + lockSize/3 + 5, 
        lockSize, 
        lockSize, 
        15, 
        tft.color565(20, 30, 40)
    );
    
    for (int i = 8; i >= 0; i--) {
        uint8_t shade = i * 18;
        tft.fillRoundRect(
            centerX - lockSize/2 + i/2, 
            logoY + lockSize/3 + i/2, 
            lockSize - i, 
            lockSize - i, 
            15, 
            tft.color565(blueR - shade/2, blueG - shade/2, blueB - shade)
        );
        delay(40);
    }
    
    tft.drawRoundRect(
        centerX - lockSize/2, 
        logoY + lockSize/3, 
        lockSize, 
        lockSize, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );

    tft.drawRoundRect(
        centerX - lockSize/2 + 1, 
        logoY + lockSize/3 + 1, 
        lockSize - 2, 
        lockSize - 2, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );
    
    for (int i = 0; i <= 10; i++) {
        float factor = i / 10.0;
        int radius = (lockSize/2) * factor;
        if (radius < 3) continue;
        tft.fillCircle(centerX, logoY, radius + 2, tft.color565(primaryR + 10, primaryG + 10, primaryB + 15));
        tft.fillCircle(centerX, logoY, radius, tft.color565(blueR + 20, blueG + 20, blueB));
        tft.drawCircle(centerX, logoY, radius, tft.color565(accentR, accentG, accentB));
        delay(50);
    }
    
    tft.fillCircle(centerX - lockSize/6, logoY - lockSize/8, lockSize/12, 
                  tft.color565(220, 230, 255));
    int keyhole_x = centerX;
    int keyhole_y = logoY + lockSize/2 + lockSize/8;
    tft.fillCircle(keyhole_x + 1, keyhole_y + 1, lockSize/10, TFT_BLACK);
    tft.fillRect(keyhole_x - lockSize/20 + 1, keyhole_y + 1, 
               lockSize/10, lockSize/6, TFT_BLACK);
    
    tft.fillCircle(keyhole_x, keyhole_y, lockSize/10, tft.color565(15, 25, 40));
    tft.fillRect(keyhole_x - lockSize/20, keyhole_y, 
               lockSize/10, lockSize/6, tft.color565(15, 25, 40));

    tft.drawPixel(keyhole_x + lockSize/20, keyhole_y - lockSize/20, tft.color565(accentR, accentG, accentB));
    tft.drawPixel(keyhole_x + lockSize/20 - 1, keyhole_y - lockSize/20 + 1, tft.color565(accentR, accentG, accentB));
    
    tft.setTextDatum(TC_DATUM);
    
    int titleY = logoY + lockSize + 30;
    tft.fillRect(0, titleY - 15, width, 50, tft.color565(primaryR, primaryG, primaryB));

    tft.setTextColor(tft.color565(20, 30, 40));
    tft.setFreeFont(FSB9);
    tft.drawString("SMART DOOR LOCK", centerX + 2, titleY + 2, GFXFF);
    
    tft.setTextColor(tft.color565(accentR, accentG, accentB));
    tft.drawString("SMART DOOR LOCK", centerX, titleY, GFXFF);
    delay(300);
    
    int versionY = titleY + 40;
    tft.fillRect(0, versionY - 15, width, 40, tft.color565(primaryR, primaryG, primaryB));
    
    tft.setTextColor(tft.color565(20, 30, 40));
    tft.drawString("Security System v2.0", centerX + 1, versionY + 1, GFXFF);
    
    tft.setTextColor(tft.color565(blueR, blueG, blueB));
    tft.drawString("Security System v2.0", centerX, versionY, GFXFF);
    delay(300);

    int barHeight = 10;
    int barY = height - barHeight - 5;
    int barWidth = width - 20;
    int barX = (width - barWidth) / 2;
    
    tft.drawRoundRect(
        barX, barY, 
        barWidth, barHeight, 
        barHeight/2, 
        tft.color565(accentR, accentG, accentB)
    );
    
    String loadingTexts[] = {
        "Initializing modules...",
        "Loading security systems...",
        "Connecting to network...",
        "Setting up sensors...",
        "Ready to secure!"
    };
    
    for (int step = 0; step < 5; step++) {
        int startPercent = step * 20;
        int endPercent = (step + 1) * 20;
        
        for (int p = startPercent; p <= endPercent; p++) {
            int progressWidth = map(p, 0, 100, 0, barWidth - 4);
            
            float factor = p / 100.0;
            uint8_t r = accentR * (1.0 - factor*0.2);
            uint8_t g = accentG * (1.0 - factor*0.1);
            uint8_t b = accentB + factor * (255 - accentB) * 0.3;
            
            tft.fillRoundRect(
                barX + 2, barY + 2,
                progressWidth, barHeight - 4,
                (barHeight - 4)/2,
                tft.color565(r, g, b)
            );
            delay(15);
        }
        delay(100);
    }
    
    for (int i = 0; i < 3; i++) {
        if (i % 2 == 0) {
            tft.fillScreen(tft.color565(accentR, accentG, accentB));
        } else {
            tft.fillScreen(tft.color565(primaryR, primaryG, primaryB));
        }
        delay(150);
    }
    
    tft.fillScreen(tft.color565(primaryR, primaryG, primaryB));
    for (int i = 0; i < height; i += 3) {
        int alpha = i * 255 / height;
        uint8_t r = map(alpha, 0, 255, primaryR, 10);
        uint8_t g = map(alpha, 0, 255, primaryG, 10);
        uint8_t b = map(alpha, 0, 255, primaryB, 10);
        
        tft.drawFastHLine(0, i, width, tft.color565(r, g, b));
        if (i % 9 == 0) delay(5);
    }
    
    delay(300);
    tft.setRotation(1);
    
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tftOutput);
}

void showingImage() {
    tft.setRotation(1);
    WebsocketsMessage msg = WebSocketClient.readBlocking();
    TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());
}

bool faceAuthentication() {
    Serial.println("Face Authentication Start");
    WebsocketsMessage msg = WebSocketClient.readBlocking();
    tft.setRotation(1);
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

void displayUnlockScreen() {
    tft.setRotation(0);
    
    uint8_t primaryR = 36, primaryG = 48, primaryB = 63; // #24303f - Xanh đen tối
    uint8_t accentR = 235, accentG = 244, accentB = 93;  // #ebf45d - Vàng chanh
    uint8_t blueR = 37, blueG = 99, blueB = 235;         // #2563eb - Xanh dương
    uint8_t greenR = 34, greenG = 197, greenB = 94;      // Success green
    
    int width = tft.width();
    int height = tft.height();
    int centerX = width / 2;
    int centerY = height / 2;
    
    for (int i = 0; i < 12; i++) {
        float factor = i / 12.0;
        uint8_t r = primaryR * (1.0 - factor) + greenR * factor;
        uint8_t g = primaryG * (1.0 - factor) + greenG * factor;
        uint8_t b = primaryB * (1.0 - factor) + greenB * factor;
        
        tft.fillScreen(tft.color565(r, g, b));
        delay(60);
    }
    
    for (int y = 0; y < height; y++) {
        float factor = y / (float)height;
        uint8_t r = greenR - (factor * 30);
        uint8_t g = greenG - (factor * 40);
        uint8_t b = greenB + (factor * 30);
        tft.drawFastHLine(0, y, width, tft.color565(r, g, b));
    }
    
    for (int r = 90; r > 0; r -= 5) {
        uint8_t alpha = map(r, 90, 0, 10, 150);
        uint16_t color = tft.color565(
            accentR * alpha / 255,
            accentG * alpha / 255,
            accentB * alpha / 255
        );
        tft.drawCircle(centerX, centerY - 20, r, color);
        if (r % 15 == 0) delay(30);
    }
    
    int logoY = height / 3 - 10;
    int lockSize = width / 3;
    
    tft.fillRoundRect(
        centerX - lockSize/2 + 5, 
        logoY + lockSize/3 + 5, 
        lockSize, 
        lockSize, 
        15, 
        tft.color565(20, 70, 40)
    );
    
    for (int i = 8; i >= 0; i--) {
        uint8_t shade = i * 15;
        tft.fillRoundRect(
            centerX - lockSize/2 + i/2, 
            logoY + lockSize/3 + i/2, 
            lockSize - i, 
            lockSize - i, 
            15, 
            tft.color565(greenR - shade/2, greenG - shade, greenB - shade/2)
        );
        delay(30);
    }
    
    tft.drawRoundRect(
        centerX - lockSize/2, 
        logoY + lockSize/3, 
        lockSize, 
        lockSize, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );

    tft.drawRoundRect(
        centerX - lockSize/2 + 1, 
        logoY + lockSize/3 + 1, 
        lockSize - 2, 
        lockSize - 2, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );
    
    for (int i = 0; i <= 10; i++) {
        float factor = i / 10.0;
        int radius = (lockSize/2) * factor;
        if (radius < 3) continue;
        
        int angle = 70 + i * 7;
        float radian = angle * PI / 180.0;
        
        int cX = centerX - (int)(cos(radian) * radius * 0.8);
        int cY = logoY - (int)(sin(radian) * radius * 0.8);
        
        if (i > 0) {
            for (int y = logoY - lockSize/2; y < logoY + 10; y++) {
                float f = y / (float)height;
                uint8_t r = greenR - (f * 30);
                uint8_t g = greenG - (f * 40);
                uint8_t b = greenB + (f * 30);
                tft.drawFastHLine(0, y, width, tft.color565(r, g, b));
            }
        }
        
        tft.fillCircle(cX, cY, radius/4, tft.color565(greenR + 30, greenG + 30, greenB - 30));
        tft.drawCircle(cX, cY, radius/4, tft.color565(accentR, accentG, accentB));
        
        int stemW = radius/4;
        int stemH = radius/2;
        tft.fillRoundRect(
            cX - stemW/2,
            cY,
            stemW,
            stemH,
            stemW/2,
            tft.color565(greenR + 30, greenG + 30, greenB - 30)
        );
        
        delay(50);
    }
    
    int keyhole_x = centerX;
    int keyhole_y = logoY + lockSize/2 + lockSize/8;
    
    tft.fillCircle(keyhole_x + 1, keyhole_y + 1, lockSize/10, tft.color565(20, 50, 30));
    tft.fillRect(keyhole_x - lockSize/20 + 1, keyhole_y + 1, 
              lockSize/10, lockSize/6, tft.color565(20, 50, 30));
    
    tft.fillCircle(keyhole_x, keyhole_y, lockSize/10, tft.color565(15, 90, 40));
    tft.fillRect(keyhole_x - lockSize/20, keyhole_y, 
              lockSize/10, lockSize/6, tft.color565(15, 90, 40));
    
    int checkY = logoY + lockSize + 30;
    
    for (int r = 0; r <= 25; r += 5) {
        tft.fillCircle(centerX, checkY, r, tft.color565(accentR, accentG, accentB));
        delay(40);
    }
    
    tft.drawCircle(centerX, checkY, 25, tft.color565(20, 70, 40));
    tft.drawCircle(centerX, checkY, 26, tft.color565(20, 70, 40));
    
    for (int i = 0; i <= 10; i++) {
        float factor = i / 10.0;
        
        int x1 = centerX - 12;
        int y1 = checkY;
        int x2 = centerX - 2 + (int)(factor * (centerX - 2 - (centerX - 12)));
        int y2 = checkY + 10 * factor;
        
        tft.drawLine(x1, y1, x2, y2, TFT_BLACK);
        tft.drawLine(x1 + 1, y1, x2 + 1, y2, TFT_BLACK);
        
        if (i >= 5) {
            float factor2 = (i - 5) / 5.0;
            if (factor2 > 1.0) factor2 = 1.0;
            
            int x3 = centerX - 2;
            int y3 = checkY + 10;
            int x4 = centerX - 2 + (int)(factor2 * ((centerX + 12) - (centerX - 2)));
            int y4 = checkY + 10 - (int)(factor2 * 20);
            
            tft.drawLine(x3, y3, x4, y4, TFT_BLACK);
            tft.drawLine(x3 + 1, y3, x4 + 1, y4, TFT_BLACK);
        }
        
        delay(40);
    }
    
    int textY = checkY + 50;
    
    tft.fillRect(0, textY - 15, width, 40, tft.color565(greenR - 15, greenG - 20, greenB + 10));
    
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(tft.color565(20, 70, 40));
    tft.drawString("SYSTEM UNLOCKED", centerX + 2, textY + 2, GFXFF);
    
    tft.setTextColor(tft.color565(accentR, accentG, accentB));
    tft.drawString("SYSTEM UNLOCKED", centerX, textY, GFXFF);
    
    delay(200);
    
    textY += 30;
    tft.fillRect(0, textY - 10, width, 30, tft.color565(greenR - 15, greenG - 20, greenB + 10));
    
    tft.setTextColor(tft.color565(20, 70, 40));
    tft.drawString("Access Granted", centerX + 1, textY + 1, GFXFF);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Access Granted", centerX, textY, GFXFF);
    
    int countdownSeconds = 3;
    int timerY = height - 50;
    
    tft.fillRoundRect(
        centerX - 70, 
        timerY, 
        140, 
        35, 
        15, 
        tft.color565(primaryR, primaryG, primaryB)
    );
    
    tft.drawRoundRect(
        centerX - 70, 
        timerY, 
        140, 
        35, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );
    
    tft.drawRoundRect(
        centerX - 71, 
        timerY - 1, 
        142, 
        37, 
        15, 
        tft.color565(accentR, accentG, accentB)
    );
    
    for (int t = countdownSeconds; t >= 0; t--) {
        tft.fillRect(
            centerX - 65, 
            timerY + 5, 
            130, 
            25, 
            tft.color565(primaryR, primaryG, primaryB)
        );
        
        tft.setTextDatum(MC_DATUM);
        
        if (t > 0) {
            char timeStr[20];
            sprintf(timeStr, "Returning in %d", t);
            
            tft.setTextColor(tft.color565(accentR, accentG, accentB));
            tft.drawString(timeStr, centerX, timerY + 17, GFXFF);
            
            int barWidth = map(t, countdownSeconds, 0, 10, 130);
            tft.fillRect(
                centerX - 65, 
                timerY + 28, 
                barWidth, 
                3, 
                tft.color565(blueR, blueG, blueB)
            );
        } else {
            tft.setTextColor(tft.color565(accentR, accentG, accentB));
            tft.drawString("Returning now", centerX, timerY + 17, GFXFF);
        }
        
        if (t > 0) {
            delay(1000);
        } else {
            delay(300);
        }
    }
    
    for (int i = 0; i < 10; i++) {
        uint8_t r = map(i, 0, 10, greenR - 15, 0);
        uint8_t g = map(i, 0, 10, greenG - 20, 0);
        uint8_t b = map(i, 0, 10, greenB + 10, 0);
        
        tft.fillScreen(tft.color565(r, g, b));
        delay(40);
    }

    tft.setRotation(1);
}

void displayEmergencyLockScreen() {
    tft.setRotation(0);
    tft.fillScreen(TFT_RED);
    
    int centerX = tft.width() / 2;
    int screenHeight = tft.height();
    
    int iconY = screenHeight / 5;
    
    tft.fillRoundRect(centerX - 30, iconY, 60, 45, 10, TFT_WHITE);
    
    tft.fillCircle(centerX, iconY - 15, 20, TFT_RED);
    tft.drawCircle(centerX, iconY - 15, 20, TFT_WHITE);
    tft.drawCircle(centerX, iconY - 15, 19, TFT_WHITE);
    
    tft.fillCircle(centerX, iconY + 20, 8, TFT_RED);
    tft.fillRect(centerX - 2, iconY + 20, 4, 15, TFT_RED);
    
    tft.drawRoundRect(centerX - 30, iconY, 60, 45, 10, TFT_YELLOW);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    
    int textY = iconY + 70;
    tft.setTextSize(2);
    tft.drawString("SYSTEM LOCKED", centerX, textY, 2);
    
    textY += 35;
    tft.setTextSize(1);
    tft.drawString("Security Alert", centerX, textY, 2);
    
    textY += 20;
    tft.drawString("Too many failed attempts", centerX, textY, 2);
    
    textY += 25;
    tft.drawString("Unlock via app", centerX, textY, 2);
}
