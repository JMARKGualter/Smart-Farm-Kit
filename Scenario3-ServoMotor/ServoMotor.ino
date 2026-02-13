#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>

Servo myservo;

#define servoPin 26
#define BUTTON 5

// Servo positions
int STOP  = 90;
int OPEN  = 170;
int CLOSE = 0;

bool isOpen = false;
bool lastButtonState = HIGH;

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) {
    u8g2.setFont(u8g2_font_6x10_tr);
  } 
  else if (sizeLevel <= 1.5) {
    u8g2.setFont(u8g2_font_ncenB14_tr);
  } 
  else if (sizeLevel <= 2.5) {
    u8g2.setFont(u8g2_font_logisoso20_tr);
  } 
  else {
    u8g2.setFont(u8g2_font_logisoso28_tr);
  }
}

char statusStr[20];
unsigned long moveStartTime = 0;   
int targetPosition = STOP;         
const int servoMoveDuration = 2000; 

void setup() {
  Serial.begin(9600);

  myservo.attach(servoPin);
  myservo.write(STOP);

  pinMode(BUTTON, INPUT_PULLUP);

  u8g2.begin();
}

void loop() {
  bool buttonState = digitalRead(BUTTON);


  if (lastButtonState == LOW && buttonState == HIGH) {
    if (!isOpen) {
      targetPosition = OPEN;   
      moveStartTime = millis();
    } else {
      targetPosition = CLOSE;  
      moveStartTime = millis();
    }
  }
  lastButtonState = buttonState;

 
  myservo.write(targetPosition);

 
  if (millis() - moveStartTime >= servoMoveDuration) {
    myservo.write(STOP);
    isOpen = (targetPosition == OPEN); 
  }

  // ---- DISPLAY STATUS ----
  sprintf(statusStr, "STATUS: %s", isOpen ? "OPEN" : "CLOSED");

  u8g2.clearBuffer();
  setTextSize(1);
  u8g2.drawStr(33, 15, "SERVO DOOR");
  u8g2.drawStr(33, 35, statusStr);
  u8g2.drawStr(33, 55, "BTN TO TOGGLE");
  u8g2.sendBuffer();

  delay(50); 
}
