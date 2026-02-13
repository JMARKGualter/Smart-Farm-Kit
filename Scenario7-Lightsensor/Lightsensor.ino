#include <Wire.h>
#include <U8g2lib.h>

#define PhotocecllPin 34
#define LED 27
#define ButtonPin 5   

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}


char lightStr[30];
char ledStr[20];
bool systemOn = true;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(PhotocecllPin, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(ButtonPin, INPUT_PULLUP);

  u8g2.begin();
}

void loop() {
 
  bool buttonState = digitalRead(ButtonPin);
  if (lastButtonState == HIGH && buttonState == LOW) {
    systemOn = !systemOn;   
    if (!systemOn) {
      digitalWrite(LED, LOW);  
    }
    delay(200); // debounce
  }
  lastButtonState = buttonState;


  int ReadValue = analogRead(PhotocecllPin);
  if (systemOn) {
    if (ReadValue < 300) {
      digitalWrite(LED, HIGH);
      sprintf(ledStr, "LED: ON");
    } else {
      digitalWrite(LED, LOW);
      sprintf(ledStr, "LED: OFF");
    }
  } else {
    sprintf(ledStr, "LED: OFF");
  }

  sprintf(lightStr, "Light: %d", ReadValue);

 
  u8g2.clearBuffer();
  setTextSize(1);
  u8g2.drawStr(33, 15, "Photoresistor");
  u8g2.drawStr(33, 35, lightStr);
 
  char statusStr[32];
  sprintf(statusStr, "System: %s", systemOn ? "ON" : "OFF");
  u8g2.drawStr(33, 50, statusStr);
  u8g2.drawStr(33, 65, ledStr);

  u8g2.sendBuffer();

  delay(300);
}
