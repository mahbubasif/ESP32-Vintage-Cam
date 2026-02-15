#include <Arduino.h>

// --- FIX 1: LOAD WIFI & SERVER LIBRARIES FIRST ---
// This prevents TFT_eSPI from breaking the WebServer definitions
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// --- FIX 2: LOAD FILESYSTEMS NEXT ---
#include "FS.h"                
#include "SD_MMC.h"    

// --- FIX 3: LOAD DISPLAY & CAMERA LAST ---
#include <SPI.h>
#include <TFT_eSPI.h>
#include "esp_camera.h"
#include "img_converters.h" 

// --- FIX 4: DEFINE NAMESPACES ---
// This tells the compiler that "File" means "fs::File"
using namespace fs; 

TFT_eSPI tft = TFT_eSPI(); 
WebServer server(80);

// --- PINS ---
#define CAPTURE_PIN 12 

// Camera Pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- GLOBALS ---
bool wifiMode = false;
bool sdCardEnabled = false;
unsigned long buttonPressStartTime = 0;
bool buttonActive = false;

// --- WEB SERVER FUNCTIONS ---

// 1. List all files on SD Card
void handleRoot() {
  String html = "<html><head><title>ESP32 Gallery</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif; background:#222; color:#fff} a{display:block; padding:10px; background:#444; color:#fff; text-decoration:none; margin:5px; border-radius:5px} h1{text-align:center}</style>";
  html += "</head><body><h1>My Photos</h1>";

  // Use fs::File explicitly (or just File since we added 'using namespace fs')
  File root = SD_MMC.open("/");
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory()){
      String fileName = file.name();
      // Only show JPG files
      if(fileName.endsWith(".jpg") || fileName.endsWith(".JPG")) {
         html += "<a href='/download?file=/" + fileName + "'>" + fileName + " (" + String(file.size()/1024) + " KB)</a>";
      }
    }
    file = root.openNextFile();
  }
  html += "<br><p style='text-align:center; color:#aaa'>Press RESET on board to take more photos.</p></body></html>";
  server.send(200, "text/html", html);
}

// 2. Download a specific file
void handleDownload() {
  if (server.hasArg("file")) {
    String fileName = server.arg("file");
    if (SD_MMC.exists(fileName)) {
      File file = SD_MMC.open(fileName, FILE_READ);
      server.streamFile(file, "image/jpeg");
      file.close();
    } else {
      server.send(404, "text/plain", "File Not Found");
    }
  }
}

// --- SETUP ---
void setup() {
  delay(1000);

  // 1. Init SD Card (First Priority)
  if(!SD_MMC.begin("/sdcard", true)){ 
    sdCardEnabled = false;
  } else {
    sdCardEnabled = true;
  }

  // 2. Init Display
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(false); 
  tft.setTextSize(2);

  // 3. Init Button
  pinMode(CAPTURE_PIN, INPUT_PULLUP);

  // 4. Init Camera
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; 
  config.frame_size = FRAMESIZE_240X240;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    tft.fillScreen(TFT_RED);
    tft.drawString("Cam Fail", 50, 110);
    return;
  }

  if (sdCardEnabled) {
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.drawString("READY!", 80, 110);
    delay(1000);
  } else {
    tft.fillScreen(TFT_RED);
    tft.drawString("NO SD", 80, 110);
    delay(2000);
  }
}

// --- SWITCH TO WIFI MODE ---
void enableWiFiMode() {
  wifiMode = true;
  
  // 1. Stop displaying camera feed
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("WIFI MODE", 60, 80);
  tft.setTextSize(1);
  tft.drawString("SSID: ESP32-Gallery", 30, 120);
  tft.drawString("IP: 192.168.4.1", 40, 140);
  
  // 2. Start Access Point
  WiFi.softAP("ESP32-Gallery", ""); // No password
  
  // 3. Start Web Server
  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.begin();
}

void loop() {
  // === MODE 1: WIFI GALLERY ===
  if (wifiMode) {
    server.handleClient();
    delay(2); 
    return; 
  }

  // === MODE 2: CAMERA PREVIEW ===
  
  // 1. Show Preview
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    tft.pushImage(0, 0, 240, 240, (uint16_t *)fb->buf);
    
    // Check Button for Actions
    if (digitalRead(CAPTURE_PIN) == LOW) {
      if (!buttonActive) {
        buttonActive = true;
        buttonPressStartTime = millis(); // Start timing
      }
      
      // Visual feedback for Long Press
      if (millis() - buttonPressStartTime > 3000) {
        tft.fillCircle(120, 120, 20, TFT_BLUE); // Blue dot indicates WiFi mode coming
      }

    } else {
      // Button Released? Check how long it was held
      if (buttonActive) {
        unsigned long duration = millis() - buttonPressStartTime;
        buttonActive = false;

        // Release frame before doing work
        esp_camera_fb_return(fb); 
        fb = NULL;

        if (duration > 3000) {
          // --- LONG PRESS: ENABLE WIFI ---
          enableWiFiMode();
          return; // Exit loop, never return to camera
        } 
        else if (duration > 50) { 
          // --- SHORT PRESS: TAKE PHOTO ---
          if (!sdCardEnabled) {
             tft.drawString("NO CARD", 70, 20);
             delay(1000);
          } else {
            tft.fillCircle(120, 120, 10, TFT_WHITE);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString("SAVING...", 70, 20);

            // Re-acquire frame for saving (clean slate)
            fb = esp_camera_fb_get();
            
            // CONVERT TO JPEG
            uint8_t * out_jpg = NULL;
            size_t out_len = 0;
            bool converted = frame2jpg(fb, 80, &out_jpg, &out_len);

            if(converted) {
              String path = "/IMG_" + String(millis()) + ".jpg";
              File file = SD_MMC.open(path.c_str(), FILE_WRITE);
              if(file){
                file.write(out_jpg, out_len);
                file.close();
                tft.drawString("SAVED!     ", 70, 20);
              }
              free(out_jpg); // Free memory!
            }
            if(fb) {
              esp_camera_fb_return(fb);
              fb = NULL;
            }
            delay(1000);
          }
        }
      }
    }
  }
  
  // Always return frame if we didn't use it
  if(fb) esp_camera_fb_return(fb);
}
