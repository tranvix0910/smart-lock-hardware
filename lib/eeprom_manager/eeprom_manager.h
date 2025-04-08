#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Định nghĩa kích thước các phân vùng
#define CONFIG_PARTITION_SIZE 200
#define RFID_CARD_SIZE 7
#define MAX_RFID_CARDS 30
#define RFID_PARTITION_SIZE (RFID_CARD_SIZE * MAX_RFID_CARDS)
#define TOTAL_EEPROM_SIZE (CONFIG_PARTITION_SIZE + RFID_PARTITION_SIZE + 30) // Thêm 30 bytes dự phòng

// Định nghĩa địa chỉ các thành phần trong phân vùng cấu hình
#define SSID_ADDR 0
#define SSID_SIZE 32
#define PASSWORD_ADDR 32
#define PASSWORD_SIZE 32
#define DEVICE_ID_ADDR 64
#define DEVICE_ID_SIZE 10
#define MAC_ADDR 96
#define MAC_SIZE 18
#define SECRET_KEY_ADDR 128
#define SECRET_KEY_SIZE 10
#define USER_ID_ADDR 160
#define USER_ID_SIZE 37

// Định nghĩa địa chỉ bắt đầu của phân vùng RFID
#define RFID_START_ADDR 200

// Định nghĩa giá trị đặc biệt cho ô trống RFID
#define RFID_EMPTY_SLOT 0xFF

class EEPROMManager {
public:
    static bool begin();
    
    // Các hàm quản lý cấu hình
    static bool writeConfig(const char* data, uint16_t address, uint16_t size, bool autoCommit = false);
    static bool readConfig(char* data, uint16_t address, uint16_t size);
    static void clearConfig(bool autoCommit = false);
    
    // Các hàm quản lý RFID
    static bool addRFIDCard(uint8_t* uid, uint8_t uidLength, bool autoCommit = false);
    static bool deleteRFIDCard(uint8_t* uid, uint8_t uidLength, bool autoCommit = false);
    static bool findEmptyRFIDSlot(uint16_t& slotAddr);
    static bool isRFIDCardExists(uint8_t* uid, uint8_t uidLength);
    static void clearAllRFIDCards(bool autoCommit = false);
    
    // Hàm tiện ích
    static void commitChanges();
    static bool isValidAddress(uint16_t address, uint16_t size);
    
    // Hàm mới để quản lý batch operations
    static void beginBatchWrite();
    static void endBatchWrite();
    
private:
    static bool _batchWriteMode;
};

#endif // EEPROM_MANAGER_H 