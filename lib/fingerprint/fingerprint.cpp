#include "fingerprint.h"

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void fingerprintInit() {
    mySerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("Connecting to fingerprint sensor...");
    finger.begin(BAUD_RATE);
    if (finger.verifyPassword()) {
        Serial.println("Connected successfully!");
        
        uint8_t p = finger.getTemplateCount();
        if (p == FINGERPRINT_OK) {
            Serial.print("Sensor contains "); 
            Serial.print(finger.templateCount); 
            Serial.println(" templates");
            
            Serial.print("Fingerprint capacity: ");
            Serial.print(finger.capacity);
            Serial.println(" templates");
        }
    } else {
        Serial.println("Fingerprint sensor not found :(");
    }
}

uint8_t getFingerprintEnroll(int id, DisplayResultCallback displayResultCallback) {

  if (id < 1 || id > 127) {
    Serial.printf("Invalid ID range: %d (must be between 1-127)\n", id);
    displayResultCallback("Invalid ID range!", TFT_RED);
    return FINGERPRINT_BADLOCATION;
  }
  
  if (!isFingerIDFree(id)) {
    Serial.printf("ID %d already in use\n", id);
    displayResultCallback("ID already used!", TFT_RED);
    return FINGERPRINT_BADLOCATION;
  }

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); 
  Serial.println(id);
  
  char message[50];
  snprintf(message, sizeof(message), "Enroll ID #%d", id);
  displayResultCallback(message, TFT_BLUE);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      displayResultCallback("Image taken", TFT_CYAN);
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      displayResultCallback("Communication error", TFT_RED);
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      displayResultCallback("Imaging error", TFT_RED);
      break;
    default:
      Serial.println("Unknown error");
      displayResultCallback("Unknown error", TFT_RED);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      displayResultCallback("Image too messy", TFT_RED);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      displayResultCallback("Communication error", TFT_RED);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      displayResultCallback("No features found", TFT_RED);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      displayResultCallback("Invalid image", TFT_RED);
      return p;
    default:
      Serial.println("Unknown error");
      displayResultCallback("Unknown error", TFT_RED);
      return p;
  }

  Serial.println("Remove finger");
  displayResultCallback("Remove finger", TFT_CYAN);
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  displayResultCallback("Place same finger again", TFT_BLUE);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      displayResultCallback("Image taken", TFT_CYAN);
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      displayResultCallback("Communication error", TFT_RED);
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      displayResultCallback("Imaging error", TFT_RED);
      break;
    default:
      Serial.println("Unknown error");
      displayResultCallback("Unknown error", TFT_RED);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      displayResultCallback("Image too messy", TFT_RED);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      displayResultCallback("Communication error", TFT_RED);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      displayResultCallback("No features found", TFT_RED);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      displayResultCallback("Invalid image", TFT_RED);
      return p;
    default:
      Serial.println("Unknown error");
      displayResultCallback("Unknown error", TFT_RED);
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  displayResultCallback("Creating model...", TFT_CYAN);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
    displayResultCallback("Prints matched!", TFT_GREEN);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    displayResultCallback("Communication error", TFT_RED);
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    displayResultCallback("Fingerprints did not match", TFT_RED);
    return p;
  } else {
    Serial.println("Unknown error");
    displayResultCallback("Unknown error", TFT_RED);
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  displayResultCallback("Storing model...", TFT_CYAN);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    snprintf(message, sizeof(message), "ID #%d stored!", id);
    displayResultCallback(message, TFT_GREEN);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    displayResultCallback("Communication error", TFT_RED);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    displayResultCallback("Invalid location (ID)", TFT_RED);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    displayResultCallback("Flash memory error", TFT_RED);
    return p;
  } else {
    Serial.printf("Unknown error: %d\n", p);
    displayResultCallback("Unknown error", TFT_RED);
    return p;
  }

  return true;
}

int getFingerprintId() {
    Serial.println("Place your finger on the sensor...");
    if (finger.getImage() == FINGERPRINT_OK) {
        if (finger.image2Tz() == FINGERPRINT_OK) {
            if (finger.fingerFastSearch() == FINGERPRINT_OK) {
                Serial.print("Fingerprint found! ID: ");
                Serial.println(finger.fingerID);
                return finger.fingerID;
            } else {
                Serial.println("Database not found");
            }
        }
    }
    return -1;
}

bool unlockWithFingerprint(DisplayResultCallback displayResultCallback) {

    if (isSystemLockedOut()) {
        displayResultCallback("System Locked!", TFT_RED);
        Serial.println("System locked out!");
        return false;
    }

    Serial.println("Waiting for finger...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 10000;
    uint8_t p = FINGERPRINT_NOFINGER;
    
    while ((p != FINGERPRINT_OK) && (millis() - startTime < timeout)) {
        p = finger.getImage();
        
        if (p == FINGERPRINT_OK) {
            displayResultCallback("Fingerprint detected!", TFT_BLUE);
            Serial.println("Fingerprint detected!");
        }
    }
    
    if (p != FINGERPRINT_OK) {
        return false;
    }
    
    displayResultCallback("Processing...", TFT_CYAN);
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        displayResultCallback("Image error", TFT_RED);
        incrementFailedAttempt();
        return false;
    }
    
    displayResultCallback("Searching...", TFT_CYAN);
    p = finger.fingerFastSearch();
    
    if (p == FINGERPRINT_OK) {
        char message[50];
        snprintf(message, sizeof(message), "Welcome ID #%d!", finger.fingerID);
        displayResultCallback(message, TFT_GREEN);
        
        Serial.print("Fingerprint matched! ID #");
        Serial.println(finger.fingerID);
        Serial.print("Confidence score: ");
        Serial.println(finger.confidence);
        
        lockOpen();
        resetFailedAttempts();
        return true;
    } else {
        displayResultCallback("Access Denied!", TFT_RED);
        Serial.println("Fingerprint not verified!");
        delay(1000);
        isNormalMode = true;
        incrementFailedAttempt();
        return false;
    }
}

bool deleteAllFingerprints(DisplayResultCallback displayResultCallback) {
    displayResultCallback("Deleting all prints...", TFT_CYAN);
    
    uint8_t p = finger.emptyDatabase();
    if (p == FINGERPRINT_OK) {
        Serial.println("All fingerprints deleted successfully!");
        displayResultCallback("All prints deleted!", TFT_GREEN);
        return true;
    } else {
        Serial.print("Failed to delete all fingerprints, error code: ");
        Serial.println(p);
        displayResultCallback("Delete failed!", TFT_RED);
        return false;
    }
}

bool deleteFingerprint(uint8_t id, DisplayResultCallback displayResultCallback) {
    char message[50];
    snprintf(message, sizeof(message), "Deleting ID #%d...", id);
    displayResultCallback(message, TFT_CYAN);
    
    uint8_t p = finger.deleteModel(id);
    
    if (p == FINGERPRINT_OK) {
        snprintf(message, sizeof(message), "ID #%d deleted!", id);
        Serial.printf("Deleted fingerprint ID #%d successfully!\n", id);
        displayResultCallback(message, TFT_GREEN);
        return true;
    } else {
        Serial.print("Failed to delete fingerprint, error code: ");
        Serial.println(p);
        switch (p) {
            case FINGERPRINT_PACKETRECIEVEERR:
                displayResultCallback("Communication error", TFT_RED);
                break;
            case FINGERPRINT_BADLOCATION:
                displayResultCallback("Invalid ID", TFT_RED);
                break;
            case FINGERPRINT_FLASHERR:
                displayResultCallback("Flash memory error", TFT_RED);
                break;
            default:
                displayResultCallback("Delete failed!", TFT_RED);
                break;
        }
        return false;
    }
}

bool isFingerIDFree(uint8_t id) {
    uint8_t p = finger.loadModel(id);
    return p != FINGERPRINT_OK;
}

int getNextFreeID() {
    for (int i = 1; i <= 127; i++) {
        if (isFingerIDFree(i)) {
            return i;
        }
    }
    return -1;
}

String getLatestFingerprintTemplateAsBase64(uint8_t fingerprintID) {
    char buffer[100];
    sprintf(buffer, "{\"fingerprintID\":%d,\"timestamp\":\"%lld\"}", 
            fingerprintID, 
            (long long)millis());
    
    // Encode this data to base64
    String base64Template = base64::encode((uint8_t*)buffer, strlen(buffer));
    Serial.printf("Fingerprint metadata: %s\n", buffer);
    Serial.printf("Base64 encoded: %s\n", base64Template.c_str());
    
    return base64Template;
}


