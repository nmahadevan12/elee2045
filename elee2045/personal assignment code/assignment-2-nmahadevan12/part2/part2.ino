#include <M5StickCPlus2.h>

#define MAX_JPG_SIZE 40000
uint8_t jpgBuffer[MAX_JPG_SIZE];

bool showImage = true;
int lastImageSize = 0;

void setup() {
  // 1. Correct Initialization for Plus 2
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // 2. Correct Serial Speed (Must match Python)
  Serial.begin(115200); 

  // 3. Screen Setup (Using M5.Lcd, not M5.Led)
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.print("Waiting for photos...");
}

void loop() {
  M5.update();

  // Toggle Screen
  if (M5.BtnA.wasPressed()) {
    showImage = !showImage;
    if (!showImage) {
      M5.Lcd.fillScreen(BLACK);
    } else if (lastImageSize > 0) {
      // Redraw the last received image
      M5.Lcd.drawJpg(jpgBuffer, lastImageSize, 0, 0);
    }
  }

  // Receive Image Logic
  if (Serial.available()) {
    // 1. Read the Header (Size of image)
    String sizeStr = Serial.readStringUntil('\n');
    int imageSize = sizeStr.toInt();

    if (imageSize > 0 && imageSize < MAX_JPG_SIZE) {
      Serial.print("A"); // Send Ack (Tell Python "I'm Ready")

      // 2. Read the Image Data
      int bytesRead = Serial.readBytes(jpgBuffer, imageSize);

      // 3. Draw if successful
      if (bytesRead == imageSize) {
        lastImageSize = imageSize;
        if (showImage) {
          M5.Lcd.drawJpg(jpgBuffer, imageSize, 0, 0);
        }
        Serial.print("D"); // Send Done (Tell Python "Send next one")
      }
    }
  }
}