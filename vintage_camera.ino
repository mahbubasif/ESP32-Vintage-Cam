#include <SPI.h>
#include <TFT_eSPI.h>
#include "esp_camera.h"

// Initialize Display Driver
TFT_eSPI tft = TFT_eSPI();

// ==========================================
// CAMERA PINS FOR AI-THINKER ESP32-CAM
// ==========================================
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void setup() {
  Serial.begin(115200);

  // 1. INITIALIZE DISPLAY
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // SWAP BYTES: The camera outputs bytes in one order,
  // the screen expects them swapped. This fixes "Blue Face" or inverted colors.
  tft.setSwapBytes(false);

  // 2. CONFIGURE CAMERA
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

  // DIRECT TO SCREEN FORMAT
  config.pixel_format = PIXFORMAT_RGB565;

  // MATCH SCREEN RESOLUTION (240x240)
  config.frame_size = FRAMESIZE_240X240;

  config.jpeg_quality = 10;
  config.fb_count = 1;  // Keep at 1 for fastest direct stream

  // 3. START CAMERA
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    tft.setCursor(20, 100);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("Camera Failed");
    return;
  }

  tft.setCursor(40, 100);
  tft.setTextColor(TFT_GREEN);
  tft.println("Camera OK!");
  delay(1000);  // Pause briefly to show status
}

void loop() {
  // Capture frame
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    return;  // Frame failed, try again next loop
  }
  // Push image to display
  // x, y, width, height, buffer
  tft.pushImage(0, 0, 240, 240, (uint16_t *)fb->buf);

  // Return buffer
  esp_camera_fb_return(fb);
}