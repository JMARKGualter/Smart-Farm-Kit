#include <Wire.h>
#include <U8g2lib.h>

#define PyroelectricPIN 23

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---- Fake setTextSize for U8g2 ----
void setTextSize(uint8_t sizeLevel) {
  switch(sizeLevel) {
    case 1:
      u8g2.setFont(u8g2_font_6x10_tr);        // small
      break;
    case 2:
      u8g2.setFont(u8g2_font_ncenB14_tr);     // medium
      break;
    case 3:
      u8g2.setFont(u8g2_font_logisoso20_tr);  // large
      break;
    case 4:
      u8g2.setFont(u8g2_font_logisoso28_tr);  // extra large
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PyroelectricPIN, INPUT);
  u8g2.begin();
}

void loop() {
  int ReadValue = digitalRead(PyroelectricPIN);

  u8g2.clearBuffer();

  if (ReadValue) {
    Serial.println("Someone");

    setTextSize(1.2); // big
    u8g2.drawStr(80, 50, "MOTION");

    setTextSize(1.2); // medium
    u8g2.drawStr(75, 60, "DETECTED");
  } 
  else {
    Serial.println("No one");

    setTextSize(1.2); // very big
    u8g2.drawStr(80, 50, "CLEAR");
  }

  u8g2.sendBuffer();
  delay(500);
}
