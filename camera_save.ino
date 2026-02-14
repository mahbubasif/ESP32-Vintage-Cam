#include <SPI.h>
#include <TFT_eSPI.h>
#include "esp_camera.h"
#include "FS.h"                
#include "SD_MMC.h"            

TFT_eSPI tft = TFT_eSPI(); 

// --- PIN CHANGE: MOVED FROM 0 TO 12 ---
#define CAPTURE_PIN 12 

// --- CAMERA PINS ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0  // <-- This was the conflict!
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
int lastButtonState = HIGH; // Default Pullup state is HIGH

void setup() {
  // 1. Safety Delay
  delay(1000);

  // 2. Start SD Card FIRST (Priority)
  // Pin 12 is safe to use as a button as long as we don't hold it 
  // pressed DURING boot.
  if(!SD_MMC.begin("/sdcard", true)){ 
    sdCardEnabled = false;
  } else {
    sdCardEnabled = true;
  }

  // 3. Start Display
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

  // 4. Setup Button on Pin 12
  pinMode(CAPTURE_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(CAPTURE_PIN); 

  // 5. Configure Camera
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
}

void loop() {
  // A. Get RGB Frame for Preview
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  // Show Preview
  tft.pushImage(0, 0, 240, 240, (uint16_t *)fb->buf);

  // B. Button Logic (Reading GPIO 12 now)
  buttonState = digitalRead(CAPTURE_PIN);

  // Detect "Press" (High -> Low transition)
  if (buttonState == LOW && lastButtonState == HIGH) {
    
    // Release the Preview Frame
    esp_camera_fb_return(fb); 
    fb = NULL;

    if (!sdCardEnabled) {
       tft.setTextColor(TFT_RED, TFT_BLACK);
       tft.drawString("NO CARD!", 70, 20);
       delay(500);
    } else {
      // --- TAKE PHOTO SEQUENCE ---
      tft.fillCircle(120, 120, 10, TFT_WHITE); // Flash Dot
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("SAVING...", 70, 20);
      
      // 2. Switch Sensor to JPEG Mode
      sensor_t * s = esp_camera_sensor_get();
      s->set_pixformat(s, PIXFORMAT_JPEG);
      
      // Give sensor time to switch modes 
      delay(100); 

      // 3. Capture JPEG Frame
      fb = esp_camera_fb_get();
      if(fb) {
        // Create unique name
        String path = "/IMG_" + String(millis()) + ".jpg";
        fs::File file = SD_MMC.open(path.c_str(), FILE_WRITE);
        
        if(file){
          file.write(fb->buf, fb->len);
          file.close();
          tft.drawString("SAVED!   ", 70, 20);
        } else {
          tft.drawString("ERR Write", 70, 20);
        }
        
        delay(800); 
        esp_camera_fb_return(fb);
        fb = NULL; 
      }
      
      // 4. Switch back to RGB for Preview
      s->set_pixformat(s, PIXFORMAT_RGB565);
    }
  } 
  else {
    // If button not pressed, just release the frame and continue
    if(fb) esp_camera_fb_return(fb);
  }

  // Update button state for next loop
  lastButtonState = buttonState;
}
