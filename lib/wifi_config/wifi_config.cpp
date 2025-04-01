#include "wifi_config.h"


// Khởi tạo các biến toàn cục
WebServer webServer(80);
String ssid;
String password;
String deviceId;
String macAddress;
String secretKey;
String userId;

// 0: Config
// 1: Connect
// 2: Lost
int wifiMode;
bool isWifiConnected = false;

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
        
        .loading-container {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 12px;
            background-color: #f3f4f6;
            border-radius: 6px;
            margin-top: 12px;
        }

        .loading-spinner {
            width: 20px;
            height: 20px;
            border: 2px solid #6b7280;
            border-top-color: transparent;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }

        .loading-text {
            color: #4b5563;
            font-size: 0.875rem;
        }

        @keyframes spin {
            to { transform: rotate(360deg); }
        }

        /* Nâng cao hiệu ứng cho header status */
        .header-status {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 8px 12px;
            background-color: #f3f4f6;
            border-radius: 6px;
            margin-top: 8px;
            transition: all 0.3s ease;
            opacity: 0;
            max-height: 0;
            overflow: hidden;
            padding-top: 0;
            padding-bottom: 0;
        }

        .header-status.show {
            display: flex;
            opacity: 1;
            max-height: 60px;
            padding-top: 8px;
            padding-bottom: 8px;
        }

        .header-status .loading-spinner {
            width: 16px;
            height: 16px;
        }

        .header-status .success-icon {
            display: none;
            width: 16px;
            height: 16px;
            color: #059669;
        }

        .header-status.success .loading-spinner {
            display: none;
        }

        .header-status.success .success-icon {
            display: block;
        }

        .header-status .status-text {
            font-size: 0.875rem;
            color: #4b5563;
        }

        .welcome-message {
            background-color: #f0fdf4;
            color: #166534;
            border: 1px solid #dcfce7;
            padding: 12px;
            border-radius: 6px;
            margin-top: 12px;
            margin-bottom: 12px;
            font-size: 0.875rem;
            display: none;
        }

        /* Thêm CSS cho trang chặn truy cập không có userId */
        .auth-container {
            text-align: center;
            display: none;
        }
        
        .auth-error {
            background-color: #fef2f2;
            color: #b91c1c;
            border: 1px solid #fee2e2;
            padding: 16px;
            border-radius: 6px;
            margin-bottom: 16px;
        }
        
        .auth-icon {
            font-size: 48px;
            margin-bottom: 16px;
        }
        
        .main-content {
            display: none;
        }

        /* Nút scanning wifi với hiệu ứng xoay */
        .scan-button {
            display: flex;
            align-items: center;
            gap: 8px;
            background-color: #f3f4f6;
            color: #4b5563;
            padding: 10px 16px;
            border-radius: 6px;
            border: none;
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .scan-button:hover {
            background-color: #e5e7eb;
        }
        
        .scan-animation {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 2px solid #6b7280;
            border-top-color: transparent;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }

        /* Thêm hiệu ứng loading khi kiểm tra xác thực */
        .verification-status {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 12px;
            background-color: #f3f4f6;
            border-radius: 6px;
            margin-top: 16px;
            font-size: 0.875rem;
            color: #4b5563;
        }
        
        .verification-status.hidden {
            display: none;
        }

        /* Hiệu ứng loading overlay */
        .loading-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(255, 255, 255, 0.7);
            display: flex;
            justify-content: center;
            align-items: center;
            z-index: 1000;
            flex-direction: column;
            gap: 20px;
            opacity: 0;
            visibility: hidden;
            transition: opacity 0.3s ease, visibility 0.3s ease;
        }

        .loading-overlay.show {
            opacity: 1;
            visibility: visible;
        }

        .loading-overlay-spinner {
            width: 40px;
            height: 40px;
            border: 3px solid #e5e7eb;
            border-top-color: #ebf45d;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }

        .loading-overlay-text {
            font-size: 1rem;
            color: #4b5563;
            text-align: center;
            max-width: 80%;
        }

        /* Cải thiện nút restart */
        .restart-btn {
            margin-top: 20px;
            background-color: #f3f4f6;
            color: #4b5563;
            font-weight: 500;
            transition: all 0.3s ease;
        }
        
        .restart-btn:hover {
            background-color: #e5e7eb;
            transform: translateY(-1px);
        }
        
        .restart-btn .icon {
            color: #4b5563;
        }
    </style>
</head>
<body>
    <div id="loading-overlay" class="loading-overlay">
        <div class="loading-overlay-spinner"></div>
        <div class="loading-overlay-text">Initializing device and checking connection status...</div>
    </div>
    
    <div class="container">
        <div id="auth-container" class="auth-container">
            <div class="auth-icon">🔒</div>
            <div class="auth-error">
                <h2>Unauthorized Access</h2>
                <p>This smart lock device has not been registered to any user.</p>
                <p>Please complete device registration process through mobile app first.</p>
            </div>
            
            <div id="verification-status" class="verification-status">
                <div class="loading-spinner"></div>
                <span>Checking device verification status...</span>
            </div>
        </div>
        
        <div id="main-content" class="main-content">
            <div class="header">
                <div class="header-title">
                    <svg class="icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 14.5V16.5M7 10.5V8.5C7 5.73858 9.23858 3.5 12 3.5C14.7614 3.5 17 5.73858 17 8.5V10.5M8.5 10.5H15.5C16.6046 10.5 17.5 11.3954 17.5 12.5V19.5C17.5 20.6046 16.6046 21.5 15.5 21.5H8.5C7.39543 21.5 6.5 20.6046 6.5 19.5V12.5C6.5 11.3954 7.39543 10.5 8.5 10.5Z" stroke="#24303f" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    <h1>Smart Lock System</h1>
                </div>
                <p class="header-description">Wifi Config</p>
                <div id="welcome-message" class="welcome-message">
                    Welcome back! Your device is verified with User ID: <span id="welcome-userId"></span>
                </div>
                <div id="header-status" class="header-status">
                    <div class="loading-spinner"></div>
                    <svg class="success-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M20 6L9 17L4 12" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    <span class="status-text">Waiting for create device from Server</span>
                </div>
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
                    <button type="button" id="scan-button" class="secondary-btn">
                        <div id="scan-icon" class="icon">
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                                <path d="M4 4V9H4.58152M19.9381 11C19.446 7.05369 16.0796 4 12 4C8.64262 4 5.76829 6.06817 4.58152 9M4.58152 9H9M20 20V15H19.4185M19.4185 15C18.2317 17.9318 15.3574 20 12 20C7.92038 20 4.55399 16.9463 4.06189 13M19.4185 15H15" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                            </svg>
                        </div>
                        <span id="scan-text">Scan again</span>
                    </button>
                    <button type="button" onclick="saveWifi()" class="primary-btn">
                        <svg class="icon" width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <path d="M20 6L9 17L4 12" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                        </svg>
                        Save config
                    </button>
                </div>
                
                <button type="button" onclick="reStart()" class="secondary-btn full-width-btn restart-btn">
                    <svg class="icon" width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M17.5001 6.5H22.0001M17.5001 6.5C17.5001 5.39543 16.6047 4.5 15.5001 4.5H8.50012C7.39555 4.5 6.50012 5.39543 6.50012 6.5M17.5001 6.5V17.5C17.5001 18.6046 16.6047 19.5 15.5001 19.5H8.50012C7.39555 19.5 6.50012 18.6046 6.50012 17.5V6.5M6.50012 6.5H2.00012M9.5 15C9.5 15.5523 9.05228 16 8.5 16C7.94772 16 7.5 15.5523 7.5 15C7.5 14.4477 7.94772 14 8.5 14C9.05228 14 9.5 14.4477 9.5 15ZM9.5 9C9.5 9.55228 9.05228 10 8.5 10C7.94772 10 7.5 9.55228 7.5 9C7.5 8.44772 7.94772 8 8.5 8C9.05228 8 9.5 8.44772 9.5 9ZM12 12.25C12 12.6642 11.6642 13 11.25 13C10.8358 13 10.5 12.6642 10.5 12.25C10.5 11.8358 10.8358 11.5 11.25 11.5C11.6642 11.5 12 11.8358 12 12.25Z" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    Restart device
                </button>
            </form>
            
            <div id="device-info" style="display: none; margin-top: 24px; padding: 16px; background-color: #f3f4f6; border-radius: 6px;">
                <div id="loading-status" class="loading-container" style="display: none;">
                    <div class="loading-spinner"></div>
                    <span class="loading-text">Waiting for show device information</span>
                </div>
                <div class="header-title" style="margin-bottom: 16px;">
                    <svg class="icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 15C13.6569 15 15 13.6569 15 12C15 10.3431 13.6569 9 12 9C10.3431 9 9 10.3431 9 12C9 13.6569 10.3431 15 12 15Z" stroke="#24303f" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                        <path d="M19.4 15C19.2669 15.3016 19.2272 15.6362 19.286 15.9606C19.3448 16.285 19.4995 16.5843 19.73 16.82L19.79 16.88C19.976 17.0657 20.1235 17.2863 20.2241 17.5291C20.3248 17.7719 20.3766 18.0322 20.3766 18.295C20.3766 18.5578 20.3248 18.8181 20.2241 19.0609C20.1235 19.3037 19.976 19.5243 19.79 19.71C19.6043 19.896 19.3837 20.0435 19.1409 20.1441C18.8981 20.2448 18.6378 20.2966 18.375 20.2966C18.1122 20.2966 17.8519 20.2448 17.6091 20.1441C17.3663 20.0435 17.1457 19.896 16.96 19.71L16.9 19.65C16.6643 19.4195 16.365 19.2648 16.0406 19.206C15.7162 19.1472 15.3816 19.1869 15.08 19.32C14.7842 19.4468 14.532 19.6572 14.3543 19.9255C14.1766 20.1938 14.0813 20.5082 14.08 20.83V21C14.08 21.5304 13.8693 22.0391 13.4942 22.4142C13.1191 22.7893 12.6104 23 12.08 23C11.5496 23 11.0409 22.7893 10.6658 22.4142C10.2907 22.0391 10.08 21.5304 10.08 21V20.91C10.0723 20.579 9.96512 20.258 9.77251 19.9887C9.5799 19.7194 9.31074 19.5143 9 19.4C8.69838 19.2669 8.36381 19.2272 8.03941 19.286C7.71502 19.3448 7.41568 19.4995 7.18 19.73L7.12 19.79C6.93425 19.976 6.71368 20.1235 6.47088 20.2241C6.22808 20.3248 5.96783 20.3766 5.705 20.3766C5.44217 20.3766 5.18192 20.3248 4.93912 20.2241C4.69632 20.1235 4.47575 19.976 4.29 19.79C4.10405 19.6043 3.95653 19.3837 3.85588 19.1409C3.75523 18.8981 3.70343 18.6378 3.70343 18.375C3.70343 18.1122 3.75523 17.8519 3.85588 17.6091C3.95653 17.3663 4.10405 17.1457 4.29 16.96L4.35 16.9C4.58054 16.6643 4.73519 16.365 4.794 16.0406C4.85282 15.7162 4.81312 15.3816 4.68 15.08C4.55324 14.7842 4.34276 14.532 4.07447 14.3543C3.80618 14.1766 3.49179 14.0813 3.17 14.08H3C2.46957 14.08 1.96086 13.8693 1.58579 13.4942C1.21071 13.1191 1 12.6104 1 12.08C1 11.5496 1.21071 11.0409 1.58579 10.6658C1.96086 10.2907 2.46957 10.08 3 10.08H3.09C3.42099 10.0723 3.742 9.96512 4.0113 9.77251C4.28059 9.5799 4.48572 9.31074 4.6 9C4.73312 8.69838 4.77282 8.36381 4.714 8.03941C4.65519 7.71502 4.50054 7.41568 4.27 7.18L4.21 7.12C4.02405 6.93425 3.87653 6.71368 3.77588 6.47088C3.67523 6.22808 3.62343 5.96783 3.62343 5.705C3.62343 5.44217 3.67523 5.18192 3.77588 4.93912C3.87653 4.69632 4.02405 4.47575 4.21 4.29C4.39575 4.10405 4.61632 3.95653 4.85912 3.85588C5.10192 3.75523 5.36217 3.70343 5.625 3.70343C5.88783 3.70343 6.14808 3.75523 6.39088 3.85588C6.63368 3.95653 6.85425 4.10405 7.04 4.29L7.1 4.35C7.33568 4.58054 7.63502 4.73519 7.95941 4.794C8.28381 4.85282 8.61838 4.81312 8.92 4.68H9C9.29577 4.55324 9.54802 4.34276 9.72569 4.07447C9.90337 3.80618 9.99872 3.49179 10 3.17V3C10 2.46957 10.2107 1.96086 10.5858 1.58579C10.9609 1.21071 11.4696 1 12 1C12.5304 1 13.0391 1.21071 13.4142 1.58579C13.7893 1.96086 14 2.46957 14 3V3.09C14.0013 3.41179 14.0966 3.72618 14.2743 3.99447C14.452 4.26276 14.7042 4.47324 15 4.6C15.3016 4.73312 15.6362 4.77282 15.9606 4.714C16.285 4.65519 16.5843 4.50054 16.82 4.27L16.88 4.21C17.0657 4.02405 17.2863 3.87653 17.5291 3.77588C17.7719 3.67523 18.0322 3.62343 18.295 3.62343C18.5578 3.62343 18.8181 3.67523 19.0609 3.77588C19.3037 3.87653 19.5243 4.02405 19.71 4.21C19.896 4.39575 20.0435 4.61632 20.1441 4.85912C20.2448 5.10192 20.2966 5.36217 20.2966 5.625C20.2966 5.88783 20.2448 6.14808 20.1441 6.39088C20.0435 6.63368 19.896 6.85425 19.71 7.04L19.65 7.1C19.4195 7.33568 19.2648 7.63502 19.206 7.95941C19.1472 8.28381 19.1869 8.61838 19.32 8.92V9C19.4468 9.29577 19.6572 9.54802 19.9255 9.72569C20.1938 9.90337 20.5082 9.99872 20.83 10H21C21.5304 10 22.0391 10.2107 22.4142 10.5858C22.7893 10.9609 23 11.4696 23 12C23 12.5304 22.7893 13.0391 22.4142 13.4142C22.0391 13.7893 21.5304 14 21 14H20.91C20.5882 14.0013 20.2738 14.0966 20.0055 14.2743C19.7372 14.452 19.5268 14.7042 19.4 15Z" stroke="#24303f" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    <h1 style="font-size: 1.25rem;">Device Information</h1>
                </div>
                <div style="display: grid; gap: 12px;">
                    <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px; background-color: #ffffff; border-radius: 6px; border: 1px solid #e5e7eb;">
                        <span style="color: #6b7280; font-size: 0.875rem;">Device ID</span>
                        <span id="device-id" style="color: #24303f; font-size: 0.875rem; font-weight: 500;"></span>
                    </div>
                    <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px; background-color: #ffffff; border-radius: 6px; border: 1px solid #e5e7eb;">
                        <span style="color: #6b7280; font-size: 0.875rem;">MAC Address</span>
                        <span id="mac-address" style="color: #24303f; font-size: 0.875rem; font-weight: 500;"></span>
                    </div>
                    <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px; background-color: #ffffff; border-radius: 6px; border: 1px solid #e5e7eb;">
                        <span style="color: #6b7280; font-size: 0.875rem;">Secret Key</span>
                        <span id="secret-key" style="color: #24303f; font-size: 0.875rem; font-weight: 500;"></span>
                    </div>
                </div>
            </div>
        </div>
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

        // Hàm hiển thị loading overlay
        function showLoadingOverlay(message) {
            var overlay = document.getElementById('loading-overlay');
            var overlayText = document.querySelector('.loading-overlay-text');
            
            if (message) {
                overlayText.textContent = message;
            }
            
            overlay.classList.add('show');
        }
        
        // Hàm ẩn loading overlay
        function hideLoadingOverlay() {
            var overlay = document.getElementById('loading-overlay');
            overlay.classList.remove('show');
        }

        // Hàm thực hiện khi trang web được tải
        window.onload = function() {
            // Hiển thị loading overlay khi bắt đầu tải trang
            showLoadingOverlay("Initializing device and checking connection status...");
            
            // Bắt đầu kiểm tra xác thực
            checkInitialVerification();
            
            // Thêm sự kiện click cho nút scan
            document.getElementById('scan-button').addEventListener('click', function() {
                scanWifi();
            });
        };

        // Tạo đối tượng XMLHttpRequest để gửi yêu cầu đến máy chủ
        var xhr = new XMLHttpRequest();

        // Hàm kiểm tra trạng thái xác thực ban đầu
        function checkInitialVerification() {
            // Hiển thị trạng thái kiểm tra xác thực
            document.getElementById('verification-status').classList.remove('hidden');
            document.getElementById('main-content').style.display = 'none';
            
            var initVerifyXhr = new XMLHttpRequest();
            initVerifyXhr.onreadystatechange = function() {
                if (initVerifyXhr.readyState == 4 && initVerifyXhr.status == 200) {
                    // Ẩn loading overlay khi đã hoàn tất kiểm tra
                    hideLoadingOverlay();
                    
                    // Đã nhận phản hồi - ẩn trạng thái kiểm tra
                    document.getElementById('verification-status').classList.add('hidden');
                    
                    try {
                        var data = JSON.parse(initVerifyXhr.responseText);
                        // Kiểm tra có userId và userId không rỗng
                        if (data.userId && data.userId.trim() !== "") {
                            // Hiển thị welcome message và cho phép truy cập
                            document.getElementById('welcome-userId').textContent = data.userId;
                            document.getElementById('welcome-message').style.display = 'block';
                            document.getElementById('main-content').style.display = 'block';
                            document.getElementById('auth-container').style.display = 'none';
                            
                            // Ẩn header status vì đã xác thực thành công
                            document.getElementById('header-status').style.display = 'none';
                            
                            // Bắt đầu quét WiFi sau khi xác thực thành công
                            setTimeout(scanWifi, 300);
                        } else {
                            // userId không tồn tại hoặc rỗng, hiển thị thông báo chặn truy cập
                            document.getElementById('welcome-message').style.display = 'none';
                            document.getElementById('main-content').style.display = 'none';
                            document.getElementById('auth-container').style.display = 'block';
                            
                            // Bắt đầu quét WiFi sau khi hiển thị giao diện
                            setTimeout(scanWifi, 300);
                        }
                    } catch (e) {
                        console.error("Error checking initial verification status:", e);
                        // Hiển thị giao diện chặn truy cập trong trường hợp lỗi
                        document.getElementById('main-content').style.display = 'none';
                        document.getElementById('auth-container').style.display = 'block';
                        
                        // Bắt đầu quét WiFi sau khi hiển thị giao diện
                        setTimeout(scanWifi, 300);
                    }
                }
            };
            
            initVerifyXhr.open("GET", "/checkVerification", true);
            initVerifyXhr.send();
        }

        // Hàm quét mạng WiFi
        function scanWifi() {
            // Hiển thị trạng thái quét
            updateStatus("Scanning WiFi...", "info");
            
            // Thay đổi nút scan
            var scanIcon = document.getElementById('scan-icon');
            var scanText = document.getElementById('scan-text');
            
            scanIcon.innerHTML = '<div class="scan-animation"></div>';
            scanText.textContent = 'Scanning...';
            
            // Vô hiệu hóa nút scan trong quá trình quét
            document.getElementById('scan-button').disabled = true;
            
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    // Kết thúc quét - phục hồi nút scan
                    scanIcon.innerHTML = '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M4 4V9H4.58152M19.9381 11C19.446 7.05369 16.0796 4 12 4C8.64262 4 5.76829 6.06817 4.58152 9M4.58152 9H9M20 20V15H19.4185M19.4185 15C18.2317 17.9318 15.3574 20 12 20C7.92038 20 4.55399 16.9463 4.06189 13M19.4185 15H15" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>';
                    scanText.textContent = 'Scan again';
                    document.getElementById('scan-button').disabled = false;
                    
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
            
            // Hiển thị trạng thái trên header
            showHeaderStatus("Connecting to WiFi and getting device information...");
            
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        updateStatus("WiFi config saved successfully!", "success");
                        document.getElementById("device-info").style.display = "block";
                        document.getElementById("loading-status").style.display = "flex";
                        
                        setTimeout(getDeviceInfo, 2000);
                    } else {
                        updateStatus("Error saving config", "error");
                    }
                }
            };
            
            xhr.open("GET", "/saveWifi?ssid=" + encodeURIComponent(ssid) + "&pass=" + encodeURIComponent(password), true);
            xhr.send();
        }

        // Hàm lấy thông tin thiết bị
        function getDeviceInfo() {
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4 && xhr.status == 200) {
                    try {
                        var data = JSON.parse(xhr.responseText);
                        document.getElementById("device-id").textContent = data.deviceId;
                        document.getElementById("mac-address").textContent = data.macAddress;
                        document.getElementById("secret-key").textContent = data.secretKey;
                        document.getElementById("loading-status").style.display = "none";
                        updateStatus("Device information loaded successfully", "success");
                        
                        // Cập nhật trạng thái header và bắt đầu kiểm tra xác thực
                        showHeaderStatus("Waiting for device registration...");
                        
                        // Bắt đầu kiểm tra trạng thái xác thực
                        setTimeout(checkVerificationStatus, 1000);
                    } catch (e) {
                        console.error("Error parsing device info:", e);
                        updateStatus("Error getting device info", "error");
                    }
                }
            };
            
            xhr.open("GET", "/getDeviceInfo", true);
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

        // Cập nhật hàm checkVerificationStatus
        function checkVerificationStatus() {
            var verifyXhr = new XMLHttpRequest();
            verifyXhr.onreadystatechange = function() {
                if (verifyXhr.readyState == 4 && verifyXhr.status == 200) {
                    try {
                        var data = JSON.parse(verifyXhr.responseText);
                        // Kiểm tra có userId và userId không rỗng
                        if (data.userId && data.userId.trim() !== "") {
                            // Cập nhật UI khi có userId
                            var headerStatus = document.getElementById('header-status');
                            headerStatus.classList.add('success', 'show');
                            document.querySelector('#header-status .status-text').textContent = 
                                "Device registered with User ID: " + data.userId;
                            
                            // Sau 3 giây, ẩn header status và hiển thị welcome message
                            setTimeout(function() {
                                headerStatus.classList.remove('show');
                                setTimeout(function() {
                                    document.getElementById('welcome-userId').textContent = data.userId;
                                    document.getElementById('welcome-message').style.display = 'block';
                                    document.getElementById('main-content').style.display = 'block';
                                    document.getElementById('auth-container').style.display = 'none';
                                }, 300);
                            }, 3000);
                        } else {
                            // userId không tồn tại hoặc rỗng, giữ trạng thái chặn truy cập
                            document.getElementById('main-content').style.display = 'none';
                            document.getElementById('auth-container').style.display = 'block';
                        }
                    } catch (e) {
                        console.error("Error checking verification status:", e);
                    }
                }
            };
            
            verifyXhr.open("GET", "/checkVerification", true);
            verifyXhr.send();
        }
        
        // Kiểm tra trạng thái xác thực mỗi 2 giây
        setInterval(checkVerificationStatus, 2000);

        // Hàm hiển thị trạng thái trên header
        function showHeaderStatus(message) {
            var headerStatus = document.getElementById('header-status');
            document.querySelector('#header-status .status-text').textContent = message || "Waiting for create device from Server";
            headerStatus.classList.add('show');
            headerStatus.classList.remove('success');
        }
    </script>
</body>
</html>
)html";

void getDeviceInfoValues(const char* &deviceIdPtr, const char* &macAddressPtr, const char* &userIdPtr) {
    deviceIdPtr = deviceId.c_str();
    macAddressPtr = macAddress.c_str();
    userIdPtr = userId.c_str();
}

void generateDeviceInfo() {
    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);
    deviceId = "ESP32-" + String(macAddr[4], HEX) + String(macAddr[5], HEX);
    deviceId.toUpperCase();

    macAddress = WiFi.macAddress();

    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    secretKey = "";
    for(int i = 0; i < 9; i++) {
        secretKey += chars[random(0, strlen(chars))];
    }
    
    EEPROM.writeString(64, deviceId);
    EEPROM.writeString(96, macAddress);
    EEPROM.writeString(128, secretKey);
    EEPROM.commit();
    
    Serial.println("Generated device info:");
    Serial.println("Device ID: " + deviceId);
    Serial.println("MAC Address: " + macAddress);
    Serial.println("Secret Key: " + secretKey);
}

void createAccessPoint(){
    Serial.println("ESP32 wifi network created!");
    WiFi.mode(WIFI_AP);
    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);

    String ssid_ap = "ESP32-" + String(macAddr[4], HEX) + String(macAddr[5], HEX);
    String password_ap = "12345678";
    ssid_ap.toUpperCase();
    WiFi.softAP(ssid_ap.c_str(), password_ap.c_str());

    Serial.println("Access point name: " + ssid_ap);
    Serial.println("Web server access address: " + WiFi.softAPIP().toString());

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
            
            ssid = ssid_temp;
            password = password_temp;

            WiFi.begin(ssid.c_str(), password.c_str());
            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
                delay(500);
                Serial.print(".");
            }
            Serial.println();

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                isWifiConnected = true;

                String topic = "connect/" + macAddress + "/" + deviceId;

                if(connectToAWSIoTCore()) {
                    Serial.println("Connected to AWS IoT!");
                    if(subscribeTopic(topic.c_str())) {
                        Serial.println("Subscribed to topic: " + topic);
                    } else {
                        Serial.println("Cannot subscribe to topic: " + topic);
                    }
                } else {
                    Serial.println("Cannot connect to AWS IoT!");
                }

                webServer.sendHeader("Connection", "close");
                webServer.send(200, "text/plain", "WiFi saved and connected successfully!");
            } else {
                Serial.println("Cannot connect to WiFi");
                isWifiConnected = false;
                webServer.sendHeader("Connection", "close");
                webServer.send(400, "text/plain", "WiFi saved but failed to connect.");
            }
        } else {
            webServer.sendHeader("Connection", "close"); 
            webServer.send(400, "text/plain", "Invalid WiFi name!");
        }
    });
    
    webServer.on("/getDeviceInfo", HTTP_GET, []() {
        DynamicJsonDocument doc(200);
        doc["deviceId"] = deviceId;
        doc["macAddress"] = macAddress;
        doc["secretKey"] = secretKey;
        
        String response;
        serializeJson(doc, response);
        
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "application/json", response);
    });
    
    webServer.on("/reStart", HTTP_GET, []() {
        Serial.println("Received request to restart device");
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/plain", "Device is restarting...");
        delay(1000);
        ESP.restart();
    });
    
    webServer.on("/checkVerification", HTTP_GET, []() {
        extern bool isDeviceVerified();
        bool verified = isDeviceVerified();
        
        DynamicJsonDocument doc(256);
        doc["verified"] = verified;
        doc["userId"] = userId;
        
        String response;
        serializeJson(doc, response);
        
        Serial.println("Check verification: userId=" + userId);
        
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "application/json", response);
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

    EEPROM.begin(200);
    Serial.println("EEPROM initialized");

    char deviceId_temp[10], macAddress_temp[18], secretKey_temp[10], userId_temp[37];
    char ssid_temp[32], password_temp[64];
    
    EEPROM.readString(0, ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32, password_temp, sizeof(password_temp));
    ssid = String(ssid_temp);
    password = String(password_temp);
    
    
    EEPROM.readString(64, deviceId_temp, sizeof(deviceId_temp));
    EEPROM.readString(96, macAddress_temp, sizeof(macAddress_temp));
    EEPROM.readString(128, secretKey_temp, sizeof(secretKey_temp));
    EEPROM.readString(160, userId_temp, sizeof(userId_temp));
    
    deviceId = String(deviceId_temp);
    macAddress = String(macAddress_temp);
    secretKey = String(secretKey_temp);
    userId = String(userId_temp);
    
    if (deviceId.length() != 9 || secretKey.length() != 9) {
        generateDeviceInfo();
    }
    
    if (ssid != "") {
        Serial.println("-----------------------------------");
        Serial.println("Reading information from EEPROM:");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);
        Serial.print("Device ID: ");
        Serial.println(deviceId);
        Serial.print("MAC Address: ");
        Serial.println(macAddress);
        Serial.print("Secret Key: ");
        Serial.println(secretKey);
        Serial.println("-----------------------------------");
        Serial.print("User ID: ");
        Serial.println(userId);
    } else {
        Serial.println("No information found in EEPROM");
    }

    setupWifi();
    
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
    
    if (isWifiConnected) {
        clientLoop();
    }
}

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

void saveUserId(String id) {
    Serial.println("Saving User ID to EEPROM: " + id);
    EEPROM.writeString(160, id);
    EEPROM.commit();
}

