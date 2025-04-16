#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>

#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "ESP32-CAMERA";
const char* password = "12345678";

const char* websockets_server_host = "192.168.4.1";
const uint16_t websockets_server_port = 8888;

using namespace websockets;
WebsocketsClient client;

unsigned long lastConnectionAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 3000;
bool isConnected = false;

bool connectToWebSocket() {
  Serial.print("Connecting to WebSocket server at ");
  Serial.print(websockets_server_host);
  Serial.print(":");
  Serial.println(websockets_server_port);
  
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
  
  if (connected) {
    Serial.println("Socket Connected!");
    isConnected = true;
  } else {
    Serial.println("Socket Connection Failed!");
    isConnected = false;
  }
  
  return connected;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, 1);
    s->set_special_effect(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 30);
    s->set_aec_value(s, 500);
    s->set_ae_level(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_denoise(s, 1);
    s->set_sharpness(s, 1);
    s->set_dcw(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_wpc(s, 1);
    s->set_bpc(s, 1);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  connectToWebSocket();
  lastConnectionAttempt = millis();
}

void loop() {
  if (!client.available()) {
    if (isConnected) {
      Serial.println("WebSocket connection lost!");
      isConnected = false;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastConnectionAttempt >= RECONNECT_INTERVAL) {
      Serial.println("Attempting to reconnect...");
      connectToWebSocket();
      lastConnectionAttempt = currentTime;
      return;
    }
    
    delay(100);
    return;
  }
  
  client.poll();
  
  if (isConnected) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    fb = esp_camera_fb_get();
    if(!fb){
      Serial.println("Camera capture failed");
      esp_camera_fb_return(fb);
      return;
    }

    size_t fb_len = 0;
    if(fb->format != PIXFORMAT_JPEG){
      Serial.println("Non-JPEG data not implemented");
      return;
    }

    if (client.available()) {
      if (!client.sendBinary((const char*) fb->buf, fb->len)) {
        Serial.println("Failed to send image, connection might be lost");
        isConnected = false;
      }
    }
    
    esp_camera_fb_return(fb);
  }
}