#include "eeprom_manager.h"

bool EEPROMManager::_batchWriteMode = false;

bool EEPROMManager::begin() {
    if (!EEPROM.begin(TOTAL_EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return false;
    }
    Serial.println("EEPROM initialized successfully");
    Serial.print("Total EEPROM size: ");
    Serial.println(TOTAL_EEPROM_SIZE);
    return true;
}

void EEPROMManager::beginBatchWrite() {
    _batchWriteMode = true;
}

void EEPROMManager::endBatchWrite() {
    _batchWriteMode = false;
    commitChanges();
}

bool EEPROMManager::isValidAddress(uint16_t address, uint16_t size) {
    return (address + size) <= TOTAL_EEPROM_SIZE;
}

bool EEPROMManager::writeConfig(const char* data, uint16_t address, uint16_t size, bool autoCommit) {
    if (!isValidAddress(address, size)) {
        Serial.println("Invalid address or size for writing config");
        return false;
    }

    if (!data) {
        Serial.println("Invalid data pointer");
        return false;
    }

    for (uint16_t i = 0; i < size; i++) {
        EEPROM.write(address + i, data[i]);
    }
    
    if (autoCommit && !_batchWriteMode) {
        commitChanges();
    }
    return true;
}

bool EEPROMManager::readConfig(char* data, uint16_t address, uint16_t size) {
    if (!isValidAddress(address, size)) {
        Serial.println("Invalid address or size for reading config");
        return false;
    }

    if (!data) {
        Serial.println("Invalid data pointer");
        return false;
    }

    for (uint16_t i = 0; i < size; i++) {
        data[i] = EEPROM.read(address + i);
    }
    
    return true;
}

void EEPROMManager::clearConfig(bool autoCommit) {
    for (uint16_t i = 0; i < CONFIG_PARTITION_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    if (autoCommit && !_batchWriteMode) {
        commitChanges();
    }
}

bool EEPROMManager::findEmptyRFIDSlot(uint16_t& slotAddr) {
    for (uint16_t i = 0; i < MAX_RFID_CARDS; i++) {
        uint16_t addr = RFID_START_ADDR + (i * RFID_CARD_SIZE);
        if (EEPROM.read(addr) == RFID_EMPTY_SLOT) {
            slotAddr = addr;
            return true;
        }
    }
    return false;
}

bool EEPROMManager::addRFIDCard(uint8_t* uid, uint8_t uidLength, bool autoCommit) {
    if (uidLength > RFID_CARD_SIZE) {
        Serial.println("UID too long");
        return false;
    }

    if (isRFIDCardExists(uid, uidLength)) {
        Serial.println("RFID card already exists");
        return false;
    }

    uint16_t emptySlot;
    if (!findEmptyRFIDSlot(emptySlot)) {
        Serial.println("No empty slots available");
        return false;
    }

    for (uint8_t i = 0; i < uidLength; i++) {
        EEPROM.write(emptySlot + i, uid[i]);
    }
    EEPROM.write(emptySlot + RFID_CARD_SIZE - 1, uidLength);
    
    if (autoCommit && !_batchWriteMode) {
        commitChanges();
    }
    return true;
}

bool EEPROMManager::deleteRFIDCard(uint8_t* uid, uint8_t uidLength, bool autoCommit) {
    for (uint16_t i = 0; i < MAX_RFID_CARDS; i++) {
        uint16_t addr = RFID_START_ADDR + (i * RFID_CARD_SIZE);
        bool match = true;
        
        uint8_t storedLength = EEPROM.read(addr + RFID_CARD_SIZE - 1);
        if (storedLength != uidLength) continue;
        
        for (uint8_t j = 0; j < uidLength; j++) {
            if (EEPROM.read(addr + j) != uid[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            for (uint8_t j = 0; j < RFID_CARD_SIZE; j++) {
                EEPROM.write(addr + j, RFID_EMPTY_SLOT);
            }
            if (autoCommit && !_batchWriteMode) {
                commitChanges();
            }
            return true;
        }
    }
    return false;
}

bool EEPROMManager::isRFIDCardExists(uint8_t* uid, uint8_t uidLength) {
    for (uint16_t i = 0; i < MAX_RFID_CARDS; i++) {
        uint16_t addr = RFID_START_ADDR + (i * RFID_CARD_SIZE);
        bool match = true;
        
        uint8_t storedLength = EEPROM.read(addr + RFID_CARD_SIZE - 1);
        if (storedLength != uidLength) continue;
        
        for (uint8_t j = 0; j < uidLength; j++) {
            if (EEPROM.read(addr + j) != uid[j]) {
                match = false;
                break;
            }
        }
        
        if (match) return true;
    }
    return false;
}

void EEPROMManager::clearAllRFIDCards(bool autoCommit) {
    for (uint16_t i = RFID_START_ADDR; i < RFID_START_ADDR + RFID_PARTITION_SIZE; i++) {
        EEPROM.write(i, RFID_EMPTY_SLOT);
    }
    if (autoCommit && !_batchWriteMode) {
        commitChanges();
    }
}

void EEPROMManager::commitChanges() {
    if (!EEPROM.commit()) {
        Serial.println("Failed to commit EEPROM changes");
    }
} 