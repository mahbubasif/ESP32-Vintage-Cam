#include <SPI.h>
#include <TFT_eSPI.h>
#include "esp_camera.h"
#include "FS.h"                
#include "SD_MMC.h"    
#include "img_converters.h"         

TFT_eSPI tft = TFT_eSPI(); 

// Button on GPIO 12
#define CAPTURE_PIN 12 

// --- CAMERA PINS ---
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

bool sdCardEnabled = false;
int buttonState = 0;
int lastButtonState = HIGH; 

void setup() {
  delay(1000);

  // 1. Start SD Card FIRST
  if(!SD_MMC.begin("/sdcard", true)){ 
    sdCardEnabled = false;
  } else {
    sdCardEnabled = true;
  }

  // 2. Start Display
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(false); 
  tft.setTextSize(2);
  
  if (sdCardEnabled) {
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.drawString("SYSTEM READY", 40, 110);
  } else {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("NO SD CARD", 50, 110);
  }
  delay(1000);

  // 3. Setup Button
  pinMode(CAPTURE_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(CAPTURE_PIN); 

  // 4. Configure Camera (STAY IN RGB MODE FOREVER)
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
  config.pixel_format = PIXFORMAT_RGB565; // Always RGB for Display
  config.frame_size = FRAMESIZE_240X240;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    tft.fillScreen(TFT_RED);
    tft.drawString("Cam Fail", 50, 110);
    return;
  }
}

void loop() {
  // A. Get RGB Frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  // Show Preview
  tft.pushImage(0, 0, 240, 240, (uint16_t *)fb->buf);

  // B. Button Logic
  buttonState = digitalRead(CAPTURE_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    
    // 1. Show Visual Feedback
    tft.fillCircle(120, 120, 10, TFT_WHITE); 
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("CONVERTING...", 50, 20);

    if (!sdCardEnabled) {
       tft.drawString("NO CARD!", 70, 50);
       delay(500);
    } else {
      
      // 2. SOFTWARE CONVERSION (The Fix)
      uint8_t * out_jpg = NULL; // Pointer for the new JPEG
      size_t out_len = 0;       // Size of the new JPEG

      // Convert the current RGB framebuffer (fb) to JPEG
      // Quality: 0-100 (80 is good)
      bool converted = frame2jpg(fb, 80, &out_jpg, &out_len);

      if(converted) {
        // Save the CONVERTED data, not the raw fb!
        String path = "/IMG_" + String(millis()) + ".jpg";
        fs::File file = SD_MMC.open(path.c_str(), FILE_WRITE);
        
        if(file){
          file.write(out_jpg, out_len); // Write the JPEG buffer
          file.close();
          tft.drawString("SAVED!     ", 70, 20);
        } else {
          tft.drawString("ERR Write", 70, 20);
        }

        // Critical: Free the temporary memory used by the converter
        free(out_jpg); 
      } else {
        tft.drawString("CONVERT FAIL", 40, 20);
      }
      
      delay(1000); 
    }
  } 

  // Release the RGB frame back to the camera
  esp_camera_fb_return(fb); 
  
  lastButtonState = buttonState;
}
