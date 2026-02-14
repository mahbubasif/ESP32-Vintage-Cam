#include "Arduino.h"
#include "FS.h"                // File System wrapper
#include "SD_MMC.h"            // Native ESP32 SD Card library

void setup() {
  // 1. Start Serial
  Serial.begin(115200);
  // Give the serial port a moment to stabilize
  delay(1000); 

  Serial.println("\n\n-----------------------------------");
  Serial.println("ESP32-CAM SD Card Test Starting...");
  Serial.println("-----------------------------------");

  // 2. Mount SD Card
  // "/sdcard" is the mount point
  // "true" enables 1-bit mode (Uses pins 2, 14, 15)
  // "false" would try 4-bit mode (Uses pins 2, 4, 12, 13, 14, 15)
  if(!SD_MMC.begin("/sdcard", true)){
    Serial.println("❌ ERROR: Card Mount Failed");
    Serial.println("Possible causes:");
    Serial.println("  - Card not inserted");
    Serial.println("  - Card format is not FAT32");
    Serial.println("  - Loose connection in slot");
    Serial.println("  - Pull-up resistor missing on GPIO 13 (rare)");
    return;
  }

  // 3. Print Card Info
  uint8_t cardType = SD_MMC.cardType();

  if(cardType == CARD_NONE){
    Serial.println("❌ ERROR: No Card Attached");
    return;
  }

  Serial.print("✅ SUCCESS: SD Card Detected! Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("📦 Card Size: %llu MB\n", cardSize);
  
  Serial.printf("💾 Total Space: %llu MB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("📂 Used Space: %llu MB\n", SD_MMC.usedBytes() / (1024 * 1024));
  
  Serial.println("-----------------------------------");
}

void loop() {
  // Do nothing
}
