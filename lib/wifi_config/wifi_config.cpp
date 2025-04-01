#include "wifi_config.h"

// Khởi tạo các biến toàn cục
WebServer webServer(80);
String ssid;
String password;

// 0: Chế độ cấu hình, 1: Chế độ kết nối, 2: Mất wifi
int wifiMode;

const char html[] PROGMEM = R"html(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Lock System - Wifi Config</title>
    <style>
        * {
            box-sizing: border-box;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            margin: 0;
            padding: 0;
        }
        
        body {
            background-color: #f7f7f7;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        
        .container {
            max-width: 480px;
            width: 100%;
            background-color: #ffffff;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
            padding: 24px;
        }
        
        /* Header */
        .header {
            margin-bottom: 24px;
        }
        
        .header-title {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 8px;
        }
        
        .header-title h1 {
            font-size: 1.5rem;
            font-weight: 600;
            color: #24303f;
        }
        
        .header-description {
            color: #6b7280;
            font-size: 0.875rem;
        }
        
        /* Status bar */
        .status {
            padding: 12px;
            margin-bottom: 20px;
            border-radius: 6px;
            font-size: 0.875rem;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .status-info {
            background-color: #f3f4f6;
            color: #4b5563;
        }
        
        .status-success {
            background-color: #f0fdf4;
            color: #166534;
            border: 1px solid #dcfce7;
        }
        
        .status-error {
            background-color: #fef2f2;
            color: #b91c1c;
            border: 1px solid #fee2e2;
        }
        
        .status-warning {
            background-color: #fffbeb;
            color: #92400e;
            border: 1px solid #fef3c7;
        }
        
        /* Form elements */
        .form-group {
            margin-bottom: 16px;
        }
        
        label {
            display: block;
            margin-bottom: 6px;
            font-size: 0.875rem;
            font-weight: 500;
            color: #374151;
        }
        
        select, input {
            width: 100%;
            padding: 10px 14px;
            border: 1px solid #e5e7eb;
            border-radius: 6px;
            font-size: 0.875rem;
            color: #1f2937;
            transition: all 0.2s;
        }
        
        select:focus, input:focus {
            outline: none;
            border-color: #ebf45d;
            box-shadow: 0 0 0 2px rgba(235, 244, 93, 0.3);
        }
        
        .button-row {
            display: flex;
            gap: 12px;
            margin-top: 20px;
        }
        
        button {
            padding: 10px 16px;
            border: none;
            border-radius: 6px;
            font-weight: 500;
            font-size: 0.875rem;
            cursor: pointer;
            transition: all 0.2s;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        .primary-btn {
            background-color: #ebf45d;
            color: #24303f;
            flex: 1;
        }
        
        .primary-btn:hover {
            background-color: #d9e154;
        }
        
        .secondary-btn {
            background-color: #f3f4f6;
            color: #4b5563;
            flex: 1;
        }
        
        .secondary-btn:hover {
            background-color: #e5e7eb;
        }
        
        .full-width-btn {
            width: 100%;
            margin-top: 12px;
        }
        
        /* Icons */
        .icon {
            display: inline-block;
            width: 20px;
            height: 20px;
        }
        
        .loading-spinner {
            animation: spin 1s linear infinite;
            width: 16px;
            height: 16px;
            border: 2px solid #6b7280;
            border-top-color: transparent;
            border-radius: 50%;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="header-title">
                <svg class="icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M12 14.5V16.5M7 10.5V8.5C7 5.73858 9.23858 3.5 12 3.5C14.7614 3.5 17 5.73858 17 8.5V10.5M8.5 10.5H15.5C16.6046 10.5 17.5 11.3954 17.5 12.5V19.5C17.5 20.6046 16.6046 21.5 15.5 21.5H8.5C7.39543 21.5 6.5 20.6046 6.5 19.5V12.5C6.5 11.3954 7.39543 10.5 8.5 10.5Z" stroke="#24303f" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                <h1>Smart Lock System</h1>
            </div>
            <p class="header-description">Wifi Config</p>
        </div>
        
        <div id="status" class="status status-info">
            <div id="status-spinner" class="loading-spinner"></div>
            <span id="status-text">Scanning WiFi...</span>
        </div>
        
        <form id="wifi-form">
            <div class="form-group">
                <label for="ssid">Wifi Name</label>
                <select id="ssid">
                    <option value="">Select Wifi</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="password">Password</label>
                <input type="password" id="password" placeholder="Enter Wifi Password">
            </div>
            
            <div class="button-row">
                <button type="button" onclick="scanWifi()" class="secondary-btn">
                    <svg class="icon" width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M4 4V9H4.58152M19.9381 11C19.446 7.05369 16.0796 4 12 4C8.64262 4 5.76829 6.06817 4.58152 9M4.58152 9H9M20 20V15H19.4185M19.4185 15C18.2317 17.9318 15.3574 20 12 20C7.92038 20 4.55399 16.9463 4.06189 13M19.4185 15H15" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    Scan again
                </button>
                <button type="button" onclick="saveWifi()" class="primary-btn">
                    <svg class="icon" width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M20 6L9 17L4 12" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    Save config
                </button>
            </div>
            
            <button type="button" onclick="reStart()" class="secondary-btn full-width-btn">
                <svg class="icon" width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M17.5001 6.5H22.0001M17.5001 6.5C17.5001 5.39543 16.6047 4.5 15.5001 4.5H8.50012C7.39555 4.5 6.50012 5.39543 6.50012 6.5M17.5001 6.5V17.5C17.5001 18.6046 16.6047 19.5 15.5001 19.5H8.50012C7.39555 19.5 6.50012 18.6046 6.50012 17.5V6.5M6.50012 6.5H2.00012M9.5 15C9.5 15.5523 9.05228 16 8.5 16C7.94772 16 7.5 15.5523 7.5 15C7.5 14.4477 7.94772 14 8.5 14C9.05228 14 9.5 14.4477 9.5 15ZM9.5 9C9.5 9.55228 9.05228 10 8.5 10C7.94772 10 7.5 9.55228 7.5 9C7.5 8.44772 7.94772 8 8.5 8C9.05228 8 9.5 8.44772 9.5 9ZM12 12.25C12 12.6642 11.6642 13 11.25 13C10.8358 13 10.5 12.6642 10.5 12.25C10.5 11.8358 10.8358 11.5 11.25 11.5C11.6642 11.5 12 11.8358 12 12.25Z" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                Restart device
            </button>
        </form>
    </div>

    <script>
        // Hàm thay đổi trạng thái
        function updateStatus(message, type) {
            const statusElement = document.getElementById("status");
            const statusTextElement = document.getElementById("status-text");
            const spinnerElement = document.getElementById("status-spinner");
            
            statusTextElement.innerText = message;
            
            // Xóa tất cả các class status hiện tại
            statusElement.classList.remove("status-info", "status-success", "status-error", "status-warning");
            
            // Thêm class mới dựa trên loại
            statusElement.classList.add("status-" + type);
            
            // Hiện/ẩn spinner
            if (type === "info") {
                spinnerElement.style.display = "inline-block";
            } else {
                spinnerElement.style.display = "none";
            }
        }

        // Hàm thực hiện khi trang web được tải
        window.onload = function() {
            scanWifi();
        };

        // Tạo đối tượng XMLHttpRequest để gửi yêu cầu đến máy chủ
        var xhr = new XMLHttpRequest();

        // Hàm quét mạng WiFi
        function scanWifi() {
            updateStatus("Scanning WiFi...", "info");
            
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        try {
                            var data = xhr.responseText;
                            var networks = JSON.parse(data);
                            
                            updateStatus("Found " + networks.length + " wifi", "success");
                            
                            var select = document.getElementById("ssid");
                            select.innerHTML = "<option value=''>Select Wifi</option>";
                            
                            for (var i = 0; i < networks.length; i++) {
                                var option = document.createElement("option");
                                option.value = networks[i];
                                option.text = networks[i];
                                select.appendChild(option);
                            }
                        } catch (e) {
                            updateStatus("Error processing data: " + e.message, "error");
                        }
                    } else {
                        updateStatus("Error connecting to server", "error");
                    }
                }
            };
            
            xhr.open("GET", "/scanWifi", true);
            xhr.send();
        }

        // Hàm lưu thông tin WiFi
        function saveWifi() {
            var ssid = document.getElementById("ssid").value;
            var password = document.getElementById("password").value;
            
            if (!ssid) {
                updateStatus("Please select a wifi", "warning");
                return;
            }
            
            updateStatus("Saving config...", "info");
            
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        updateStatus(xhr.responseText, "success");
                    } else {
                        updateStatus("Error saving config", "error");
                    }
                }
            };
            
            xhr.open("GET", "/saveWifi?ssid=" + encodeURIComponent(ssid) + "&pass=" + encodeURIComponent(password), true);
            xhr.send();
        }

        // Hàm khởi động lại ESP32
        function reStart() {
            if (confirm("Are you sure you want to restart the device?")) {
                updateStatus("Restarting...", "info");
                
                xhr.onreadystatechange = function() {
                    if (xhr.readyState == 4 && xhr.status == 200) {
                        updateStatus(xhr.responseText, "success");
                    }
                };
                
                xhr.open("GET", "/reStart", true);
                xhr.send();
            }
        }
    </script>
</body>
</html>
)html";

void createAccessPoint(){
    Serial.println("ESP32 wifi network created!");
    WiFi.mode(WIFI_AP);
    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);

    String ssid_ap = "ESP32-" + String(macAddr[4], HEX) + String(macAddr[5], HEX);
    String password_ap = "12345678";
    ssid_ap.toUpperCase();
    WiFi.softAP(ssid_ap.c_str(), password_ap.c_str());

    Serial.println("Access point name:" + ssid_ap);
    Serial.println("Web server access address:" + WiFi.softAPIP().toString());

    wifiMode = 0;
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case IP_EVENT_STA_GOT_IP:
            Serial.println("-----------------------------------");
            Serial.println("WiFi connected!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            Serial.println("-----------------------------------");
            wifiMode = 1;
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED:
            Serial.println("-----------------------------------");
            Serial.println("WiFi disconnected! Reconnecting...");
            Serial.println("-----------------------------------");
            wifiMode = 2;
            WiFi.begin(ssid, password);
            break;
        default:
            break;
    }
}

void setupWifi() {
    if (ssid != "") {
        Serial.println("-----------------------------------");
        Serial.println("Connecting to WiFi...");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.println("-----------------------------------");
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        WiFi.onEvent(WiFiEvent);
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            wifiMode = 1;
        } else {
            Serial.println("Cannot connect to WiFi. Switching to AP mode.");
            WiFi.disconnect();
            delay(1000);
            createAccessPoint();
        }
    } else {
        createAccessPoint();
    }
}

void setupWebServer() {
    webServer.on("/", HTTP_GET, []() {
        Serial.println("Received request to load home page");
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/html", html);
    });
    
    // Xử lý quét WiFi
    webServer.on("/scanWifi", HTTP_GET, []() {
        Serial.println("Received request to scan WiFi");
        
        int networks = WiFi.scanNetworks(true, true);
        unsigned long timeout = millis();
        
        while (networks < 0 && millis() - timeout < 10000) {
            delay(100);
            networks = WiFi.scanComplete();
        }
        
        DynamicJsonDocument doc(2048);
        JsonArray array = doc.to<JsonArray>();
        
        for (int i = 0; i < networks; i++) {
            array.add(WiFi.SSID(i));
        }
        
        String wifiList;
        serializeJson(array, wifiList);
        
        Serial.print("Number of WiFi networks found: ");
        Serial.println(networks);
        
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "application/json", wifiList);
    });
    
    // Xử lý lưu cấu hình WiFi
    webServer.on("/saveWifi", HTTP_GET, []() {
        Serial.println("Received request to save WiFi");
        String ssid_temp = webServer.arg("ssid");
        String password_temp = webServer.arg("pass");
        
        if (ssid_temp.length() > 0) {
            Serial.println("-----------------------------------");
            Serial.println("Saving WiFi information:");
            Serial.print("SSID: ");
            Serial.println(ssid_temp);
            Serial.println("-----------------------------------");
            
            EEPROM.writeString(0, ssid_temp);
            EEPROM.writeString(32, password_temp);
            EEPROM.commit();
            
            webServer.sendHeader("Connection", "close");
            webServer.send(200, "text/plain", "WiFi information saved successfully!");
        } else {
            webServer.sendHeader("Connection", "close");
            webServer.send(400, "text/plain", "Invalid WiFi name!");
        }
    });
    
    webServer.on("/reStart", HTTP_GET, []() {
        Serial.println("Received request to restart device");
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/plain", "Device is restarting...");
        delay(1000);
        ESP.restart();
    });
    
    webServer.onNotFound([]() {
        Serial.println("Received request to non-existent page - redirecting to home page");
        webServer.sendHeader("Location", "/", true);
        webServer.send(302, "text/plain", "");
    });
    
    webServer.begin();
    Serial.println("Web server is ready!");
}

void wifiConfigInit() {
    Serial.println("======== WiFi Config Init ========");
    
    // Khởi tạo EEPROM
    EEPROM.begin(100);
    Serial.println("EEPROM initialized");
    
    // Đọc thông tin WiFi từ EEPROM
    char ssid_temp[32], password_temp[64];
    EEPROM.readString(0, ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32, password_temp, sizeof(password_temp));
    
    ssid = String(ssid_temp);
    password = String(password_temp);
    
    if (ssid != "") {
        Serial.println("-----------------------------------");
        Serial.println("Reading WiFi information from EEPROM:");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.println("-----------------------------------");
    } else {
        Serial.println("No WiFi information found in EEPROM");
    }

    setupWifi();
    
    // Nếu ở chế độ cấu hình, thiết lập web server
    if (wifiMode == 0) {
        Serial.println("Starting web server...");
        setupWebServer();
    }
    
    Serial.println("===================================");
}

void wifiConfigRun() {
    if (wifiMode == 0) {
        webServer.handleClient();
    }
}

// Kiểm tra kết nối WiFi
bool checkWiFiStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi is connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("WiFi is not connected!");
        return false;
    }
}

void wifiAPSetup() {
    String ssid_ap = "ESP32-CAMERA";
    String password_ap = "12345678";
    WiFi.softAP(ssid_ap.c_str(), password_ap.c_str());
    Serial.println("Access Point Setup: " + ssid_ap);
}



